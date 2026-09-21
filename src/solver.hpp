#pragma once
// =============================================================================
//  Multi-street Discounted CFR.
//
//  Memory
//    A flop tree replicates the river betting subtree across 49x48 runouts, so
//    the information sets are stored as float, not double, and there are only
//    two buffers: cumulative regret and cumulative strategy. The current
//    strategy is regret-matched on the fly, and the average strategy is
//    normalised on demand, so neither costs a third buffer.
//
//    A node's block holds one entry per action per combo its OWNER can hold,
//    not per combo on the board. A board offers 1176 and a real range has a
//    couple of hundred, so that alone is worth about 6x.
//
//  Updates
//    Alternating: iteration t traverses for one player only. That is what the
//    DCFR paper uses, it halves the per-iteration cost, and it removes the need
//    to freeze a shared current strategy between two traversals.
//
//  Chance nodes
//    v(h) = (1/D) * sum over dealable cards c not in h of v(h | c),
//    with D = 52 - |board so far| - 2, and every combo using c getting zero
//    reach below. The subtrees under one chance node touch disjoint memory, so
//    the fan-out is where the work is parallelised.
// =============================================================================

#include "config.hpp"
#include "range.hpp"
#include "tree.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
//  A persistent pool. Spawning threads per parallel section was costing more
//  than the work itself on the smaller trees: a turn iteration fans out at four
//  points, so at 16 threads that was 64 thread creations for 2.4 ms of work.
//  Here the threads are created once and parked on a condition variable.
//
//  The caller takes part as worker 0, so `run` uses every core including its
//  own, and work is handed out by an atomic counter so an uneven fan-out (a
//  monotone flop has orbits of very different sizes) still balances.
// -----------------------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(int n) : n_(n < 1 ? 1 : n) {
        for (int t = 1; t < n_; ++t) th_.emplace_back([this, t]() { worker(t); });
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(m_);
            stop_.store(true);
            ++gen_;
            agen_.store(gen_);
        }
        cv_start_.notify_all();
        for (std::thread& t : th_) if (t.joinable()) t.join();
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    int size() const { return n_; }

    void run(int total, const std::function<void(int, int)>& fn) {
        if (total <= 0) return;
        if (n_ <= 1) {
            for (int i = 0; i < total; ++i) fn(i, 0);
            return;
        }
        fn_    = &fn;
        total_ = total;
        next_.store(0, std::memory_order_relaxed);
        busy_.store(n_ - 1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(m_);
            ++gen_;
            agen_.store(gen_, std::memory_order_release);   // publishes fn_/total_
        }
        cv_start_.notify_all();

        drain(0);                                            // the caller works too

        for (int spin = 0; spin < SPIN && busy_.load(std::memory_order_acquire) != 0; ++spin)
            relax();
        if (busy_.load(std::memory_order_acquire) != 0) {
            std::unique_lock<std::mutex> lk(m_);
            cv_end_.wait(lk, [this]() { return busy_.load() == 0; });
        }
    }

private:
    // A parallel section here can be as short as a millisecond, and parking on a
    // condition variable costs tens of microseconds each way. Workers spin for a
    // moment looking for the next section before they sleep, which is the
    // difference between the fan-out being worth parallelising and not.
    enum { SPIN = 0 };

    static void relax() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #if defined(__GNUC__) || defined(__clang__)
        __builtin_ia32_pause();
    #else
        std::this_thread::yield();
    #endif
#else
        std::this_thread::yield();
#endif
    }

    void drain(int tid) {
        int i;
        while ((i = next_.fetch_add(1, std::memory_order_relaxed)) < total_)
            (*fn_)(i, tid);
    }

    void worker(int tid) {
        unsigned long long seen = 0;
        for (;;) {
            bool have = false;
            for (int spin = 0; spin < SPIN; ++spin) {
                if (agen_.load(std::memory_order_acquire) != seen) { have = true; break; }
                relax();
            }
            if (!have) {
                std::unique_lock<std::mutex> lk(m_);
                cv_start_.wait(lk, [this, &seen]() {
                    return stop_.load() || agen_.load() != seen;
                });
            }
            seen = agen_.load(std::memory_order_acquire);
            if (stop_.load()) return;

            drain(tid);

            if (busy_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard<std::mutex> lk(m_);
                cv_end_.notify_all();
            }
        }
    }

    int                                   n_;
    std::vector<std::thread>              th_;
    std::mutex                            m_;
    std::condition_variable               cv_start_, cv_end_;
    const std::function<void(int, int)>*  fn_    = nullptr;
    int                                   total_ = 0;
    std::atomic<int>                      next_{0};
    std::atomic<int>                      busy_{0};
    unsigned long long                    gen_ = 0;      // guarded by m_
    std::atomic<unsigned long long>       agen_{0};      // published copy
    std::atomic<bool>                     stop_{false};
};

// -----------------------------------------------------------------------------
struct NodeView {
    std::vector<double>              own_reach;
    std::vector<double>              opp_reach;
    std::vector<double>              compat;
    std::vector<double>              cfv;
    std::vector<std::vector<double>> action_cfv;
    bool ok = false;
};

// Cuando toca sacar un aviso de progreso.
//
// Va por el contador GLOBAL de iteraciones, no por el de la llamada. Cuando el
// objetivo de precision esta puesto, el solve avanza a trozos pequenos para
// poder mirar la precision a menudo, y mirandolo por el contador local cada
// trozo sacaba su primera y su ultima: dos avisos cada sesenta y cuatro
// iteraciones en vez de uno cada mil.
inline bool should_report(long long t_done, int report_every) {
    return report_every > 0 && (t_done % report_every == 0);
}

class DCFRSolver {
public:
    DCFRSolver(GameTree& tree, const Deal& deal,
               const std::vector<double>& oop, const std::vector<double>& ip)
        : T(tree), D_(deal), nh_(deal.num()), deckN_(deal.deckN()) {
        range_[0] = oop;
        range_[1] = ip;
        range_[0].resize(static_cast<size_t>(nh_), 0.0);
        range_[1].resize(static_cast<size_t>(nh_), 0.0);

        // A board has 1176 possible combos but a real range holds a couple of
        // hundred. Every per-hand loop used to walk all 1176 and every block
        // stored all 1176; the dead ones carry zero reach, so nothing ever reads
        // their strategies or regrets. Laying the memory out over the live ones
        // and looping over them is the single biggest saving in the traversal,
        // and it changes no number that anybody looks at.
        for (int p = 0; p < 2; ++p) {
            live_[p].clear();
            livemask_[p].assign(static_cast<size_t>(nh_), 0);
            for (int h = 0; h < nh_; ++h)
                if (range_[p][static_cast<size_t>(h)] > 0.0) {
                    live_[p].push_back(h);
                    livemask_[p][static_cast<size_t>(h)] = 1;
                }
            nlive_[p] = static_cast<int>(live_[p].size());
            // Per card, the combos that use it AND that this player can hold.
            // The chance node subtracts the dealt card's combos out of a sum it
            // only ever built over the live ones.
            for (int c = 0; c < 52; ++c) {
                with_card_live_[p][c].clear();
                for (int h : D_.with_card[static_cast<size_t>(c)])
                    if (livemask_[p][static_cast<size_t>(h)])
                        with_card_live_[p][c].push_back(h);
            }
        }
        T.layout(nlive_[0], nlive_[1]);

        // The showdown sweep walks hands in strength order. Combos neither
        // player can hold add nothing to the running sums and are never asked
        // about, so a strength order restricted to the union of the two ranges
        // gives an identical answer in a fraction of the steps -- and after
        // everything else was trimmed, that sweep is 62% of an iteration.
        {
            std::vector<unsigned char> inU(static_cast<size_t>(nh_), 0);
            for (int p = 0; p < 2; ++p)
                for (int h : live_[p]) inU[static_cast<size_t>(h)] = 1;
            sdn_ = 0;
            live_union_.clear();
            for (int h = 0; h < nh_; ++h)
                if (inU[static_cast<size_t>(h)]) { ++sdn_; live_union_.push_back(h); }
            sd_pack_.assign(static_cast<size_t>(D_.num_runouts) * sdn_, SdEntry());
            std::vector<SdEntry> one;
            for (int r = 0; r < D_.num_runouts; ++r) {
                pack_sweep(D_, &D_.order[static_cast<size_t>(r) * nh_],
                           &D_.scores[static_cast<size_t>(r) * nh_], nh_,
                           inU.data(), one);
                std::copy(one.begin(), one.end(),
                          sd_pack_.begin() + static_cast<size_t>(r) * sdn_);
            }
        }

        regret.assign(static_cast<size_t>(T.mem_size), 0.0f);
        strat_sum.assign(static_cast<size_t>(T.mem_size), 0.0f);
        // One stamp per decision node per runout, so a block can be brought up
        // to date the moment it is read instead of by a pass over everything.
        stamp_.assign(static_cast<size_t>(T.stamp_size), 0);
        logpos_.assign(1, 0.0);
        logneg_.assign(1, 0.0);

        compute_equity(D_, range_[1], eq_[0]);
        compute_equity(D_, range_[0], eq_[1]);
        compat_counts(D_, range_[1].data(), nh_, start_compat_[0]);
        compat_counts(D_, range_[0].data(), nh_, start_compat_[1]);

        nthreads_ = static_cast<int>(std::thread::hardware_concurrency());
        if (cfg::THREADS > 0) nthreads_ = cfg::THREADS;
        else if (const char* e = std::getenv("SOLVER_THREADS")) {
            const int v = std::atoi(e);
            if (v > 0) nthreads_ = v;
        }
        if (nthreads_ < 1) nthreads_ = 1;
        if (nthreads_ > 64) nthreads_ = 64;
        // The main traversal and the parallel children must never share a
        // frame stack: the children restart at depth 0 while their ancestors
        // still hold frames above them.
        main_scr_.init(T.max_depth + 4, nh_);
        scr_.resize(static_cast<size_t>(nthreads_));
        for (Scratch& s : scr_) s.init(T.max_depth + 4, nh_);
        pool_.reset(new ThreadPool(nthreads_));
    }

    // ---- accessors -------------------------------------------------------
    int              num_hands()  const { return nh_; }
    const Deal&      deal()       const { return D_; }
    const GameTree&  tree()       const { return T; }
    int              iterations_done() const { return t_done; }
    int              threads()    const { return nthreads_; }
    // Rough breakdown of where an iteration goes, for tuning.
    double t_traverse() const { return t_trav_; }
    double t_discount() const { return t_disc_; }
    const std::vector<double>& range(int p) const { return range_[p]; }

    // The DCFR discount for one iteration: positive and negative cumulative
    // regret decay at different rates. This used to be written out in three
    // places -- the per-iteration bookkeeping, the traversal's fast path, and
    // the reconstruction after loading a saved tree. Three copies of one
    // formula, only one of which anything actually applied, so a change to
    // either of the others would have gone unnoticed.
    static void discount_factors(int t, double& dpos, double& dneg) {
        const double td = static_cast<double>(t);
        const double ta = std::pow(td, cfg::DCFR_ALPHA);
        const double tb = std::pow(td, cfg::DCFR_BETA);
        dpos = ta / (ta + 1.0);
        dneg = tb / (tb + 1.0);
    }

    // What the traversal would multiply a regret by to bring it from iteration
    // `from` up to `to`. The one-step case is every block every iteration; the
    // multi-step case goes through the log prefix sums, and the two agreeing is
    // an invariant nothing else would notice breaking.
    double catchup_factor(int from, int to, bool positive) const {
        if (from < 0 || to < from || to >= static_cast<int>(logpos_.size())) return 0.0;
        const std::vector<double>& L = positive ? logpos_ : logneg_;
        return std::exp(L[static_cast<size_t>(to)] - L[static_cast<size_t>(from)]);
    }

    // ---- persistence -----------------------------------------------------
    //  Everything needed to carry on solving where the file left off: both
    //  buffers, the deferred-discount stamps and the lazy strategy-sum scale.
    //  The caller has already matched the tree, so only the sizes are checked.
    // FNV-1a over the counters and both buffers, eight bytes at a time. Not a
    // cryptographic hash and not trying to be: it exists to catch a truncated
    // or half-written file, which is the way a 55 MB solve actually goes wrong.
    unsigned long long state_hash() const {
        unsigned long long h = 1469598103934665603ULL;
        auto mix = [&h](const void* p, size_t bytes) {
            const unsigned char* b = static_cast<const unsigned char*>(p);
            size_t i = 0;
            for (; i + 8 <= bytes; i += 8) {
                unsigned long long w;
                std::memcpy(&w, b + i, 8);
                h = (h ^ w) * 1099511628211ULL;
            }
            for (; i < bytes; ++i) { h ^= b[i]; h *= 1099511628211ULL; }
        };
        mix(&t_done, sizeof t_done);
        mix(&strat_w_, sizeof strat_w_);
        mix(regret.data(), regret.size() * sizeof(float));
        mix(strat_sum.data(), strat_sum.size() * sizeof(float));
        mix(stamp_.data(), stamp_.size() * sizeof(int));
        return h;
    }

    // Lo que `save_state` va a escribir, EXACTO. No es una estimacion: es la
    // cuenta de lo que escribe la funcion de aqui abajo, campo por campo. Con
    // eso se puede saber si un arbol cabe en el disco ANTES de empezar a
    // escribirlo, que es la unica forma de que el aviso llegue a tiempo.
    long long state_bytes() const {
        return static_cast<long long>(sizeof t_done) +
               static_cast<long long>(sizeof strat_w_) +
               2 * static_cast<long long>(sizeof(long long)) +
               2 * static_cast<long long>(regret.size()) *
                   static_cast<long long>(sizeof(float)) +
               static_cast<long long>(stamp_.size()) *
                   static_cast<long long>(sizeof(int));
    }

    bool save_state(std::FILE* f) const {
        const long long n = static_cast<long long>(regret.size());
        const long long b = static_cast<long long>(stamp_.size());
        if (std::fwrite(&t_done, sizeof t_done, 1, f) != 1)       return false;
        if (std::fwrite(&strat_w_, sizeof strat_w_, 1, f) != 1)   return false;
        if (std::fwrite(&n, sizeof n, 1, f) != 1)                 return false;
        if (std::fwrite(&b, sizeof b, 1, f) != 1)                 return false;
        if (n && std::fwrite(strat_sum.data(), sizeof(float), static_cast<size_t>(n), f)
                 != static_cast<size_t>(n)) return false;
        if (n && std::fwrite(regret.data(), sizeof(float), static_cast<size_t>(n), f)
                 != static_cast<size_t>(n)) return false;
        if (b && std::fwrite(stamp_.data(), sizeof(int), static_cast<size_t>(b), f)
                 != static_cast<size_t>(b)) return false;
        return true;
    }

    bool load_state(std::FILE* f, std::string& e) {
        long long n = 0, b = 0;
        int td = 0;
        double sw = 1.0;
        if (std::fread(&td, sizeof td, 1, f) != 1 ||
            std::fread(&sw, sizeof sw, 1, f) != 1 ||
            std::fread(&n, sizeof n, 1, f) != 1 ||
            std::fread(&b, sizeof b, 1, f) != 1) { e = "truncated header"; return false; }
        if (n != static_cast<long long>(regret.size()) ||
            b != static_cast<long long>(stamp_.size())) {
            e = "the file was saved for a differently shaped tree";
            return false;
        }
        if (n && std::fread(strat_sum.data(), sizeof(float), static_cast<size_t>(n), f)
                 != static_cast<size_t>(n)) { e = "truncated strategy"; return false; }
        if (n && std::fread(regret.data(), sizeof(float), static_cast<size_t>(n), f)
                 != static_cast<size_t>(n)) { e = "truncated regrets"; return false; }
        if (b && std::fread(stamp_.data(), sizeof(int), static_cast<size_t>(b), f)
                 != static_cast<size_t>(b)) { e = "truncated stamps"; return false; }
        t_done   = td;
        strat_w_ = sw;
        // The log prefix sums are a pure function of the iteration count and the
        // DCFR exponents, so they are recomputed rather than stored.
        logpos_.assign(1, 0.0);
        logneg_.assign(1, 0.0);
        for (int k = 1; k <= t_done; ++k) {
            double dp, dn;
            discount_factors(k, dp, dn);
            logpos_.push_back(logpos_.back() + std::log(dp > 1e-300 ? dp : 1e-300));
            logneg_.push_back(logneg_.back() + std::log(dn > 1e-300 ? dn : 1e-300));
        }
        return true;
    }

    double   equity(int p, int h) const { return eq_[p][static_cast<size_t>(h)]; }
    double   start_compat(int p, int h) const { return start_compat_[p][static_cast<size_t>(h)]; }

    // ---- nodelocking -----------------------------------------------------
    // `inst` es el runout que se bloquea; -1 bloquea el nodo en todos, que es
    // lo que hacia antes y lo que necesitan las configuraciones ya guardadas.
    bool lock_hands(int ci, int nid, long long inst, const std::vector<int>& hands,
                    const std::vector<double>& probs) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return false;
        RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        BetTree& bt = rc.tree;
        if (nid < 0 || nid >= static_cast<int>(bt.nodes.size())) return false;
        Node& n = bt.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) return false;
        if (static_cast<int>(probs.size()) != n.num_actions) return false;
        if (inst >= rc.instances) return false;

        double s = 0.0;
        for (double p : probs) { if (p < 0.0) return false; s += p; }
        if (s <= 1e-12) return false;

        const size_t need = static_cast<size_t>(n.num_actions) * static_cast<size_t>(nh_);
        Node::LockBlock* b = nullptr;
        if (inst < 0) {
            // Todos los runouts: un solo bloque y `slot` vacio.
            if (n.lock_blk.empty()) n.lock_blk.resize(1);
            n.lock_slot.clear();
            b = &n.lock_blk[0];
        } else {
            // Un runout concreto. Si ya habia un bloque "para todos", se le da
            // su sitio a cada instancia antes de anadir el nuevo: si no, el
            // lock viejo desapareceria en silencio al poner el primero nuevo.
            if (n.lock_slot.empty()) {
                const int base = n.lock_blk.empty() ? -1 : 0;
                n.lock_slot.assign(static_cast<size_t>(rc.instances), base);
            }
            if (static_cast<long long>(n.lock_slot.size()) != rc.instances)
                n.lock_slot.assign(static_cast<size_t>(rc.instances), -1);
            int& slot = n.lock_slot[static_cast<size_t>(inst)];
            if (slot < 0 || (n.lock_blk.size() == 1 && n.lock_slot.size() > 1)) {
                // Bloque propio para esta instancia: escribir sobre uno
                // compartido cambiaria tambien los demas runouts.
                Node::LockBlock nuevo;
                if (slot >= 0) nuevo = n.lock_blk[static_cast<size_t>(slot)];
                n.lock_blk.push_back(nuevo);
                slot = static_cast<int>(n.lock_blk.size()) - 1;
            }
            b = &n.lock_blk[static_cast<size_t>(slot)];
        }
        // Se comprueba el TAMANO, no si esta vacio. Un bloque puede sobrevivir
        // en el nodo desde otro tablero o desde un arbol con otro numero de
        // acciones, y entonces escribir en el con las medidas de ahora se sale
        // del monticulo. Con el binario optimizado eso era un segfault en un
        // sitio que no tenia nada que ver.
        if (b->hand.size() != static_cast<size_t>(nh_) || b->strategy.size() != need) {
            b->hand.assign(static_cast<size_t>(nh_), static_cast<uint8_t>(0));
            b->strategy.assign(need, 0.0);
        }
        for (int h : hands) {
            if (h < 0 || h >= nh_) continue;
            b->hand[static_cast<size_t>(h)] = 1u;
            for (int a = 0; a < n.num_actions; ++a)
                b->strategy[static_cast<size_t>(a) * nh_ + h] =
                    probs[static_cast<size_t>(a)] / s;
        }
        n.is_locked = true;
        return true;
    }

    // El mix viene por CODIGO de accion -- "B33", "R66", "X" -- y no por tipo.
    //
    // Por tipo no se podia decir cual: un river con dos tamanos tiene dos
    // acciones de apostar y las dos son AK_BET, asi que "apuesta el 90%" caia
    // en una de las dos y la otra se quedaba como estaba, en silencio. Con el
    // board de dos tamanos del usuario eso es justo el nodo que se quiere
    // tocar.
    //
    // Un token que no sea un codigo de este nodo se intenta como TIPO, que es
    // lo que escribe la gente en la consola ("B=0.9") y lo que llevan dentro
    // las configuraciones guardadas antes de esto.
    bool lock_hands_coded(int ci, int nid, long long inst, const std::vector<int>& hands,
                          const std::vector<std::pair<std::string, double>>& mix) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return false;
        const BetTree& bt = T.ctx[static_cast<size_t>(ci)].tree;
        if (nid < 0 || nid >= static_cast<int>(bt.nodes.size())) return false;
        const Node& n = bt.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) return false;
        std::vector<double> probs(static_cast<size_t>(n.num_actions), 0.0);
        for (const auto& pr : mix) {
            int a = -1;
            for (int k = 0; k < n.num_actions; ++k)
                if (bt.act(n, k).code == pr.first) { a = k; break; }
            if (a < 0) {
                ActionKind ak;
                if (!parse_action_kind(pr.first, ak)) return false;
                a = bt.action_index(n, ak);
            }
            if (a < 0) return false;
            probs[static_cast<size_t>(a)] += pr.second;
        }
        return lock_hands(ci, nid, inst, hands, probs);
    }

    bool clear_locks(int ci, int nid) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return false;
        BetTree& bt = T.ctx[static_cast<size_t>(ci)].tree;
        if (nid < 0 || nid >= static_cast<int>(bt.nodes.size())) return false;
        Node& n = bt.nodes[static_cast<size_t>(nid)];
        n.is_locked = false;
        n.lock_blk.clear();
        n.lock_slot.clear();
        return true;
    }

    bool is_hand_locked(int ci, int nid, long long inst, int h) const {
        const Node& n = T.ctx[static_cast<size_t>(ci)].tree.nodes[static_cast<size_t>(nid)];
        const Node::LockBlock* b = n.lock_for(inst);
        return b && !b->hand.empty() && b->hand[static_cast<size_t>(h)] != 0u;
    }

    // ---- main loop -------------------------------------------------------
    // `progress` counts iterations finished in this call; raising `cancel` stops
    // the loop at an iteration boundary, and what has accumulated so far is
    // still a valid average strategy -- just less converged.
    void run(int iterations, int report_every,
             std::atomic<int>* progress = nullptr,
             std::atomic<bool>* cancel = nullptr) {
        std::vector<double> out(static_cast<size_t>(nh_), 0.0);
        for (int k = 1; k <= iterations; ++k) {
            if (cancel && cancel->load()) break;
            ++t_done;
            const int trav = t_done % 2;      // alternating updates
            set_discount_target(t_done);
            const auto tA = std::chrono::steady_clock::now();
            cfr(0, 0, T.ctx[0].tree.root, range_[trav].data(), range_[1 - trav].data(),
                trav, main_scr_, 0, out.data(), true);
            const auto tB = std::chrono::steady_clock::now();
            discount(t_done);
            const auto tC = std::chrono::steady_clock::now();
            t_trav_ += std::chrono::duration<double>(tB - tA).count();
            t_disc_ += std::chrono::duration<double>(tC - tB).count();

            if (should_report(t_done, report_every)) {
                const double ev = root_ev(0);
                const double ev1 = (cfg::RAKE_PCT > 0.0) ? root_ev(1) : cfg::POT0 - ev;
                std::printf("   iter %-8d  OOP %8.4f  IP %8.4f\n", t_done, ev, ev1);
                std::fflush(stdout);
            }
            if (progress) progress->store(k);
        }
    }

    // Average EV of the whole game for a player, in chips, under the average
    // strategy. The root belongs to one player, so this cannot go through
    // query(): it traverses for `p` whoever owns the root.
    double root_ev(int p) {
        std::vector<double> v(static_cast<size_t>(nh_), 0.0);
        cfr(0, 0, T.ctx[0].tree.root, range_[p].data(), range_[1 - p].data(),
            p, main_scr_, 0, v.data(), true, MODE_EVAL);
        double num = 0.0, den = 0.0;
        for (int h = 0; h < nh_; ++h) {
            const double rh = range_[p][static_cast<size_t>(h)];
            if (rh <= 0.0) continue;
            num += rh * v[static_cast<size_t>(h)];
            den += rh * start_compat_[p][static_cast<size_t>(h)];
        }
        return den > 1e-12 ? num / den + cfg::POT0 * 0.5 : 0.0;
    }

    // EV in chips at any node, for EITHER player -- including the one who is not
    // acting there. Walks the reaches down to the node and evaluates the subtree
    // from that player's side.
    double node_ev_player(int ci, int nid, long long inst, int player) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return 0.0;
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) return 0.0;
        std::vector<double> rs, ro;
        if (!reach_at(ci, nid, inst, player, rs, ro)) return 0.0;
        std::vector<double> compat;
        compat_counts(D_, ro.data(), nh_, compat);
        std::vector<double> v(static_cast<size_t>(nh_), 0.0);
        cfr(ci, inst, nid, rs.data(), ro.data(), player, main_scr_, 0, v.data(),
            true, MODE_EVAL);
        double num = 0.0, den = 0.0;
        for (int h = 0; h < nh_; ++h) {
            const double w = rs[static_cast<size_t>(h)] * compat[static_cast<size_t>(h)];
            if (w <= 0.0) continue;
            den += w;
            num += w * (v[static_cast<size_t>(h)] / compat[static_cast<size_t>(h)] +
                        cfg::POT0 * 0.5);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    // Constrained best response: locked hands keep playing their frozen mix.
    double best_response_value(int br) {
        std::vector<double> v(static_cast<size_t>(nh_), 0.0);
        cfr(0, 0, T.ctx[0].tree.root, range_[br].data(), range_[1 - br].data(),
            br, main_scr_, 0, v.data(), true, /*mode*/ MODE_BR);
        double num = 0.0, den = 0.0;
        for (int h = 0; h < nh_; ++h) {
            const double rh = range_[br][static_cast<size_t>(h)];
            if (rh <= 0.0) continue;
            num += rh * v[static_cast<size_t>(h)];
            den += rh * start_compat_[br][static_cast<size_t>(h)];
        }
        return den > 1e-12 ? num / den : 0.0;
    }
    // La SUMA de lo que ganaria cada jugador respondiendo lo mejor posible al
    // otro. Es la que usan las comprobaciones, porque con la suma la frase "lo
    // que gana un jugador cabe dentro de la explotabilidad" es cierta sin mas.
    // Lo que se ENSENA no es esto: es expl_shown(), aqui debajo.
    double exploitability() { return best_response_value(0) + best_response_value(1); }

    // La mejor respuesta EN UN NODO: contra la estrategia de la solucion, que
    // haria quien juega para explotarla al maximo.
    //
    // No es lo mismo que mirar el mejor action_cfv del nodo. Ese compara las
    // acciones suponiendo que DESPUES se sigue jugando la solucion; esto es la
    // mejor respuesta de verdad, que sigue explotando calle abajo. La
    // diferencia es justo lo que se quiere ver: cuanto te cuesta un desvio
    // contra alguien que no para de castigarlo.
    //
    // Reutiliza la travesia que ya calcula la explotabilidad; lo unico nuevo es
    // apuntar los valores por accion al pasar por el nodo pedido.
    bool br_at(int ci, int nid, long long inst, std::vector<double>& vals, int& A_out) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return false;
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) return false;
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) return false;
        if (inst < 0 || inst >= rc.instances) return false;

        br_ci_ = ci; br_nid_ = nid; br_inst_ = inst;
        br_val_.clear();
        br_actions_ = 0;
        const int br = n.player;          // la mejor respuesta es de quien actua
        std::vector<double> v(static_cast<size_t>(nh_), 0.0);
        cfr(0, 0, T.ctx[0].tree.root, range_[br].data(), range_[1 - br].data(),
            br, main_scr_, 0, v.data(), true, MODE_BR);
        br_ci_ = -1;
        if (br_val_.empty()) return false;   // el nodo no se visito
        vals.swap(br_val_);
        A_out = br_actions_;
        return true;
    }

    // ---- reading one node ------------------------------------------------
    //  Walks the reaches down to (ctx, node, instance) under the average
    //  strategy, then evaluates the subtree from there. On a flop node that is
    //  about the cost of one iteration; on a river node it is instant.
    bool query(int ci, int nid, long long inst, NodeView& out) {
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return false;
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const BetTree&  bt = rc.tree;
        if (nid < 0 || nid >= static_cast<int>(bt.nodes.size())) return false;
        const Node& n = bt.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) return false;
        if (inst < 0 || inst >= rc.instances) return false;
        const int player = n.player;   // a node's strategy belongs to its owner

        std::vector<double> rs(static_cast<size_t>(nh_)), ro(static_cast<size_t>(nh_));
        if (!reach_at(ci, nid, inst, player, rs, ro)) return false;

        out.own_reach = rs;
        out.opp_reach = ro;
        compat_counts(D_, ro.data(), nh_, out.compat);

        const int A = n.num_actions;
        out.cfv.assign(static_cast<size_t>(nh_), 0.0);
        out.action_cfv.assign(static_cast<size_t>(A), std::vector<double>(static_cast<size_t>(nh_), 0.0));

        // Values of each action, then the strategy-weighted node value.
        //
        // The strategy has to be THIS instance's, not the one averaged over
        // every runout of this node. Using the block average made the node
        // value disagree with the frequencies displayed beside it -- those come
        // from the instance -- and made it depend on whether suits were
        // collapsed, because the block average sums over stored instances
        // without weighting by orbit size, and collapsing changes how many
        // stored instances there are.
        std::vector<double> st(static_cast<size_t>(A) * nh_);
        avg_strategy_inst(ci, nid, inst, st.data());
        for (int a = 0; a < A; ++a) {
            std::vector<double> nr(static_cast<size_t>(nh_));
            for (int h = 0; h < nh_; ++h)
                nr[static_cast<size_t>(h)] = rs[static_cast<size_t>(h)] * st[static_cast<size_t>(a) * nh_ + h];
            cfr(ci, inst, bt.child(n, a), nr.data(), ro.data(), player, main_scr_, 0,
                out.action_cfv[static_cast<size_t>(a)].data(), true, MODE_EVAL);
        }
        for (int h = 0; h < nh_; ++h) {
            double s = 0.0;
            for (int a = 0; a < A; ++a)
                s += st[static_cast<size_t>(a) * nh_ + h] * out.action_cfv[static_cast<size_t>(a)][static_cast<size_t>(h)];
            out.cfv[static_cast<size_t>(h)] = s;
        }
        out.ok = true;
        return true;
    }

    // Average strategy of a node, as an A*nh block.
    void avg_strategy_block(int ci, int nid, double* st) const {
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        const int A = n.num_actions;
        // Averaged over every runout instance of this node: what the player does
        // at this decision point across the board, which is what a strategy
        // panel should show.
        const std::vector<int>& LN = live_[n.player];
        const int nl = nlive_[n.player];
        // Each stored instance counts for as many runouts as its orbit holds.
        // Summing them raw would let a monotone flop, where a heart turn leaves
        // 22 river orbits and a non-heart 35, weigh the rare shapes as heavily
        // as the common ones -- and would make the answer depend on whether
        // suits were collapsed, which it must not.
        const int levels = rc.street - T.start;
        std::vector<double> acc(static_cast<size_t>(A) * nh_, 0.0);
        std::vector<int> perms;
        for (long long b = 0; b < rc.instances; ++b) {
            D_.orbit_members(b, levels, perms);
            if (perms.empty()) continue;
            const float* ss = &strat_sum[static_cast<size_t>(block_index(rc, b, n, 0))];
            for (int p : perms) {
                const std::vector<int>& fwd = D_.perm_combo[static_cast<size_t>(p)];
                for (int a = 0; a < A; ++a)
                    for (int i = 0; i < nl; ++i)
                        acc[static_cast<size_t>(a) * nh_ +
                            fwd[static_cast<size_t>(LN[static_cast<size_t>(i)])]] +=
                            ss[static_cast<size_t>(a) * nl + i];
            }
        }
        normalise_block(n, acc.data(), st, A);

        // Y una mano bloqueada IGUAL en todos los runouts se ensena tal cual.
        //
        // El promedio de arriba ya sale bien -- strat_sum acumula la estrategia
        // bloqueada como cualquier otra -- pero sale bien con el error de la
        // suma en coma flotante, y aqui el numero que el usuario puso a mano
        // tiene que volver a salir exacto. Solo se puede cuando el lock es el
        // mismo en todos los runouts: si bloqueaste el 3s al 30% y el 4d no,
        // "que hace en este nodo" no tiene una respuesta unica, y entonces el
        // promedio es justo lo que hay que ensenar.
        if (n.is_locked) {
            for (int h = 0; h < nh_; ++h) {
                const Node::LockBlock* uno = nullptr;
                bool todos = true;
                for (long long b = 0; b < rc.instances && todos; ++b) {
                    const Node::LockBlock* lb = n.lock_for(b);
                    if (!lb || lb->hand.empty() || !lb->hand[static_cast<size_t>(h)]) {
                        todos = false;
                        break;
                    }
                    if (!uno) uno = lb;
                    else if (uno != lb) {
                        for (int a = 0; a < A; ++a)
                            if (std::fabs(uno->strategy[static_cast<size_t>(a) * nh_ + h] -
                                          lb->strategy[static_cast<size_t>(a) * nh_ + h]) > 1e-12) {
                                todos = false;
                                break;
                            }
                    }
                }
                if (todos && uno)
                    for (int a = 0; a < A; ++a)
                        st[static_cast<size_t>(a) * nh_ + h] =
                            uno->strategy[static_cast<size_t>(a) * nh_ + h];
            }
        }
    }

    // Average strategy of one runout instance. Allocation-free: it runs inside
    // the evaluation traversal.
    void avg_strategy_inst(int ci, int nid, long long inst, double* st) const {
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        const int A = n.num_actions;
        const float* ss = &strat_sum[static_cast<size_t>(block_index(rc, inst, n, 0))];
        const std::vector<int>& LN = live_[n.player];
        const int nl = nlive_[n.player];
        const double u = 1.0 / static_cast<double>(A);
        // Combos the actor cannot hold get the uniform mix. Nothing reads them
        // -- their reach is zero -- but leaving them finite keeps any later
        // multiply harmless.
        for (int h = 0; h < nh_; ++h)
            for (int a = 0; a < A; ++a) st[static_cast<size_t>(a) * nh_ + h] = u;
        const Node::LockBlock* lb = n.lock_for(inst);
        for (int i = 0; i < nl; ++i) {
            const int h = LN[static_cast<size_t>(i)];
            if (lb && lb->hand[static_cast<size_t>(h)]) {
                for (int a = 0; a < A; ++a)
                    st[static_cast<size_t>(a) * nh_ + h] =
                        lb->strategy[static_cast<size_t>(a) * nh_ + h];
                continue;
            }
            double s = 0.0;
            for (int a = 0; a < A; ++a) s += ss[static_cast<size_t>(a) * nl + i];
            if (s > 1e-12) {
                const double inv = 1.0 / s;
                for (int a = 0; a < A; ++a)
                    st[static_cast<size_t>(a) * nh_ + h] = ss[static_cast<size_t>(a) * nl + i] * inv;
            } else {
                for (int a = 0; a < A; ++a) st[static_cast<size_t>(a) * nh_ + h] = u;
            }
        }
    }

    // ---- frecuencia de linea ---------------------------------------------
    //  Con que frecuencia se llega a cada linea EN TOTAL, sumando runouts.
    //
    //  No es la frecuencia que ensena el panel de estrategia. Esa es
    //  CONDICIONAL -- "cuando llego aqui, apuesto el 70%" -- y esta es
    //  ABSOLUTA -- "esta linea se juega en el 4% de las manos". Una linea de
    //  tres subidas puede ir al 70% en cada paso y ser practicamente
    //  inexistente en total: 0,7 * 0,7 * 0,7 ya es un tercio, y con los
    //  repartos de carta por medio se queda en nada. Es la diferencia entre
    //  "que hago aqui" y "cuanto importa esto", y la segunda es la que dice
    //  donde merece la pena mirar.
    //
    //  Cuesta un recorrido leyendo estrategias, sin evaluar ningun subarbol:
    //  nada que ver con query(), que evalua uno entero por accion.
    //
    //  Devuelve masa sin normalizar; la primera entrada es la raiz, asi que el
    //  porcentaje de cualquier linea es su masa partido la de la raiz.
    void line_freqs(std::vector<std::pair<std::string, double>>& out) {
        out.clear();
        if (T.ctx.empty()) return;
        std::map<std::string, double> acc;
        std::vector<double> a(static_cast<size_t>(nh_)), b(static_cast<size_t>(nh_));
        for (int h = 0; h < nh_; ++h) {
            a[static_cast<size_t>(h)] = range_[0][static_cast<size_t>(h)];
            b[static_cast<size_t>(h)] = range_[1][static_cast<size_t>(h)];
        }
        freq_walk(0, T.ctx[0].tree.root, 0, a.data(), b.data(), 1.0, "r:0", true, acc);
        out.assign(acc.begin(), acc.end());
    }

private:
    enum Mode { MODE_CFR = 0, MODE_EVAL, MODE_BR };

    // La masa conjunta de pares (mi mano, mano del rival) que llega a un nodo.
    // Es el mismo numero que NodeStats::wtot, por otro camino.
    double joint_mass(const double* r0, const double* r1) {
        std::vector<double> compat;
        compat_counts(D_, r1, nh_, compat);
        double m = 0.0;
        for (int h = 0; h < nh_; ++h)
            m += r0[static_cast<size_t>(h)] * compat[static_cast<size_t>(h)];
        return m;
    }

    // `cuenta` distingue el nodo que ESTRENA un nombre de linea de los que lo
    // repiten. Un reparto de carta no es un paso de la linea, asi que el nodo
    // de reparto, y la raiz de la calle siguiente, llevan el mismo nombre que
    // la accion que los trajo; si los tres sumaran, cada linea de turn contaria
    // tres veces.
    void freq_walk(int ci, int nid, long long inst, const double* r0, const double* r1,
                   double cw, const std::string& pre, bool cuenta,
                   std::map<std::string, double>& acc) {
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        if (cuenta) acc[pre] += cw * joint_mass(r0, r1);

        if (n.type == NT_CONT) {
            const int cc = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
            if (cc < 0) return;
            const int level = rc.street - T.start;
            const std::vector<Orbit>& orbits =
                (level == 0) ? D_.lvl1 : D_.lvl2[static_cast<size_t>(inst)];
            const long long base = (level == 0) ? 0 : D_.lvl2_base[static_cast<size_t>(inst)];
            // El mismo divisor que usa chance() para los valores: las cartas que
            // pueden salir son el mazo menos las cuatro de las dos manos, y esas
            // cuatro no dependen de cuales sean.
            const double invD = 1.0 / static_cast<double>(deckN_ - level - 4);
            std::vector<double> a(static_cast<size_t>(nh_)), b(static_cast<size_t>(nh_));
            for (size_t k = 0; k < orbits.size(); ++k) {
                const Orbit& o = orbits[k];
                const int c = D_.deck[static_cast<size_t>(o.rep_slot)];
                std::memcpy(a.data(), r0, sizeof(double) * static_cast<size_t>(nh_));
                std::memcpy(b.data(), r1, sizeof(double) * static_cast<size_t>(nh_));
                for (int h : D_.with_card[static_cast<size_t>(c)]) {
                    a[static_cast<size_t>(h)] = 0.0;
                    b[static_cast<size_t>(h)] = 0.0;
                }
                // Solo se resuelve el representante de cada orbita, pero la
                // orbita esta construida para que sus permutaciones conserven
                // los rangos: los demas miembros llevan la misma masa, asi que
                // se cuentan multiplicando en vez de recorriendolos.
                freq_walk(cc, T.ctx[static_cast<size_t>(cc)].tree.root,
                          base + static_cast<long long>(k), a.data(), b.data(),
                          cw * invD * static_cast<double>(o.member_slot.size()),
                          pre, false, acc);
            }
            return;
        }
        if (n.type != NT_DECISION) return;

        const int p = n.player;
        const int A = n.num_actions;
        std::vector<double> st(static_cast<size_t>(A) * nh_);
        avg_strategy_inst(ci, nid, inst, st.data());
        const double* mio = (p == 0) ? r0 : r1;
        std::vector<double> nr(static_cast<size_t>(nh_));
        for (int a = 0; a < A; ++a) {
            for (int h = 0; h < nh_; ++h)
                nr[static_cast<size_t>(h)] = mio[static_cast<size_t>(h)] *
                    st[static_cast<size_t>(a) * nh_ + h];
            const std::string paso = pre + ":" + line_step(rc, rc.tree.act(n, a));
            freq_walk(ci, rc.tree.child(n, a), inst,
                      (p == 0) ? nr.data() : r0, (p == 0) ? r1 : nr.data(),
                      cw, paso, true, acc);
        }
    }


    // El nodo cuyos valores por accion quiere br_at(); -1 = no capturar nada,
    // que es lo que vale para exploitability() y no le cuesta nada.
    int       br_ci_ = -1, br_nid_ = -1;
    long long br_inst_ = -1;
    int       br_actions_ = 0;
    std::vector<double> br_val_;

    GameTree&   T;
    const Deal& D_;
    int         nh_;
    int         deckN_;
    int         t_done = 0;
    int         nthreads_ = 1;
    double      t_trav_ = 0.0, t_disc_ = 0.0;
    // Reciprocal of the accumulated strategy discount, applied to the increment.
    double      strat_w_ = 1.0;

    std::vector<double> range_[2];
    std::vector<double> eq_[2];
    std::vector<double> start_compat_[2];

    std::vector<float> regret, strat_sum;

    // Deferred DCFR discount. `stamp_[b]` counts how many end-of-iteration
    // discounts block b has already absorbed; the traversal brings it up to
    // date on the spot, so the regrets are touched once per iteration instead
    // of twice. logpos_/logneg_ are prefix sums of the log factors, which is
    // what makes an arbitrary gap cheap to close -- although in practice every
    // block is read every iteration and the gap is always one.
    std::vector<int>           live_[2];      // combo indices each player actually holds
    std::vector<int>           live_union_;   // held by either -- see the opponent reach
    std::vector<int>           with_card_live_[2][52];   // live combos using each card
    std::vector<unsigned char> livemask_[2];
    int                        nlive_[2] = { 0, 0 };
    std::vector<SdEntry>       sd_pack_;      // strength order, live union only
    int                        sdn_ = 0;

    std::vector<int>    stamp_;
    std::vector<double> logpos_, logneg_;
    int                 stamp_target_ = 0;
    float               step_pos_ = 1.0f, step_neg_ = 1.0f;

    struct Frame {
        std::vector<double> ch, st, a, b, nrm;
        void init(int nh) {
            ch.assign(static_cast<size_t>(cfg::MAX_ACTIONS) * nh, 0.0);
            st.assign(static_cast<size_t>(cfg::MAX_ACTIONS) * nh, 0.0);
            a.assign(static_cast<size_t>(nh), 0.0);
            b.assign(static_cast<size_t>(nh), 0.0);
            nrm.assign(static_cast<size_t>(nh), 0.0);   // regret-matching denominators
        }
    };
    struct Scratch {
        std::vector<Frame> f;
        void init(int depth, int nh) {
            f.resize(static_cast<size_t>(depth));
            for (Frame& x : f) x.init(nh);
        }
    };
    Scratch                     main_scr_;
    std::vector<Scratch>        scr_;
    std::unique_ptr<ThreadPool> pool_;

    // ---- indexing --------------------------------------------------------
    //  A node's block holds one entry per action per LIVE combo of the player
    //  who acts there -- not per combo on the board. Inside a block, hands are
    //  addressed by their position in that player's live list, so the storage
    //  is dense and a whole action runs contiguously.
    long long block_index(const RoundCtx& rc, long long inst, const Node& n, int a) const {
        return rc.mem_offset + inst * rc.stride + n.block_offset +
               static_cast<long long>(a) * nlive_[n.player];
    }

    long long stamp_index(const RoundCtx& rc, long long inst, const Node& n) const {
        return rc.stamp_offset + inst * rc.tree.ndec + n.dec_slot;
    }

    // Brings one block's regrets up to the current iteration's discount level.
    void catch_up(const RoundCtx& rc, long long inst, const Node& n) {
        const long long si = stamp_index(rc, inst, n);
        const int last = stamp_[static_cast<size_t>(si)];
        if (last == stamp_target_) return;
        float fp, fn;
        if (last == stamp_target_ - 1) {          // the usual case
            fp = step_pos_;
            fn = step_neg_;
        } else {
            fp = static_cast<float>(std::exp(logpos_[static_cast<size_t>(stamp_target_)] -
                                             logpos_[static_cast<size_t>(last)]));
            fn = static_cast<float>(std::exp(logneg_[static_cast<size_t>(stamp_target_)] -
                                             logneg_[static_cast<size_t>(last)]));
        }
        float* rg = &regret[static_cast<size_t>(block_index(rc, inst, n, 0))];
        const long long cnt = static_cast<long long>(n.num_actions) * nlive_[n.player];
        for (long long i = 0; i < cnt; ++i)
            rg[static_cast<size_t>(i)] *= (rg[static_cast<size_t>(i)] > 0.0f) ? fp : fn;
        stamp_[static_cast<size_t>(si)] = stamp_target_;
    }

    int runout_of(long long inst) const {
        return D_.runout_by_inst[static_cast<size_t>(inst)];
    }

    // Aqui NO se sustituye por el lock, y no es un olvido: esto promedia entre
    // runouts, y con un lock por runout "esta bloqueada" no tiene una sola
    // respuesta. No hace falta: strat_sum ya acumula la estrategia bloqueada
    // igual que cualquier otra -- lo unico que se salta un nodo bloqueado es la
    // actualizacion del regret -- asi que el promedio ya sale bien. Una mano
    // bloqueada que no llega nunca sale uniforme, como cualquier otra que no
    // llega.
    void normalise_block(const Node& n, const double* acc, double* st, int A) const {
        (void)n;
        for (int h = 0; h < nh_; ++h) {
            double s = 0.0;
            for (int a = 0; a < A; ++a) s += acc[static_cast<size_t>(a) * nh_ + h];
            if (s > 1e-12) {
                const double inv = 1.0 / s;
                for (int a = 0; a < A; ++a)
                    st[static_cast<size_t>(a) * nh_ + h] = acc[static_cast<size_t>(a) * nh_ + h] * inv;
            } else {
                const double u = 1.0 / static_cast<double>(A);
                for (int a = 0; a < A; ++a) st[static_cast<size_t>(a) * nh_ + h] = u;
            }
        }
    }

    // Regret matching+ straight out of the stored regrets.
    //
    // Training brings the block up to date first; the read-only passes do not
    // need to. Regret matching clamps negatives to zero and then normalises, so
    // a pending discount -- one factor for the whole block on the positives,
    // and anything at all on entries that were about to be clamped away -- comes
    // straight back out of the division and cannot change the strategy.
    void current_strategy(const RoundCtx& rc, long long inst, const Node& n, double* st,
                          bool training, double* sum) {
        if (training) catch_up(rc, inst, n);
        const int A = n.num_actions;
        const float* rg = &regret[static_cast<size_t>(block_index(rc, inst, n, 0))];

        // One pass per hand, not one per action: with two or three actions the
        // whole inner loop stays in registers, and an action-major rewrite --
        // which vectorises far better on paper -- measured 6% SLOWER because it
        // walks the strategy block three times instead of once. Memory traffic
        // wins again. What is worth hoisting is the lock test, which is
        // loop-invariant and used to cost a branch on every hand.
        (void)sum;
        const double u = 1.0 / static_cast<double>(A);
        // The strategy at this node belongs to whoever acts here, so it is that
        // player's live combos that have one. `i` walks the block densely, `h`
        // is the combo it stands for -- `st` stays full width because the rest
        // of the traversal indexes it by combo.
        const std::vector<int>& LN = live_[n.player];
        const int nl = nlive_[n.player];
        // Un puntero en vez del bool de antes: se resuelve aqui, fuera del
        // bucle, asi que el interior sigue costando lo mismo.
        const Node::LockBlock* lb = n.lock_for(inst);
        for (int i = 0; i < nl; ++i) {
            const int h = LN[static_cast<size_t>(i)];
            if (lb && lb->hand[static_cast<size_t>(h)]) {
                for (int a = 0; a < A; ++a)
                    st[static_cast<size_t>(a) * nh_ + h] =
                        lb->strategy[static_cast<size_t>(a) * nh_ + h];
                continue;
            }
            double s = 0.0;
            for (int a = 0; a < A; ++a) {
                const double r = rg[static_cast<size_t>(a) * nl + i];
                const double v = (r > 0.0) ? r : 0.0;
                st[static_cast<size_t>(a) * nh_ + h] = v;
                s += v;
            }
            if (s > 1e-12) {
                const double inv = 1.0 / s;
                for (int a = 0; a < A; ++a) st[static_cast<size_t>(a) * nh_ + h] *= inv;
            } else {
                for (int a = 0; a < A; ++a) st[static_cast<size_t>(a) * nh_ + h] = u;
            }
        }
    }

    // ---- terminals -------------------------------------------------------
    //  Without rake the winner nets +W and the loser -W, and a chop is nothing
    //  for either. With rake R the winner nets W-R, the loser still -W, and a
    //  chop costs each of them R/2. Folding that together:
    //
    //      out = W*(win - lose) - R*(win + tie/2)
    //          = (W - R/2)*(win - lose) - (R/2)*compat
    //
    //  since win + tie + lose = compat. The compat pass is only paid for when
    //  rake is actually on -- this is the hottest loop in the solver.
    void showdown_cfv(int runout, const double* ro, double W, double R, double* out,
                      double* win, double* lose, int trav, int opp) const {
        showdown_split(&sd_pack_[static_cast<size_t>(runout) * sdn_],
                       sdn_, ro, win, lose, livemask_[trav].data());
        if (R <= 0.0) {
            for (int i : live_[trav]) out[i] = W * (win[i] - lose[i]);
            return;
        }
        double card[52];
        double tot = 0.0;
        compat_sums(ro, opp, card, tot);
        const double half = 0.5 * R;
        for (int i : live_[trav]) {
            const Combo& me = D_.combos[static_cast<size_t>(i)];
            const double compat = tot - card[me.c1] - card[me.c2] + ro[i];
            out[i] = (W - half) * (win[i] - lose[i]) - half * compat;
        }
    }

    // Per-card running sums of the opponent's reach, which is what turns the
    // blocker correction into O(1) per hand.
    void compat_sums(const double* ro, int opp, double* card, double& tot) const {
        for (int c = 0; c < 52; ++c) card[c] = 0.0;
        tot = 0.0;
        for (int i : live_[opp]) {
            const double w = ro[i];
            if (w == 0.0) continue;
            tot += w;
            card[D_.combos[static_cast<size_t>(i)].c1] += w;
            card[D_.combos[static_cast<size_t>(i)].c2] += w;
        }
    }

    void fold_cfv(const double* ro, double sW, double* out, int trav, int opp) const {
        double card[52];
        double tot = 0.0;
        compat_sums(ro, opp, card, tot);
        for (int i : live_[trav]) {
            const Combo& me = D_.combos[static_cast<size_t>(i)];
            out[i] = sW * (tot - card[me.c1] - card[me.c2] + ro[i]);
        }
    }

    // ---- the traversal ---------------------------------------------------
    void cfr(int ci, long long inst, int nid, const double* rs, const double* ro,
             int trav, Scratch& S, int depth, double* out,
             bool may_par, int mode = MODE_CFR) {
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const BetTree&  bt = rc.tree;
        const Node&     n  = bt.nodes[static_cast<size_t>(nid)];

        if (n.type == NT_SHOWDOWN) {
            Frame& FT = S.f[static_cast<size_t>(depth)];
            showdown_cfv(runout_of(inst), ro, n.terminal_W, n.rake, out,
                         FT.a.data(), FT.b.data(), trav, 1 - trav);
            return;
        }
        if (n.type == NT_FOLD) {
            // The folder loses what they put in whatever the house takes; the
            // winner is the one who pays the rake out of the pot.
            const double sW = (n.player == trav) ? -n.terminal_W
                                                 : (n.terminal_W - n.rake);
            fold_cfv(ro, sW, out, trav, 1 - trav);
            return;
        }
        if (n.type == NT_CONT) { chance(ci, inst, n, rs, ro, trav, S, depth, out, may_par, mode); return; }

        const int A = n.num_actions;
        Frame& F = S.f[static_cast<size_t>(depth)];
        double* ch = F.ch.data();
        double* st = F.st.data();
        double* nr = F.a.data();

        // A best response is a response to the strategy the solver REPORTS,
        // which is the average -- not to the current regret-matching iterate.
        // In CFR the iterate does not converge; it circles the equilibrium for
        // ever, and only the average settles. Measuring against the iterate
        // made exploitability a number about a strategy nobody is ever shown:
        // on the polarised toy, whose average strategy sits on the algebra to
        // six decimals, it read 0.17 after two hundred thousand iterations and
        // was still wandering. Against the average it reads 0.00003.
        if (mode == MODE_CFR) current_strategy(rc, inst, n, st, true, F.nrm.data());
        else                  avg_strategy_inst(ci, nid, inst, st);

        const std::vector<int>& LT = live_[trav];

        if (n.player == trav) {
            for (int a = 0; a < A; ++a) {
                const double* sa = st + static_cast<size_t>(a) * nh_;
                // Only the live set, unlike the opponent's reach below. The
                // traverser's reach is read back in exactly three places -- the
                // strategy sum, this same loop one level down, and the memcpy at
                // a chance node -- and all three touch live indices only. On a
                // flop that is 204 combos of 1176, so five sixths of these
                // multiplies were going into entries nobody would ever read.
                for (int h : LT) nr[h] = rs[h] * sa[h];
                cfr(ci, inst, bt.child(n, a), nr, ro, trav, S, depth + 1,
                    ch + static_cast<size_t>(a) * nh_, may_par, mode);
            }
            if (mode == MODE_BR) {
                // Si este es el nodo que se ha pedido, se guardan los valores
                // de cada accion ANTES de quedarse con el maximo. El maximo es
                // lo que vale la mejor respuesta; lo que se guarda aqui es de
                // donde sale, que es lo que hay que ensenar.
                if (ci == br_ci_ && nid == br_nid_ && inst == br_inst_) {
                    br_val_.assign(static_cast<size_t>(A) * nh_, 0.0);
                    for (int a = 0; a < A; ++a)
                        for (int h = 0; h < nh_; ++h)
                            br_val_[static_cast<size_t>(a) * nh_ + h] =
                                ch[static_cast<size_t>(a) * nh_ + h];
                    br_actions_ = A;
                }
                const Node::LockBlock* lbr = n.lock_for(inst);
                for (int h : LT) {
                    const bool frozen = lbr && lbr->hand[static_cast<size_t>(h)];
                    if (frozen) {
                        double s = 0.0;
                        for (int a = 0; a < A; ++a)
                            s += st[static_cast<size_t>(a) * nh_ + h] * ch[static_cast<size_t>(a) * nh_ + h];
                        out[h] = s;
                    } else {
                        double best = ch[h];
                        for (int a = 1; a < A; ++a) {
                            const double v = ch[static_cast<size_t>(a) * nh_ + h];
                            if (v > best) best = v;
                        }
                        out[h] = best;
                    }
                }
                return;
            }
            for (int h : LT) out[h] = 0.0;
            for (int a = 0; a < A; ++a) {
                const double* sa = st + static_cast<size_t>(a) * nh_;
                const double* cv = ch + static_cast<size_t>(a) * nh_;
                for (int h : LT) out[h] += sa[h] * cv[h];
            }
            if (mode == MODE_CFR) {
                // The lock test used to sit inside this loop, where it stopped
                // the whole thing from vectorising for the 99.9% of nodes that
                // are not locked. It is loop-invariant, so it is hoisted.
                const Node::LockBlock* lc = n.lock_for(inst);
                const double sw = strat_w_;
                const int nl = nlive_[trav];          // this node is the traverser's
                for (int a = 0; a < A; ++a) {
                    float* rg = &regret[static_cast<size_t>(block_index(rc, inst, n, a))];
                    float* ss = &strat_sum[static_cast<size_t>(block_index(rc, inst, n, a))];
                    const double* sa = st + static_cast<size_t>(a) * nh_;
                    const double* cv = ch + static_cast<size_t>(a) * nh_;
                    if (!lc) {
                        for (int i = 0; i < nl; ++i) {
                            const int h = LT[static_cast<size_t>(i)];
                            rg[i] = static_cast<float>(rg[i] + (cv[h] - out[h]));
                        }
                    } else {
                        // Locked combos are frozen: no regret, but their fixed
                        // strategy still propagates values through the tree.
                        for (int i = 0; i < nl; ++i) {
                            const int h = LT[static_cast<size_t>(i)];
                            if (!lc->hand[static_cast<size_t>(h)])
                                rg[i] = static_cast<float>(rg[i] + (cv[h] - out[h]));
                        }
                    }
                    for (int i = 0; i < nl; ++i) {
                        const int h = LT[static_cast<size_t>(i)];
                        ss[i] = static_cast<float>(ss[i] + sw * rs[h] * sa[h]);
                    }
                }
            }
        } else {
            for (int h : LT) out[h] = 0.0;
            for (int a = 0; a < A; ++a) {
                const double* sa = st + static_cast<size_t>(a) * nh_;
                // The union of the two ranges, not the live set of either.
                //
                // Restricting this was tried twice before and moved a 20-chip
                // pot to 20.25, and the note left behind said the reason was
                // not understood. It is this. The opponent's reach is not read
                // only at the opponent's combos: `fold_cfv` and the rake branch
                // of `showdown_cfv` read `ro[i]` at the TRAVERSER's combos, for
                // the self-blocking term. So the set that has to be written is
                // both players' combos at once.
                //
                // And it has to be a set that does not depend on who is
                // traversing, because these frames are reused across
                // traversals. Written over `live_[opp]`, a frame that held
                // player 0's reach keeps its old numbers at the combos only
                // player 0 holds, and the next traversal reads them as player
                // 1's. Written over the union, every entry is rewritten every
                // time and there is nothing stale to read. Entries outside the
                // union are read by nobody: the showdown table is packed over
                // the union too.
                //
                // On a flop that is the difference between 1176 multiplies and
                // about 300.
                for (int h : live_union_) nr[h] = ro[h] * sa[h];
                double* cv = ch + static_cast<size_t>(a) * nh_;
                cfr(ci, inst, bt.child(n, a), rs, nr, trav, S, depth + 1, cv, may_par, mode);
                for (int h : LT) out[h] += cv[h];
            }
        }
    }

    // ---- chance node -----------------------------------------------------
    //  Only one card per orbit is solved. Every other member of the orbit is the
    //  same subtree seen through a suit permutation, so its contribution is read
    //  back from the representative's values with the combo indices permuted.
    void chance(int ci, long long inst, const Node& n, const double* rs, const double* ro,
                int trav, Scratch& S, int depth, double* out, bool may_par, int mode) {
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const int child_ctx = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
        const int level = rc.street - T.start;

        const std::vector<Orbit>& orbits =
            (level == 0) ? D_.lvl1 : D_.lvl2[static_cast<size_t>(inst)];
        const long long base = (level == 0) ? 0 : D_.lvl2_base[static_cast<size_t>(inst)];

        const int nk = static_cast<int>(orbits.size());
        // How many cards the sum is averaged over, and it is NOT the number the
        // hero can see. Write the sum out and swap the order:
        //
        //   v(h) = (1/N) * sum over c not in h of sum over h' of pi(h') u(h,h',c)
        //        = (1/N) * sum over h' of pi(h') * sum over c not in h or h'
        //        = ((D-2)/N) * sum over h' of pi(h') E_c[u | h, h']
        //
        // because every villain hand is two cards, neither on the board and
        // neither in h, so it blocks exactly two of the D = deck - level - 2
        // cards the hero could see. Dividing by D therefore lands (D-2)/D short
        // of the value; the right divisor is D-2.
        //
        // The old divisor was D, and the error compounds per chance street: a
        // turn came out at 44/46 of the truth and a flop at (45/47)(44/46).
        // Nothing here could catch it -- every closed-form toy in the checks is
        // on a river, where there is no chance node -- and it only surfaced
        // against another solver on a spot with no betting at all, where the answer
        // is pure showdown and can be worked out by hand.
        const double invD = 1.0 / static_cast<double>(deckN_ - level - 4);

        // Only the traverser's combos, here and in accumulate_orbit below.
        // `out` is this node's value for the traverser and is read at exactly
        // those; the rest were 972 zeroes of 1176 written for nobody.
        const std::vector<int>& LT = live_[trav];
        for (int h : LT) out[h] = 0.0;

        const bool go_par = may_par && nthreads_ > 1 && nk > 1;
        if (go_par) {
            // Sibling subtrees write to different runout instances, so their
            // memory never overlaps and no locking is needed.
            std::vector<std::vector<double>> res(static_cast<size_t>(nk),
                std::vector<double>(static_cast<size_t>(nh_), 0.0));
            std::vector<std::vector<double>> lrs(static_cast<size_t>(nthreads_),
                std::vector<double>(static_cast<size_t>(nh_)));
            std::vector<std::vector<double>> lro(static_cast<size_t>(nthreads_),
                std::vector<double>(static_cast<size_t>(nh_)));
            pool_->run(nk, [&](int j, int tid) {
                // Costliest orbit first (see Deal::lvl1_order).
                const int k = (level == 0 && static_cast<int>(D_.lvl1_order.size()) == nk)
                            ? D_.lvl1_order[static_cast<size_t>(j)] : j;
                const int c = D_.deck[static_cast<size_t>(orbits[static_cast<size_t>(k)].rep_slot)];
                double* a = lrs[static_cast<size_t>(tid)].data();
                double* b = lro[static_cast<size_t>(tid)].data();
                std::memcpy(a, rs, sizeof(double) * static_cast<size_t>(nh_));
                std::memcpy(b, ro, sizeof(double) * static_cast<size_t>(nh_));
                for (int h : D_.with_card[static_cast<size_t>(c)]) { a[h] = 0.0; b[h] = 0.0; }
                cfr(child_ctx, base + k, T.ctx[static_cast<size_t>(child_ctx)].tree.root,
                    a, b, trav, scr_[static_cast<size_t>(tid)], 0,
                    res[static_cast<size_t>(k)].data(), false, mode);
            });
            for (int k = 0; k < nk; ++k)
                accumulate_orbit(orbits[static_cast<size_t>(k)],
                                 res[static_cast<size_t>(k)].data(), out, trav);
        } else {
            Frame& F = S.f[static_cast<size_t>(depth)];
            double* a  = F.a.data();
            double* b  = F.b.data();
            double* cv = F.ch.data();
            for (int k = 0; k < nk; ++k) {
                const Orbit& o = orbits[static_cast<size_t>(k)];
                const int c = D_.deck[static_cast<size_t>(o.rep_slot)];
                std::memcpy(a, rs, sizeof(double) * static_cast<size_t>(nh_));
                std::memcpy(b, ro, sizeof(double) * static_cast<size_t>(nh_));
                for (int h : D_.with_card[static_cast<size_t>(c)]) { a[h] = 0.0; b[h] = 0.0; }
                cfr(child_ctx, base + k, T.ctx[static_cast<size_t>(child_ctx)].tree.root,
                    a, b, trav, S, depth + 1, cv, false, mode);
                accumulate_orbit(o, cv, out, trav);
            }
        }
        for (int h : LT) out[h] *= invD;
    }

    // Adds one orbit's whole contribution. For member card c reached by suit
    // permutation p, the value of hand h is the representative's value of the
    // hand p maps onto h -- and hands containing c drop out.
    // Restricted to the traverser's combos, which needs one thing to be true:
    // that `back` maps a live combo to a live combo. It does, but only since
    // the collapsing group was made to preserve the ranges -- a permutation in
    // the group maps {h : range[h] > 0} onto itself by definition, so a live
    // hand's image is live. Before that fix the second level could collapse by
    // a permutation the ranges had never survived, and this loop would have
    // read `v` at a combo outside the range. Which is to say: this is 5.8x less
    // work that only became legal this morning.
    void accumulate_orbit(const Orbit& o, const double* v, double* out, int trav) const {
        const std::vector<int>& LT = live_[trav];
        for (size_t m = 0; m < o.member_slot.size(); ++m) {
            const int card = D_.deck[static_cast<size_t>(o.member_slot[m])];
            const int p    = o.member_perm[m];
            const std::vector<int>& gone = with_card_live_[trav][static_cast<size_t>(card)];
            if (p == D_.identity()) {
                for (int h : LT) out[h] += v[h];
                for (int h : gone) out[h] -= v[h];
            } else {
                const std::vector<int>& back = D_.perm_combo[static_cast<size_t>(D_.perm_inv[p])];
                for (int h : LT) out[h] += v[back[static_cast<size_t>(h)]];
                for (int h : gone) out[h] -= v[back[static_cast<size_t>(h)]];
            }
        }
    }

    // ---- DCFR discounting ------------------------------------------------
    // The strategy sum never gets swept. Its discount is a single factor shared
    // by every entry, so instead of scaling the whole buffer down each
    // iteration the increment is scaled up by the reciprocal -- and since the
    // average strategy normalises across actions, the common factor cancels and
    // never has to be undone. That halves the memory traffic of this pass,
    // which on a flop is the difference between touching 0.9 GB and 0.4 GB
    // every single iteration.
    //
    // Regrets do not get swept either. Positive and negative entries take
    // different factors, so no single scalar can stand in for the pair -- but
    // the sign of an entry cannot change while nobody is writing to it, so the
    // right factor is still known when the block is next read. All this has to
    // do is record what the traversal will owe.
    void discount(int t) {
        const double td = static_cast<double>(t);
        double dpos, dneg;
        discount_factors(t, dpos, dneg);
        const double dstr = std::pow(td / (td + 1.0), cfg::DCFR_GAMMA);

        logpos_.push_back(logpos_.back() + std::log(dpos > 1e-300 ? dpos : 1e-300));
        logneg_.push_back(logneg_.back() + std::log(dneg > 1e-300 ? dneg : 1e-300));
        if (dstr > 1e-300) strat_w_ /= dstr;
    }

    // Called before each traversal: the regrets it reads must already carry
    // every discount up to and including the previous iteration's.
    void set_discount_target(int t) {
        stamp_target_ = t - 1;
        if (stamp_target_ < 0) stamp_target_ = 0;
        // Recomputed from the formula rather than from the log prefix sums, so
        // the one-step case -- which is every block, every iteration -- lands on
        // exactly the float the old sweep would have produced.
        if (stamp_target_ > 0) {
            double dp, dn;
            discount_factors(stamp_target_, dp, dn);
            step_pos_ = static_cast<float>(dp);
            step_neg_ = static_cast<float>(dn);
        } else {
            step_pos_ = step_neg_ = 1.0f;
        }
    }

    // ---- reach vectors down to one node ----------------------------------
    bool reach_at(int ci, int nid, long long inst, int player,
                  std::vector<double>& rs, std::vector<double>& ro) {
        // The chain of contexts from the root down to `ci`.
        std::vector<int> chain;
        for (int c = ci; c >= 0; c = T.ctx[static_cast<size_t>(c)].parent) chain.push_back(c);
        std::reverse(chain.begin(), chain.end());

        rs.assign(static_cast<size_t>(nh_), 0.0);
        ro.assign(static_cast<size_t>(nh_), 0.0);
        for (int h = 0; h < nh_; ++h) {
            rs[static_cast<size_t>(h)] = range_[player][static_cast<size_t>(h)];
            ro[static_cast<size_t>(h)] = range_[1 - player][static_cast<size_t>(h)];
        }

        // Which instance each context on the way down lives in, and which card
        // was dealt to get there. Both come from the deal rather than from
        // arithmetic on the index, because with suits collapsed the index counts
        // orbits and no arithmetic on it recovers a card.
        std::vector<long long> chain_inst;
        std::vector<int>       chain_slot;
        if (!D_.decompose(inst, static_cast<int>(chain.size()) - 1,
                          chain_inst, chain_slot)) return false;

        for (size_t ck = 0; ck < chain.size(); ++ck) {
            const int c = chain[ck];
            const RoundCtx& rc = T.ctx[static_cast<size_t>(c)];
            const BetTree&  bt = rc.tree;

            // Which node of this round do we have to reach?
            int target;
            if (ck + 1 == chain.size()) {
                target = nid;
            } else {
                const int want_cont = T.ctx[static_cast<size_t>(chain[ck + 1])].parent_cont;
                target = -1;
                for (size_t i = 0; i < bt.nodes.size(); ++i)
                    if (bt.nodes[i].type == NT_CONT && bt.nodes[i].cont_id == want_cont) {
                        target = static_cast<int>(i);
                        break;
                    }
                if (target < 0) return false;
            }
            if (ck >= chain_inst.size()) return false;
            if (!walk_round(c, chain_inst[ck], target, player, rs, ro)) return false;

            if (ck + 1 < chain.size()) {
                // Deal the runout card that leads to the next context: every
                // combo using it is impossible from here down.
                if (ck >= chain_slot.size()) return false;
                const int card = D_.deck[static_cast<size_t>(chain_slot[ck])];
                for (int h : D_.with_card[static_cast<size_t>(card)]) {
                    rs[static_cast<size_t>(h)] = 0.0;
                    ro[static_cast<size_t>(h)] = 0.0;
                }
            }
        }
        return true;
    }

    // Multiplies the reaches by the average strategy along the unique path from
    // the round's root to `target`.
    bool walk_round(int ci, long long inst, int target, int player,
                    std::vector<double>& rs, std::vector<double>& ro) const {
        const BetTree& bt = T.ctx[static_cast<size_t>(ci)].tree;
        std::vector<int> path;
        if (!find_path(bt, bt.root, target, path)) return false;
        std::vector<double> st;
        for (size_t k = 0; k + 1 < path.size(); ++k) {
            const Node& n = bt.nodes[static_cast<size_t>(path[k])];
            if (n.type != NT_DECISION) return false;
            int a = -1;
            for (int i = 0; i < n.num_actions; ++i)
                if (bt.child(n, i) == path[k + 1]) { a = i; break; }
            if (a < 0) return false;
            st.assign(static_cast<size_t>(n.num_actions) * nh_, 0.0);
            avg_strategy_inst(ci, path[k], inst, st.data());
            std::vector<double>& tgt = (n.player == player) ? rs : ro;
            for (int h = 0; h < nh_; ++h)
                tgt[static_cast<size_t>(h)] *= st[static_cast<size_t>(a) * nh_ + h];
        }
        return true;
    }

    static bool find_path(const BetTree& bt, int from, int to, std::vector<int>& path) {
        path.push_back(from);
        if (from == to) return true;
        const Node& n = bt.nodes[static_cast<size_t>(from)];
        if (n.type == NT_DECISION)
            for (int a = 0; a < n.num_actions; ++a)
                if (find_path(bt, bt.child(n, a), to, path)) return true;
        path.pop_back();
        return false;
    }
};

// La explotabilidad COMO SE ENSENA, que es la convencion de la referencia: la MEDIA
// de las dos mejores respuestas y no la suma.
//
// Las dos convenciones convivian sin decirlo, y se veia en la pantalla: el
// objetivo de precision se comparaba contra la media mientras que el numero que
// salia al lado era la suma. Pedias el 1% del bote, paraba diciendo que lo habia
// alcanzado, y te ensenaba 1,46% -- exactamente el doble -- con un texto debajo
// prometiendo que nunca te daria peor de lo que pides.
//
// Ahora pasa por aqui todo lo que el usuario lee: la web, la consola y el
// objetivo de precision. Y de paso el numero queda comparable con el de la referencia sin
// traducirlo: su "Exploitable for: 0.022" y el nuestro miden lo mismo.
inline double expl_shown(double suma) { return suma < 0.0 ? suma : 0.5 * suma; }
