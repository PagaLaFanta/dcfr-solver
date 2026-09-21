// =============================================================================
//  DCFR RIVER SOLVER  --  Discounted Counterfactual Regret Minimization
// -----------------------------------------------------------------------------
//  Single translation unit. C++17. STL only. No external dependencies.
//
//  Features
//    * Flat (pointer-free) Structure-of-Arrays memory for every information set.
//      All regrets / strategy sums / average strategies live in one contiguous
//      std::vector<double>; nodes hold integer offsets, children are indices.
//      Layout inside a node block is ACTION-MAJOR:  idx = mem_offset + a*NH + h
//      so the innermost CFR loop walks memory linearly over hands.
//    * Discounted CFR (Brown & Sandholm 2019) with alpha/beta/gamma discounting.
//    * Vector-form (range vs range) traversal: one pass returns the whole
//      counterfactual-value vector for every hand at once.
//    * Exact card-removal / blocker handling at every terminal node.
//    * NODELOCKING: per-node, per-hand fixed strategies (is_locked +
//      locked_strategy). Locked hands never accumulate regret; the rest of the
//      tree best-responds around them (exploitative solving).
//    * Per-hand, per-action counterfactual EV extraction (solver-grade EV).
//    * Average-strategy (frequency) extraction.
//    * Hand-type bucketing (NUTS / VALUE / BLUFF-CATCHER / AIR) with aggregated
//      frequencies and EVs.
//    * Constrained best-response / exploitability metric (respects locks).
//
//  Toy game: synthetic River subgame.
//    - 13 distinct cards (ranks 2..A), each player is dealt exactly one.
//    - Both players cannot hold the same card -> real blocker effects.
//    - Trivial evaluator: higher rank wins the pot, no ties possible.
//    - Betting: OOP acts first, 75% pot bets, one raise allowed per line.
//
//  Utility convention (zero-sum internally, solver-style for display):
//      u_p = (share of final pot) - POT0/2 - (chips invested in the subgame)
//      u_0 + u_1 == 0 always.
//      Displayed EV = u_p + POT0/2  ==  "chips taken out of the pot, net of the
//      chips put in during this subgame".  A hand that always wins a checked-down
//      pot of 20 shows EV = 20.00; a hand that always loses shows EV = 0.00.
//      Invariant: EV_display(OOP) + EV_display(IP) == POT0.
// =============================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// =============================================================================
//  1. CONFIGURATION
// =============================================================================
namespace cfg {
// ---- compile time: these size the flat arrays, so they cannot move ---------
constexpr int NUM_HANDS   = 13;   // one card per player, ranks 2..A
constexpr int MAX_ACTIONS = 6;    // upper bound on branching factor

// ---- runtime knobs, driven by the interactive console ---------------------
inline double POT0       = 20.0;   // dead money on the river
inline double STACK      = 100.0;  // effective stack behind (per player)
inline int    MAX_RAISES = 2;      // a bet counts as raise #1

// Discounted CFR hyper-parameters (defaults recommended in the DCFR paper).
inline double DCFR_ALPHA = 1.5;    // discount on POSITIVE cumulative regret
inline double DCFR_BETA  = 0.0;    // discount on NEGATIVE cumulative regret
inline double DCFR_GAMMA = 2.0;    // discount on the average strategy

// Hand-type bucket thresholds: lowest card index belonging to each bucket.
inline int TH_NUTS  = 11;  // K, A
inline int TH_VALUE = 8;   // T, J, Q
inline int TH_BC    = 4;   // 6, 7, 8, 9  (everything below is AIR)
} // namespace cfg

static constexpr int NH = cfg::NUM_HANDS;

using HandVec = std::array<double, cfg::NUM_HANDS>;

// =============================================================================
//  2. CARDS, TRIVIAL EVALUATOR, HAND-TYPE BUCKETING
// =============================================================================
static const char* const RANK_NAME[cfg::NUM_HANDS] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"
};

// The evaluator: hand strength IS the card index. Higher index wins.
// (Kept deliberately trivial so the structural machinery above it is complete.)
inline int hand_strength(int hand) { return hand; }

enum HandType {
    HT_AIR = 0,
    HT_BLUFFCATCHER,
    HT_VALUE,
    HT_NUTS,
    HT_COUNT
};

static const char* const HT_NAME[HT_COUNT] = {
    "AIR", "BLUFF-CATCHER", "VALUE", "NUTS"
};

// Classifier: maps a holding to a strategic bucket. Thresholds are runtime
// settable from the console ("set buckets 11,8,4").
inline HandType classify(int hand) {
    if (hand >= cfg::TH_NUTS)  return HT_NUTS;
    if (hand >= cfg::TH_VALUE) return HT_VALUE;
    if (hand >= cfg::TH_BC)    return HT_BLUFFCATCHER;
    return HT_AIR;
}

// =============================================================================
//  3. GAME TREE  (flat arrays, integer indices, zero pointers)
// =============================================================================
enum NodeType { NT_DECISION = 0, NT_SHOWDOWN, NT_FOLD };
enum ActionKind { AK_FOLD = 0, AK_CHECK, AK_CALL, AK_BET, AK_RAISE };

struct ActionInfo {
    ActionKind  kind;
    double      to_amount;   // total chips invested by the actor AFTER this action
    std::string code;        // compact code used in the node path ("X", "B15", ...)
    std::string label;       // human readable ("Check", "Bet 15", "Raise to 45")
};

struct Node {
    NodeType type          = NT_DECISION;
    int      player        = -1;   // decision: actor. NT_FOLD: the player folding.
    int      num_actions   = 0;
    int      action_offset = -1;   // -> GameTree::actions
    int      child_offset  = -1;   // -> GameTree::children
    int      mem_offset    = -1;   // -> flat regret / strategy buffers
    double   terminal_W    = 0.0;  // magnitude of the terminal payoff
    double   pot           = 0.0;  // pot size at this node (display only)
    std::string path;              // e.g. "R/B15/R45"

    // ---- NODELOCKING -------------------------------------------------------
    bool                 is_locked = false;  // true if ANY hand is locked here
    std::vector<uint8_t> locked_hand;        // size NH, 1 = this hand is frozen
    std::vector<double>  locked_strategy;    // action-major: [a*NH + h]
};

struct GameTree {
    std::vector<Node>       nodes;
    std::vector<int>        children;  // flat child-index table
    std::vector<ActionInfo> actions;   // flat action table
    int root     = 0;
    int mem_size = 0;                  // total doubles per strategy buffer

    const ActionInfo& act(const Node& n, int a) const {
        return actions[n.action_offset + a];
    }
    int child(const Node& n, int a) const {
        return children[n.child_offset + a];
    }
    int action_index(const Node& n, ActionKind k) const {
        for (int a = 0; a < n.num_actions; ++a)
            if (act(n, a).kind == k) return a;
        return -1;
    }
    int child_by_kind(int nid, ActionKind k) const {
        const Node& n = nodes[nid];
        int a = action_index(n, k);
        return (a < 0) ? -1 : child(n, a);
    }
    // Node ids are rebuilt from scratch whenever the tree geometry changes, so
    // anything that has to survive a rebuild (locks, the console cwd) is keyed
    // by path instead.
    int find_path(const std::string& p) const {
        for (size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i].path == p) return static_cast<int>(i);
        return -1;
    }
    static std::string parent_path(const std::string& p) {
        const size_t s = p.find_last_of('/');
        return (s == std::string::npos) ? p : p.substr(0, s);
    }
};

// =============================================================================
//  4. TREE BUILDER
// =============================================================================
struct TreeConfig {
    std::vector<double> bet_fracs   = { 0.75 };  // fraction of the current pot
    std::vector<double> raise_fracs = { 1.00 };  // fraction of the pot after call
    int                 max_raises  = cfg::MAX_RAISES;
};

class TreeBuilder {
public:
    explicit TreeBuilder(const TreeConfig& c) : c_(c) {}

    GameTree build() {
        t_ = GameTree();
        t_.root = rec(/*player*/ 0, /*inv0*/ 0.0, /*inv1*/ 0.0,
                      /*raises*/ 0, /*facing*/ false, /*prev_check*/ false, "R");
        // Assign the flat information-set memory offsets in a single sweep.
        int off = 0;
        for (Node& n : t_.nodes) {
            if (n.type != NT_DECISION) continue;
            n.mem_offset = off;
            off += n.num_actions * NH;
        }
        t_.mem_size = off;
        return t_;
    }

private:
    TreeConfig c_;
    GameTree   t_;

    static std::string num(double v) {
        char b[32];
        std::snprintf(b, sizeof(b), "%g", v);
        return std::string(b);
    }

    int add_terminal(NodeType ty, int folder, double W, double pot,
                     const std::string& path) {
        Node n;
        n.type       = ty;
        n.player     = folder;   // -1 for showdown
        n.terminal_W = W;
        n.pot        = pot;
        n.path       = path;
        int id = static_cast<int>(t_.nodes.size());
        t_.nodes.push_back(n);
        return id;
    }

    struct Succ {
        bool     terminal;
        NodeType ty;
        int      folder;
        double   W;
        double   ni0, ni1;
        int      nraises;
        bool     nfacing;
        bool     nprev;
    };

    int rec(int player, double inv0, double inv1, int raises,
            bool facing, bool prev_check, const std::string& path) {
        const double inv[2]  = { inv0, inv1 };
        const int    opp     = 1 - player;
        const double to_call = inv[opp] - inv[player];
        const double pot     = cfg::POT0 + inv0 + inv1;

        std::vector<ActionInfo> acts;
        std::vector<Succ>       succ;

        if (facing && to_call > 1e-9) {
            // ---- FOLD ------------------------------------------------------
            acts.push_back(ActionInfo{ AK_FOLD, inv[player], "F", "Fold" });
            succ.push_back(Succ{ true, NT_FOLD, player,
                                 cfg::POT0 * 0.5 + inv[player],
                                 inv0, inv1, 0, false, false });

            // ---- CALL (closes the action -> showdown) ----------------------
            {
                double ni[2] = { inv0, inv1 };
                ni[player]   = inv[opp];
                acts.push_back(ActionInfo{ AK_CALL, inv[opp], "C",
                                           "Call " + num(to_call) });
                succ.push_back(Succ{ true, NT_SHOWDOWN, -1,
                                     cfg::POT0 * 0.5 + inv[opp],
                                     ni[0], ni[1], 0, false, false });
            }

            // ---- RAISE -----------------------------------------------------
            if (raises < c_.max_raises && inv[opp] < cfg::STACK - 1e-9) {
                for (double f : c_.raise_fracs) {
                    const double pot_after_call = cfg::POT0 + 2.0 * inv[opp];
                    double to = inv[opp] + f * pot_after_call;
                    const double min_to = inv[opp] + to_call;  // min-raise rule
                    if (to < min_to) to = min_to;
                    if (to > cfg::STACK) to = cfg::STACK;
                    if (to <= inv[opp] + 1e-9) continue;       // no chips behind
                    double ni[2] = { inv0, inv1 };
                    ni[player]   = to;
                    acts.push_back(ActionInfo{ AK_RAISE, to, "R" + num(to),
                                               "Raise to " + num(to) });
                    succ.push_back(Succ{ false, NT_DECISION, -1, 0.0,
                                         ni[0], ni[1], raises + 1, true, false });
                }
            }
        } else {
            // ---- CHECK -----------------------------------------------------
            acts.push_back(ActionInfo{ AK_CHECK, inv[player], "X", "Check" });
            if (prev_check) {
                // check-check closes the street -> showdown
                succ.push_back(Succ{ true, NT_SHOWDOWN, -1,
                                     cfg::POT0 * 0.5 + inv[player],
                                     inv0, inv1, 0, false, false });
            } else {
                succ.push_back(Succ{ false, NT_DECISION, -1, 0.0,
                                     inv0, inv1, raises, false, true });
            }

            // ---- BET -------------------------------------------------------
            if (raises < c_.max_raises && inv[player] < cfg::STACK - 1e-9) {
                for (double f : c_.bet_fracs) {
                    double to = inv[player] + f * pot;
                    if (to > cfg::STACK) to = cfg::STACK;
                    if (to <= inv[player] + 1e-9) continue;
                    double ni[2] = { inv0, inv1 };
                    ni[player]   = to;
                    acts.push_back(ActionInfo{ AK_BET, to, "B" + num(to),
                                               "Bet " + num(to - inv[player]) });
                    succ.push_back(Succ{ false, NT_DECISION, -1, 0.0,
                                         ni[0], ni[1], raises + 1, true, false });
                }
            }
        }

        if (static_cast<int>(acts.size()) > cfg::MAX_ACTIONS) {
            std::printf("FATAL: branching factor %d exceeds MAX_ACTIONS %d\n",
                        static_cast<int>(acts.size()), cfg::MAX_ACTIONS);
            std::fflush(stdout);
            std::abort();
        }

        // ---- materialise the node -----------------------------------------
        Node n;
        n.type          = NT_DECISION;
        n.player        = player;
        n.num_actions   = static_cast<int>(acts.size());
        n.pot           = pot;
        n.path          = path;
        n.action_offset = static_cast<int>(t_.actions.size());
        n.child_offset  = static_cast<int>(t_.children.size());

        for (const ActionInfo& a : acts) t_.actions.push_back(a);
        t_.children.resize(static_cast<size_t>(n.child_offset) + n.num_actions, -1);

        const int nid = static_cast<int>(t_.nodes.size());
        t_.nodes.push_back(n);

        // ---- recurse (node indices stay valid across reallocation) ---------
        for (int a = 0; a < static_cast<int>(succ.size()); ++a) {
            const Succ&       s     = succ[a];
            const std::string cpath = path + "/" + acts[a].code;
            int cid;
            if (s.terminal) {
                cid = add_terminal(s.ty, s.folder, s.W,
                                   cfg::POT0 + s.ni0 + s.ni1, cpath);
            } else {
                cid = rec(1 - player, s.ni0, s.ni1, s.nraises,
                          s.nfacing, s.nprev, cpath);
            }
            t_.children[t_.nodes[nid].child_offset + a] = cid;
        }
        return nid;
    }
};

// -----------------------------------------------------------------------------
//  Reporting sink. Every report function writes here instead of straight to
//  stdout, so the console can dump a full report into a file without any
//  freopen() trickery. Interactive chatter (progress, errors) keeps using
//  stdout directly.
// -----------------------------------------------------------------------------
static FILE* g_out = stdout;

// -----------------------------------------------------------------------------
//  Pretty printer for the betting tree.
// -----------------------------------------------------------------------------
static void print_tree(const GameTree& T, int nid, int depth) {
    const Node&       n = T.nodes[nid];
    const std::string ind(static_cast<size_t>(depth) * 2, ' ');

    if (n.type == NT_DECISION) {
        std::fprintf(g_out, "%s[%02d] %-18s %-3s to act  pot=%6.1f  ->", ind.c_str(),
                    nid, n.path.c_str(), (n.player == 0 ? "OOP" : "IP"), n.pot);
        for (int a = 0; a < n.num_actions; ++a)
            std::fprintf(g_out, " %s%s", T.act(n, a).label.c_str(),
                        (a + 1 < n.num_actions ? " |" : ""));
        std::fprintf(g_out, "\n");
        for (int a = 0; a < n.num_actions; ++a)
            print_tree(T, T.child(n, a), depth + 1);
    } else if (n.type == NT_SHOWDOWN) {
        std::fprintf(g_out, "%s[%02d] %-18s SHOWDOWN         pot=%6.1f  "
                    "(winner nets %+.1f)\n",
                    ind.c_str(), nid, n.path.c_str(), n.pot, n.terminal_W);
    } else {
        std::fprintf(g_out, "%s[%02d] %-18s %-3s FOLDS         pot=%6.1f  "
                    "(forfeits %.1f)\n",
                    ind.c_str(), nid, n.path.c_str(),
                    (n.player == 0 ? "OOP" : "IP"), n.pot, n.terminal_W);
    }
}

// =============================================================================
//  5. ANALYSIS RECORD  (per node, filled by the post-solve evaluation pass)
// =============================================================================
struct NodeAnalysis {
    HandVec own_reach{};   // reach of the node owner (range weight * own strategy)
    HandVec opp_reach{};   // reach of the other player
    HandVec cfv{};         // counterfactual value of the node, per hand
    std::array<HandVec, cfg::MAX_ACTIONS> action_cfv{};  // per action, per hand
    bool    filled = false;
};

enum EvalMode { EM_AVERAGE = 0, EM_BEST_RESPONSE };

// =============================================================================
//  6. DCFR SOLVER
// =============================================================================
class DCFRSolver {
public:
    DCFRSolver(GameTree& tree, const std::array<HandVec, 2>& ranges)
        : T(tree), range(ranges) {
        regret.assign(static_cast<size_t>(T.mem_size), 0.0);
        strat_sum.assign(static_cast<size_t>(T.mem_size), 0.0);
        cur.assign(static_cast<size_t>(T.mem_size), 0.0);
        avg.assign(static_cast<size_t>(T.mem_size), 0.0);
        analysis.assign(T.nodes.size(), NodeAnalysis{});
        update_current_strategies();   // uniform seed
        avg = cur;
    }

    // ---------------------------------------------------------------------
    //  NODELOCKING API
    // ---------------------------------------------------------------------
    // Freeze an explicit list of hands at a node to a fixed action mix.
    bool lock_hands(int nid, const std::vector<int>& hands,
                    const std::vector<double>& probs) {
        if (nid < 0 || nid >= static_cast<int>(T.nodes.size())) return false;
        Node& n = T.nodes[nid];
        if (n.type != NT_DECISION) return false;
        if (static_cast<int>(probs.size()) != n.num_actions) return false;

        double s = 0.0;
        for (double p : probs) {
            if (p < 0.0) return false;
            s += p;
        }
        if (s <= 1e-12) return false;

        if (n.locked_hand.empty()) {
            n.locked_hand.assign(static_cast<size_t>(NH), static_cast<uint8_t>(0));
            n.locked_strategy.assign(static_cast<size_t>(n.num_actions) * NH, 0.0);
        }
        for (int h : hands) {
            if (h < 0 || h >= NH) continue;
            n.locked_hand[h] = 1u;
            for (int a = 0; a < n.num_actions; ++a)
                n.locked_strategy[static_cast<size_t>(a) * NH + h] = probs[a] / s;
        }
        n.is_locked = true;
        return true;
    }

    // Freeze hands specifying the mix by ACTION KIND, so the call site never
    // depends on the action ordering the builder happened to produce (and so a
    // lock survives a tree rebuild with different sizings).
    bool lock_hands_kind(int nid, const std::vector<int>& hands,
                         const std::vector<std::pair<ActionKind, double>>& mix) {
        if (nid < 0 || nid >= static_cast<int>(T.nodes.size())) return false;
        const Node& n = T.nodes[nid];
        if (n.type != NT_DECISION) return false;

        std::vector<double> probs(static_cast<size_t>(n.num_actions), 0.0);
        for (const auto& pr : mix) {
            const int a = T.action_index(n, pr.first);
            if (a < 0) return false;   // that action does not exist at this node
            probs[static_cast<size_t>(a)] += pr.second;
        }
        return lock_hands(nid, hands, probs);
    }

    // Freeze a whole hand-type bucket.
    bool lock_hand_type(int nid, HandType ht,
                        const std::vector<std::pair<ActionKind, double>>& mix) {
        std::vector<int> hands;
        for (int h = 0; h < NH; ++h)
            if (classify(h) == ht) hands.push_back(h);
        return lock_hands_kind(nid, hands, mix);
    }

    // Release every lock at a node.
    bool clear_locks(int nid) {
        if (nid < 0 || nid >= static_cast<int>(T.nodes.size())) return false;
        Node& n = T.nodes[nid];
        if (n.type != NT_DECISION) return false;
        n.is_locked = false;
        n.locked_hand.clear();
        n.locked_strategy.clear();
        return true;
    }

    // ---------------------------------------------------------------------
    //  MAIN LOOP
    // ---------------------------------------------------------------------
    // Runs `iterations` more DCFR iterations, continuing from whatever has
    // already been accumulated, so the console can `solve 1000` and then
    // `iterate 9000` to tighten the same solution.
    void run(int iterations, int report_every) {
        if (report_every > 0)
            std::printf("   %-8s  %-14s  %-14s  %s\n", "iter",
                        "BR(OOP)", "BR(IP)", "exploitability");

        for (int k = 1; k <= iterations; ++k) {
            ++t_done;
            update_current_strategies();
            // Simultaneous updates: both traversals read the SAME frozen
            // current strategy, so the two regret updates cannot interfere.
            cfr(T.root, range[0], range[1], 0);
            cfr(T.root, range[1], range[0], 1);
            discount(t_done);

            if (report_every > 0 && (k == 1 || k % report_every == 0)) {
                build_average();
                const double b0 = best_response_value(0);
                const double b1 = best_response_value(1);
                std::printf("   %-8d  %+10.5f     %+10.5f     %10.6f chips "
                            "(%.4f%% pot)\n",
                            t_done, b0, b1, b0 + b1,
                            100.0 * (b0 + b1) / cfg::POT0);
            }
        }
        build_average();
        analyze();
    }

    int iterations_done() const { return t_done; }

    // ---------------------------------------------------------------------
    //  QUERIES  (all values already normalised into chips)
    // ---------------------------------------------------------------------
    double avg_strategy(int nid, int h, int a) const {
        const Node& n = T.nodes[nid];
        return avg[static_cast<size_t>(n.mem_offset) + static_cast<size_t>(a) * NH + h];
    }

    double opp_total(int nid) const {
        double s = 0.0;
        for (double x : analysis[nid].opp_reach) s += x;
        return s;
    }

    // Number of (my hand, villain hand) combinations that reach this node.
    double combo_weight(int nid, int h) const {
        const NodeAnalysis& na = analysis[nid];
        return na.own_reach[h] * (opp_total(nid) - na.opp_reach[h]);
    }

    // EV in chips of taking action `a` with hand `h` at node `nid`.
    double hand_action_ev(int nid, int h, int a) const {
        const NodeAnalysis& na = analysis[nid];
        const double d = opp_total(nid) - na.opp_reach[h];
        if (d < 1e-12) return 0.0;
        return na.action_cfv[a][h] / d + cfg::POT0 * 0.5;
    }

    // EV in chips of hand `h` at node `nid` under the solved strategy.
    double hand_node_ev(int nid, int h) const {
        const NodeAnalysis& na = analysis[nid];
        const double d = opp_total(nid) - na.opp_reach[h];
        if (d < 1e-12) return 0.0;
        return na.cfv[h] / d + cfg::POT0 * 0.5;
    }

    double bucket_weight(int nid, HandType ht) const {
        double w = 0.0;
        for (int h = 0; h < NH; ++h)
            if (classify(h) == ht) w += combo_weight(nid, h);
        return w;
    }

    double bucket_freq(int nid, HandType ht, int a) const {
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            if (classify(h) != ht) continue;
            const double w = combo_weight(nid, h);
            den += w;
            num += w * avg_strategy(nid, h, a);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    double bucket_action_ev(int nid, HandType ht, int a) const {
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            if (classify(h) != ht) continue;
            const double w = combo_weight(nid, h);
            den += w;
            num += w * hand_action_ev(nid, h, a);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    double bucket_node_ev(int nid, HandType ht) const {
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            if (classify(h) != ht) continue;
            const double w = combo_weight(nid, h);
            den += w;
            num += w * hand_node_ev(nid, h);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    double node_freq(int nid, int a) const {
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            const double w = combo_weight(nid, h);
            den += w;
            num += w * avg_strategy(nid, h, a);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    double node_ev(int nid) const {
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            const double w = combo_weight(nid, h);
            den += w;
            num += w * hand_node_ev(nid, h);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    // Fraction of all starting (my hand, villain hand) combinations that
    // actually reach this node under the solved strategies.
    double node_reach_pct(int nid) const {
        const Node& n   = T.nodes[nid];
        const int   opp = 1 - n.player;
        double rtot = 0.0;
        for (int h = 0; h < NH; ++h) rtot += range[opp][h];
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            num += combo_weight(nid, h);
            den += range[n.player][h] * (rtot - range[opp][h]);
        }
        return den > 1e-12 ? 100.0 * num / den : 0.0;
    }

    bool is_hand_locked(int nid, int h) const {
        const Node& n = T.nodes[nid];
        return n.is_locked && !n.locked_hand.empty() && n.locked_hand[h] != 0u;
    }

    // Constrained best response: locked hands keep playing their frozen mix.
    double best_response_value(int br) {
        const HandVec v = eval(T.root, range[br], range[1 - br], br, avg,
                               EM_BEST_RESPONSE, nullptr);
        double opp_tot = 0.0;
        for (double x : range[1 - br]) opp_tot += x;
        double num = 0.0, den = 0.0;
        for (int h = 0; h < NH; ++h) {
            num += range[br][h] * v[h];
            den += range[br][h] * (opp_tot - range[1 - br][h]);
        }
        return den > 1e-12 ? num / den : 0.0;
    }

    double exploitability() { return best_response_value(0) + best_response_value(1); }

    const GameTree&                  tree()     const { return T; }
    const std::vector<NodeAnalysis>& analyses() const { return analysis; }
    const std::array<HandVec, 2>&    ranges()   const { return range; }

private:
    GameTree&              T;
    std::array<HandVec, 2> range;
    int                    t_done = 0;   // iterations accumulated so far

    // ---- FLAT INFORMATION-SET MEMORY (Structure of Arrays) ---------------
    std::vector<double> regret;     // cumulative (discounted) regret
    std::vector<double> strat_sum;  // cumulative (discounted) strategy
    std::vector<double> cur;        // current strategy of this iteration
    std::vector<double> avg;        // normalised average strategy

    std::vector<NodeAnalysis> analysis;

    // ---------------------------------------------------------------------
    //  Terminal evaluation, vectorised over the opponent's whole range.
    //  Card removal is exact: entry i of the opponent vector is never counted
    //  when we hold card i.
    // ---------------------------------------------------------------------
    static HandVec showdown_cfv(const HandVec& ro, double W) {
        double pre[NH + 1];
        pre[0] = 0.0;
        for (int i = 0; i < NH; ++i) pre[i + 1] = pre[i] + ro[i];
        const double tot = pre[NH];

        HandVec v;
        for (int i = 0; i < NH; ++i) {
            const double lower  = pre[i];            // villain hands we beat
            const double higher = tot - pre[i + 1];  // villain hands that beat us
            v[i] = W * (lower - higher);
        }
        return v;
    }

    static HandVec fold_cfv(const HandVec& ro, double signed_W) {
        double tot = 0.0;
        for (double x : ro) tot += x;
        HandVec v;
        for (int i = 0; i < NH; ++i) v[i] = signed_W * (tot - ro[i]);
        return v;
    }

    // ---------------------------------------------------------------------
    //  Regret matching+ over the flat buffers, honouring nodelocks.
    // ---------------------------------------------------------------------
    void update_current_strategies() {
        for (const Node& n : T.nodes) {
            if (n.type != NT_DECISION) continue;
            const int    A = n.num_actions;
            const size_t M = static_cast<size_t>(n.mem_offset);

            for (int h = 0; h < NH; ++h) {
                if (n.is_locked && n.locked_hand[h]) {
                    for (int a = 0; a < A; ++a)
                        cur[M + static_cast<size_t>(a) * NH + h] =
                            n.locked_strategy[static_cast<size_t>(a) * NH + h];
                    continue;
                }
                double tmp[cfg::MAX_ACTIONS];
                double s = 0.0;
                for (int a = 0; a < A; ++a) {
                    const double r = regret[M + static_cast<size_t>(a) * NH + h];
                    tmp[a] = (r > 0.0) ? r : 0.0;
                    s += tmp[a];
                }
                if (s > 1e-12) {
                    const double inv_s = 1.0 / s;
                    for (int a = 0; a < A; ++a)
                        cur[M + static_cast<size_t>(a) * NH + h] = tmp[a] * inv_s;
                } else {
                    const double u = 1.0 / static_cast<double>(A);
                    for (int a = 0; a < A; ++a)
                        cur[M + static_cast<size_t>(a) * NH + h] = u;
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    //  Vector-form CFR traversal.
    //    rs = reach of the traverser, ro = reach of the opponent.
    //    Returns the traverser's counterfactual value for every hand.
    // ---------------------------------------------------------------------
    HandVec cfr(int nid, const HandVec& rs, const HandVec& ro, int trav) {
        const Node& n = T.nodes[nid];

        if (n.type == NT_SHOWDOWN) return showdown_cfv(ro, n.terminal_W);
        if (n.type == NT_FOLD) {
            const double sgn = (n.player == trav) ? -1.0 : 1.0;
            return fold_cfv(ro, sgn * n.terminal_W);
        }

        const int    A = n.num_actions;
        const size_t M = static_cast<size_t>(n.mem_offset);
        std::array<HandVec, cfg::MAX_ACTIONS> ch;
        HandVec out{};

        if (n.player == trav) {
            for (int a = 0; a < A; ++a) {
                const double* st = &cur[M + static_cast<size_t>(a) * NH];
                HandVec nr;
                for (int h = 0; h < NH; ++h) nr[h] = rs[h] * st[h];
                ch[a] = cfr(T.child(n, a), nr, ro, trav);
            }
            for (int a = 0; a < A; ++a) {
                const double* st = &cur[M + static_cast<size_t>(a) * NH];
                for (int h = 0; h < NH; ++h) out[h] += st[h] * ch[a][h];
            }
            const bool lk = n.is_locked;
            for (int a = 0; a < A; ++a) {
                double*       rg = &regret[M + static_cast<size_t>(a) * NH];
                double*       ss = &strat_sum[M + static_cast<size_t>(a) * NH];
                const double* st = &cur[M + static_cast<size_t>(a) * NH];
                for (int h = 0; h < NH; ++h) {
                    // Locked hands are frozen: they never accumulate regret,
                    // but their fixed strategy still propagates values.
                    if (!(lk && n.locked_hand[h])) rg[h] += ch[a][h] - out[h];
                    ss[h] += rs[h] * st[h];
                }
            }
        } else {
            for (int a = 0; a < A; ++a) {
                const double* st = &cur[M + static_cast<size_t>(a) * NH];
                HandVec nr;
                for (int h = 0; h < NH; ++h) nr[h] = ro[h] * st[h];
                ch[a] = cfr(T.child(n, a), rs, nr, trav);
                for (int h = 0; h < NH; ++h) out[h] += ch[a][h];
            }
        }
        return out;
    }

    // ---------------------------------------------------------------------
    //  DCFR discounting, applied to the whole flat buffer once per iteration.
    // ---------------------------------------------------------------------
    void discount(int t) {
        const double td   = static_cast<double>(t);
        const double ta   = std::pow(td, cfg::DCFR_ALPHA);
        const double tb   = std::pow(td, cfg::DCFR_BETA);
        const double dpos = ta / (ta + 1.0);
        const double dneg = tb / (tb + 1.0);
        const double dstr = std::pow(td / (td + 1.0), cfg::DCFR_GAMMA);

        const size_t nsz = regret.size();
        for (size_t i = 0; i < nsz; ++i) {
            regret[i] *= (regret[i] > 0.0) ? dpos : dneg;
            strat_sum[i] *= dstr;
        }
    }

    // ---------------------------------------------------------------------
    //  Average strategy extraction (the "frequencies" a solver reports).
    // ---------------------------------------------------------------------
    void build_average() {
        for (const Node& n : T.nodes) {
            if (n.type != NT_DECISION) continue;
            const int    A = n.num_actions;
            const size_t M = static_cast<size_t>(n.mem_offset);

            for (int h = 0; h < NH; ++h) {
                if (n.is_locked && n.locked_hand[h]) {
                    for (int a = 0; a < A; ++a)
                        avg[M + static_cast<size_t>(a) * NH + h] =
                            n.locked_strategy[static_cast<size_t>(a) * NH + h];
                    continue;
                }
                double s = 0.0;
                for (int a = 0; a < A; ++a)
                    s += strat_sum[M + static_cast<size_t>(a) * NH + h];
                if (s > 1e-12) {
                    const double inv_s = 1.0 / s;
                    for (int a = 0; a < A; ++a)
                        avg[M + static_cast<size_t>(a) * NH + h] =
                            strat_sum[M + static_cast<size_t>(a) * NH + h] * inv_s;
                } else {
                    for (int a = 0; a < A; ++a)
                        avg[M + static_cast<size_t>(a) * NH + h] =
                            cur[M + static_cast<size_t>(a) * NH + h];
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    //  Evaluation traversal. Identical shape to cfr(), but it never touches
    //  regrets: it plays a fixed strategy and (optionally) records the full
    //  per-hand / per-action counterfactual values into `out`.
    //  In EM_BEST_RESPONSE the traverser maximises instead of mixing, except
    //  on locked hands, which stay frozen (constrained best response).
    // ---------------------------------------------------------------------
    HandVec eval(int nid, const HandVec& rs, const HandVec& ro, int trav,
                 const std::vector<double>& st, EvalMode mode,
                 std::vector<NodeAnalysis>* out) const {
        const Node& n = T.nodes[nid];

        if (n.type == NT_SHOWDOWN) return showdown_cfv(ro, n.terminal_W);
        if (n.type == NT_FOLD) {
            const double sgn = (n.player == trav) ? -1.0 : 1.0;
            return fold_cfv(ro, sgn * n.terminal_W);
        }

        const int    A = n.num_actions;
        const size_t M = static_cast<size_t>(n.mem_offset);
        std::array<HandVec, cfg::MAX_ACTIONS> ch;
        HandVec res{};

        if (n.player == trav) {
            for (int a = 0; a < A; ++a) {
                const double* p = &st[M + static_cast<size_t>(a) * NH];
                HandVec nr;
                for (int h = 0; h < NH; ++h) nr[h] = rs[h] * p[h];
                ch[a] = eval(T.child(n, a), nr, ro, trav, st, mode, out);
            }
            for (int h = 0; h < NH; ++h) {
                const bool frozen = n.is_locked && n.locked_hand[h];
                if (mode == EM_BEST_RESPONSE && !frozen) {
                    double best = ch[0][h];
                    for (int a = 1; a < A; ++a)
                        if (ch[a][h] > best) best = ch[a][h];
                    res[h] = best;
                } else {
                    double s = 0.0;
                    for (int a = 0; a < A; ++a)
                        s += st[M + static_cast<size_t>(a) * NH + h] * ch[a][h];
                    res[h] = s;
                }
            }
            if (out) {
                NodeAnalysis& na = (*out)[nid];
                na.own_reach = rs;
                na.opp_reach = ro;
                na.cfv       = res;
                for (int a = 0; a < A; ++a) na.action_cfv[a] = ch[a];
                na.filled = true;
            }
        } else {
            for (int a = 0; a < A; ++a) {
                const double* p = &st[M + static_cast<size_t>(a) * NH];
                HandVec nr;
                for (int h = 0; h < NH; ++h) nr[h] = ro[h] * p[h];
                ch[a] = eval(T.child(n, a), rs, nr, trav, st, mode, out);
                for (int h = 0; h < NH; ++h) res[h] += ch[a][h];
            }
        }
        return res;
    }

    // Two passes: each one fills the nodes owned by its traverser.
    void analyze() {
        analysis.assign(T.nodes.size(), NodeAnalysis{});
        eval(T.root, range[0], range[1], 0, avg, EM_AVERAGE, &analysis);
        eval(T.root, range[1], range[0], 1, avg, EM_AVERAGE, &analysis);
    }
};

// =============================================================================
//  7. REPORTING
// =============================================================================
static void hr(const char* title) {
    std::fprintf(g_out, "\n"
        "=============================================================================\n"
        "  %s\n"
        "=============================================================================\n",
        title);
}

static void report_ranges(const std::array<HandVec, 2>& r) {
    std::fprintf(g_out, "  Hand-type buckets (13 single-card holdings, higher rank wins):\n");
    for (int t = HT_COUNT - 1; t >= 0; --t) {
        std::fprintf(g_out, "    %-14s : ", HT_NAME[t]);
        for (int h = NH - 1; h >= 0; --h)
            if (classify(h) == static_cast<HandType>(t))
                std::fprintf(g_out, "%s ", RANK_NAME[h]);
        std::fprintf(g_out, "\n");
    }
    std::fprintf(g_out, "\n  Range weights:\n    %-5s", "OOP");
    for (int h = 0; h < NH; ++h) std::fprintf(g_out, " %s:%.2f", RANK_NAME[h], r[0][h]);
    std::fprintf(g_out, "\n    %-5s", "IP");
    for (int h = 0; h < NH; ++h) std::fprintf(g_out, " %s:%.2f", RANK_NAME[h], r[1][h]);
    std::fprintf(g_out, "\n");
}

// (a) Global action frequencies at every decision node of the tree.
static void report_frequencies(const DCFRSolver& S) {
    const GameTree& T = S.tree();
    std::fprintf(g_out, "  %-18s %-4s %8s %9s   %s\n",
                "NODE", "WHO", "REACH%", "EV(chips)", "ACTION FREQUENCIES");
    std::fprintf(g_out, "  ---------------------------------------------------------"
                "--------------------\n");
    for (int nid = 0; nid < static_cast<int>(T.nodes.size()); ++nid) {
        const Node& n = T.nodes[nid];
        if (n.type != NT_DECISION) continue;
        std::fprintf(g_out, "  %-18s %-4s %7.2f%% %9.3f   ", n.path.c_str(),
                    (n.player == 0 ? "OOP" : "IP"),
                    S.node_reach_pct(nid), S.node_ev(nid));
        for (int a = 0; a < n.num_actions; ++a)
            std::fprintf(g_out, "%s %5.2f%%%s", T.act(n, a).code.c_str(),
                        100.0 * S.node_freq(nid, a),
                        (a + 1 < n.num_actions ? "  " : ""));
        if (n.is_locked) std::fprintf(g_out, "   <-- NODELOCKED");
        std::fprintf(g_out, "\n");
    }
    std::fprintf(g_out, "\n  Legend: X=check  B15=bet 15  F=fold  C=call  Rnn=raise to nn\n");
}

// (b) Strategy + EV aggregated by hand type at one node.
static void report_buckets(const DCFRSolver& S, int nid, const char* title) {
    const GameTree& T = S.tree();
    const Node&     n = T.nodes[nid];
    std::fprintf(g_out, "\n  %s   [node %s, %s to act, pot %.1f]\n", title,
                n.path.c_str(), (n.player == 0 ? "OOP" : "IP"), n.pot);
    std::fprintf(g_out, "  %-14s %8s", "HAND TYPE", "COMBO%");
    for (int a = 0; a < n.num_actions; ++a)
        std::fprintf(g_out, " | %-4s %6s %7s", T.act(n, a).code.c_str(), "freq", "EV");
    std::fprintf(g_out, " | %8s\n", "EV(node)");

    double wtot = 0.0;
    for (int h = 0; h < NH; ++h) wtot += S.combo_weight(nid, h);

    for (int t = HT_COUNT - 1; t >= 0; --t) {
        const HandType ht = static_cast<HandType>(t);
        const double   bw = S.bucket_weight(nid, ht);
        std::fprintf(g_out, "  %-14s %7.2f%%", HT_NAME[t],
                    wtot > 1e-12 ? 100.0 * bw / wtot : 0.0);
        for (int a = 0; a < n.num_actions; ++a)
            std::fprintf(g_out, " | %-4s %5.1f%% %7.3f", "",
                        100.0 * S.bucket_freq(nid, ht, a),
                        S.bucket_action_ev(nid, ht, a));
        std::fprintf(g_out, " | %8.3f\n", S.bucket_node_ev(nid, ht));
    }
}

// Per-hand strategy and per-action counterfactual EV at one node.
static void report_hand_table(const DCFRSolver& S, int nid, const char* title) {
    const GameTree& T = S.tree();
    const Node&     n = T.nodes[nid];
    std::fprintf(g_out, "\n  %s   [node %s, %s to act, pot %.1f]\n", title,
                n.path.c_str(), (n.player == 0 ? "OOP" : "IP"), n.pot);
    std::fprintf(g_out, "  %-5s %-4s %-14s %7s", "HAND", "LOCK", "TYPE", "COMBO%");
    for (int a = 0; a < n.num_actions; ++a)
        std::fprintf(g_out, " | %-4s %6s %7s", T.act(n, a).code.c_str(), "freq", "EV");
    std::fprintf(g_out, " | %8s\n", "EV(node)");

    double wtot = 0.0;
    for (int h = 0; h < NH; ++h) wtot += S.combo_weight(nid, h);

    for (int h = NH - 1; h >= 0; --h) {
        std::fprintf(g_out, "  %-5s %-4s %-14s %6.2f%%", RANK_NAME[h],
                    S.is_hand_locked(nid, h) ? "LOCK" : "",
                    HT_NAME[classify(h)],
                    wtot > 1e-12 ? 100.0 * S.combo_weight(nid, h) / wtot : 0.0);
        for (int a = 0; a < n.num_actions; ++a)
            std::fprintf(g_out, " | %-4s %5.1f%% %7.3f", "",
                        100.0 * S.avg_strategy(nid, h, a),
                        S.hand_action_ev(nid, h, a));
        std::fprintf(g_out, " | %8.3f\n", S.hand_node_ev(nid, h));
    }
}

// (c) EV of BET vs CHECK, per hand type.
static void report_bet_vs_check(const DCFRSolver& S, int nid) {
    const GameTree& T  = S.tree();
    const Node&     n  = T.nodes[nid];
    const int       aX = T.action_index(n, AK_CHECK);
    const int       aB = T.action_index(n, AK_BET);
    if (aX < 0 || aB < 0) {
        std::fprintf(g_out, "  (node %s has no check/bet pair)\n", n.path.c_str());
        return;
    }
    std::fprintf(g_out, "  %-14s | %10s %10s | %10s %10s | %8s | %s\n", "HAND TYPE",
                "EV(Check)", "EV(Bet)", "freq Check", "freq Bet", "EV diff",
                "BETTER LINE");
    std::fprintf(g_out, "  ------------------------------------------------------------"
                "---------------------------\n");
    for (int t = HT_COUNT - 1; t >= 0; --t) {
        const HandType ht  = static_cast<HandType>(t);
        const double   evX = S.bucket_action_ev(nid, ht, aX);
        const double   evB = S.bucket_action_ev(nid, ht, aB);
        const double   d   = evB - evX;
        const char*    win = (std::fabs(d) < 1e-4) ? "indifferent"
                                                   : (d > 0.0 ? "BET" : "CHECK");
        std::fprintf(g_out, "  %-14s | %10.4f %10.4f | %9.2f%% %9.2f%% | %+8.4f | %s\n",
                    HT_NAME[t], evX, evB,
                    100.0 * S.bucket_freq(nid, ht, aX),
                    100.0 * S.bucket_freq(nid, ht, aB), d, win);
    }
}

// Side-by-side comparison of two solves (baseline vs nodelocked).
static void report_delta(const DCFRSolver& A, const DCFRSolver& B, int nid) {
    const GameTree& T  = A.tree();
    const Node&     n  = T.nodes[nid];
    const int       aX = T.action_index(n, AK_CHECK);
    const int       aB = T.action_index(n, AK_BET);
    if (aX < 0 || aB < 0) return;

    std::fprintf(g_out, "  OOP strategy at the root, GTO baseline vs exploitative "
                "(vs calling station)\n\n");
    std::fprintf(g_out, "  %-14s | %-22s | %-22s | %s\n", "HAND TYPE",
                "BET freq  (GTO -> LOCK)", "EV(Bet)   (GTO -> LOCK)",
                "EV(hand)  (GTO -> LOCK)");
    std::fprintf(g_out, "  ------------------------------------------------------------"
                "---------------------------------\n");
    for (int t = HT_COUNT - 1; t >= 0; --t) {
        const HandType ht = static_cast<HandType>(t);
        std::fprintf(g_out, "  %-14s | %7.2f%% -> %7.2f%%  %+6.2f | %7.3f -> %7.3f "
                    "%+6.3f | %7.3f -> %7.3f %+6.3f\n",
                    HT_NAME[t],
                    100.0 * A.bucket_freq(nid, ht, aB),
                    100.0 * B.bucket_freq(nid, ht, aB),
                    100.0 * (B.bucket_freq(nid, ht, aB) - A.bucket_freq(nid, ht, aB)),
                    A.bucket_action_ev(nid, ht, aB),
                    B.bucket_action_ev(nid, ht, aB),
                    B.bucket_action_ev(nid, ht, aB) - A.bucket_action_ev(nid, ht, aB),
                    A.bucket_node_ev(nid, ht),
                    B.bucket_node_ev(nid, ht),
                    B.bucket_node_ev(nid, ht) - A.bucket_node_ev(nid, ht));
    }
}

static void report_game_value(const DCFRSolver& S) {
    const int    root = S.tree().root;
    const double ev0  = S.node_ev(root);
    const double ev1  = cfg::POT0 - ev0;
    std::fprintf(g_out, "\n  Game value : OOP = %.4f chips   IP = %.4f chips   "
                "(constant-sum: OOP + IP = %.4f = pot %.1f)\n",
                ev0, ev1, ev0 + ev1, cfg::POT0);
}

// =============================================================================
//  8. SCRIPTED DEMO  (solver.exe --demo)
//     The original hard-coded showcase: solve the spot twice, once free and
//     once with IP locked into calling every AIR hand, and diff the two.
// =============================================================================
static int run_demo() {
    std::printf(
        "#############################################################################\n"
        "#   DCFR RIVER SOLVER  --  Discounted CFR, flat memory, nodelocking,        #\n"
        "#                          per-hand EV extraction and hand bucketing.       #\n"
        "#############################################################################\n");

    // ---------------------------------------------------------------- tree
    TreeConfig tc;
    tc.bet_fracs   = { 0.75 };  // 75% pot bet
    tc.raise_fracs = { 1.00 };  // pot-sized raise
    tc.max_raises  = 2;

    GameTree base = TreeBuilder(tc).build();

    hr("1. BETTING TREE");
    std::printf("  Initial pot %.1f   effective stack %.1f   max raises %d\n\n",
                cfg::POT0, cfg::STACK, tc.max_raises);
    print_tree(base, base.root, 1);
    std::printf("\n  Nodes: %d total   flat info-set memory: %d doubles per buffer "
                "(%lu bytes)\n",
                static_cast<int>(base.nodes.size()), base.mem_size,
                static_cast<unsigned long>(sizeof(double)) *
                    static_cast<unsigned long>(base.mem_size));

    // -------------------------------------------------------------- ranges
    std::array<HandVec, 2> ranges;
    for (int p = 0; p < 2; ++p)
        for (int h = 0; h < NH; ++h) ranges[p][h] = 1.0;  // uniform, all 13 cards

    hr("2. RANGES AND HAND-TYPE CLASSIFIER");
    report_ranges(ranges);

    const int ITERS = 1000;

    // ------------------------------------------------------ node shortcuts
    const int root_id   = base.root;
    const int ip_vs_bet = base.child_by_kind(base.root, AK_BET);   // IP faces the bet
    const int ip_vs_chk = base.child_by_kind(base.root, AK_CHECK); // IP after a check

    // =====================================================================
    //  SOLVE 1 : unconstrained equilibrium (baseline)
    // =====================================================================
    hr("3. SOLVE #1 -- GTO BASELINE (no nodelock), 1000 DCFR iterations");
    std::printf("  DCFR parameters: alpha=%.2f  beta=%.2f  gamma=%.2f\n\n",
                cfg::DCFR_ALPHA, cfg::DCFR_BETA, cfg::DCFR_GAMMA);

    GameTree   treeGTO = base;
    DCFRSolver gto(treeGTO, ranges);
    gto.run(ITERS, 100);
    report_game_value(gto);

    hr("3a. GLOBAL ACTION FREQUENCIES  (GTO baseline)");
    report_frequencies(gto);

    hr("3b. STRATEGY BY HAND TYPE  (GTO baseline)");
    report_buckets(gto, root_id,   "OOP first decision");
    report_buckets(gto, ip_vs_bet, "IP facing the 75% pot bet");

    hr("3c. PER-HAND STRATEGY AND EV  (GTO baseline)");
    report_hand_table(gto, root_id, "OOP first decision");

    // =====================================================================
    //  SOLVE 2 : nodelocked -- IP is a calling station with AIR
    // =====================================================================
    hr("4. SOLVE #2 -- NODELOCK: IP CALLS 100% WITH AIR (calling station)");

    GameTree   treeLOCK = base;
    DCFRSolver exploit(treeLOCK, ranges);

    const bool ok = exploit.lock_hand_type(ip_vs_bet, HT_AIR,
                                           { { AK_CALL, 1.0 } });
    std::printf("  Lock applied at node %s (IP facing the bet): %s\n",
                treeLOCK.nodes[ip_vs_bet].path.c_str(),
                ok ? "OK -- AIR calls 100%, never folds, never raises"
                   : "FAILED");
    std::printf("  Locked hands: ");
    for (int h = 0; h < NH; ++h)
        if (exploit.is_hand_locked(ip_vs_bet, h)) std::printf("%s ", RANK_NAME[h]);
    std::printf("\n  Those hands accumulate NO regret; the rest of the tree "
                "best-responds around them.\n\n");

    exploit.run(ITERS, 100);
    report_game_value(exploit);
    std::printf("  (exploitability above is the CONSTRAINED one: the locked "
                "hands are not free to deviate)\n");

    hr("4a. GLOBAL ACTION FREQUENCIES  (nodelocked)");
    report_frequencies(exploit);

    hr("4b. OOP EXPLOITATIVE STRATEGY BY HAND TYPE  (nodelocked)");
    report_buckets(exploit, root_id,   "OOP first decision");
    report_buckets(exploit, ip_vs_bet, "IP facing the 75% pot bet (LOCKED node)");
    report_buckets(exploit, ip_vs_chk, "IP after OOP checks");

    hr("4c. PER-HAND STRATEGY AND EV  (nodelocked)");
    report_hand_table(exploit, root_id,   "OOP first decision");
    report_hand_table(exploit, ip_vs_bet, "IP facing the bet -- LOCK visible here");

    // =====================================================================
    //  DELTA + REQUIRED EV SUMMARY
    // =====================================================================
    hr("5. EXPLOITATIVE ADJUSTMENT  (baseline  ->  nodelocked)");
    report_delta(gto, exploit, root_id);

    hr("6. EV OF BET vs CHECK BY HAND TYPE  (the requested breakdown)");
    std::printf("\n  --- GTO BASELINE ---\n");
    report_bet_vs_check(gto, root_id);
    std::printf("\n  --- VS CALLING STATION (nodelocked) ---\n");
    report_bet_vs_check(exploit, root_id);

    std::printf("\n  Reading: EV is in chips out of a %.0f chip pot, net of chips\n"
                "  invested in this subgame. 0.00 = lose everything, %.0f = win it all.\n",
                cfg::POT0, cfg::POT0);

    hr("DONE");
    return 0;
}

// =============================================================================
//  9. INTERACTIVE CONSOLE
//     A REPL over the solver: configure a spot, solve it, walk the tree, read
//     strategies and EVs, drop nodelocks, re-solve, export. Same command
//     grammar whether it is typed, piped, or fed with --script.
// =============================================================================

// ---- small string helpers ---------------------------------------------------
static std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(trim(cur)); cur.clear(); }
        else cur += c;
    }
    out.push_back(trim(cur));
    return out;
}

static std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream is(s);
    std::string t;
    while (is >> t) out.push_back(t);
    return out;
}

static bool parse_double(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        const double v = std::stod(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) { return false; }
}

static bool parse_int(const std::string& s, int& out) {
    double d;
    if (!parse_double(s, d)) return false;
    out = static_cast<int>(d);
    return true;
}

// "A" -> 12, "2" -> 0, "t"/"T" -> 8. Returns -1 if not a rank.
static int rank_index(const std::string& s) {
    const std::string u = upper(trim(s));
    for (int h = 0; h < NH; ++h)
        if (u == RANK_NAME[h]) return h;
    return -1;
}

// Bucket name -> HandType, or HT_COUNT if the token is not a bucket name.
static HandType bucket_by_name(const std::string& s) {
    const std::string u = upper(trim(s));
    if (u == "AIR")                                        return HT_AIR;
    if (u == "BC" || u == "BLUFFCATCHER" ||
        u == "BLUFF-CATCHER" || u == "BLUFF_CATCHER")      return HT_BLUFFCATCHER;
    if (u == "VALUE" || u == "VAL")                        return HT_VALUE;
    if (u == "NUTS" || u == "NUT")                         return HT_NUTS;
    return HT_COUNT;
}

// Hand selector: "AIR", "A", "2-5", "all", or any comma-separated mix of them.
// Returns false and fills `err` when a token makes no sense.
static bool parse_hands(const std::string& spec, std::vector<int>& out,
                        std::string& err) {
    std::vector<uint8_t> mark(static_cast<size_t>(NH), 0u);
    for (const std::string& tokRaw : split(spec, ',')) {
        const std::string tok = trim(tokRaw);
        if (tok.empty()) continue;

        if (lower(tok) == "all") {
            for (int h = 0; h < NH; ++h) mark[h] = 1u;
            continue;
        }
        const HandType ht = bucket_by_name(tok);
        if (ht != HT_COUNT) {
            for (int h = 0; h < NH; ++h) if (classify(h) == ht) mark[h] = 1u;
            continue;
        }
        const size_t dash = tok.find('-');
        if (dash != std::string::npos && dash > 0 && dash + 1 < tok.size()) {
            const int lo = rank_index(tok.substr(0, dash));
            const int hi = rank_index(tok.substr(dash + 1));
            if (lo < 0 || hi < 0) { err = "bad rank range '" + tok + "'"; return false; }
            for (int h = std::min(lo, hi); h <= std::max(lo, hi); ++h) mark[h] = 1u;
            continue;
        }
        const int r = rank_index(tok);
        if (r < 0) { err = "unknown hand selector '" + tok + "'"; return false; }
        mark[r] = 1u;
    }
    out.clear();
    for (int h = 0; h < NH; ++h) if (mark[h]) out.push_back(h);
    if (out.empty()) { err = "selector matched no hands"; return false; }
    return true;
}

static bool parse_action_kind(const std::string& s, ActionKind& out) {
    const std::string u = upper(trim(s));
    if (u == "F" || u == "FOLD")  { out = AK_FOLD;  return true; }
    if (u == "X" || u == "CHECK") { out = AK_CHECK; return true; }
    if (u == "C" || u == "CALL")  { out = AK_CALL;  return true; }
    if (u == "B" || u == "BET")   { out = AK_BET;   return true; }
    if (u == "R" || u == "RAISE") { out = AK_RAISE; return true; }
    return false;
}

static const char* kind_name(ActionKind k) {
    switch (k) {
        case AK_FOLD:  return "Fold";
        case AK_CHECK: return "Check";
        case AK_CALL:  return "Call";
        case AK_BET:   return "Bet";
        case AK_RAISE: return "Raise";
    }
    return "?";
}

// A lock, stored in a form that survives a tree rebuild: addressed by node path
// and by action KIND rather than by node id and action index.
struct LockSpec {
    std::string                                 path;
    std::vector<int>                            hands;
    std::vector<std::pair<ActionKind, double>>  mix;
};

// =============================================================================
class Console {
public:
    Console() {
        for (int p = 0; p < 2; ++p)
            for (int h = 0; h < NH; ++h) range_[p][h] = 1.0;
        rebuild(true);
    }

    int run(std::istream& in, bool interactive) {
        if (interactive) banner();
        std::string line;
        while (true) {
            if (interactive) {
                std::printf("\n%s> ", tree_.nodes[cur_].path.c_str());
                std::fflush(stdout);
            }
            if (!std::getline(in, line)) break;
            // Editors love saving UTF-8 with a BOM, and PowerShell prepends one
            // when piping; either way it would poison the first command.
            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF)
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (!interactive) std::printf("\n%s> %s\n",
                                          tree_.nodes[cur_].path.c_str(), line.c_str());
            if (!dispatch(line)) break;   // quit
        }
        return 0;
    }

private:
    TreeConfig                  tc_;
    GameTree                    tree_;
    std::array<HandVec, 2>      range_{};
    std::vector<LockSpec>       locks_;
    std::unique_ptr<DCFRSolver> S_;
    int                         cur_    = 0;
    int                         iters_  = 1000;
    bool                        solved_ = false;

    // ---------------------------------------------------------------------
    static void banner() {
        std::printf(
"#############################################################################\n"
"#   DCFR RIVER SOLVER  --  interactive console                              #\n"
"#   `help` for commands, `solve` to run, `quit` to exit.                    #\n"
"#############################################################################\n");
    }

    void err(const std::string& m) const { std::printf("  ! %s\n", m.c_str()); }
    void ok(const std::string& m)  const { std::printf("  %s\n", m.c_str()); }

    bool need_solution() const {
        if (solved_ && S_) return true;
        std::printf("  ! no solution yet -- run `solve` first\n");
        return false;
    }

    bool need_decision(int nid) const {
        if (tree_.nodes[nid].type == NT_DECISION) return true;
        std::printf("  ! %s is a terminal node, nothing to show there\n",
                    tree_.nodes[nid].path.c_str());
        return false;
    }

    // ---------------------------------------------------------------------
    //  Tree (re)construction. Node ids die here, so the cwd and every lock are
    //  re-resolved by path afterwards.
    // ---------------------------------------------------------------------
    bool rebuild(bool quiet) {
        // Guard the branching factor before the builder aborts on it.
        const size_t widest_check = 1 + tc_.bet_fracs.size();
        const size_t widest_facing = 2 + tc_.raise_fracs.size();
        const size_t widest = std::max(widest_check, widest_facing);
        if (widest > static_cast<size_t>(cfg::MAX_ACTIONS)) {
            err("too many sizings: that would need " + std::to_string(widest) +
                " actions at a node, MAX_ACTIONS is " +
                std::to_string(cfg::MAX_ACTIONS));
            return false;
        }

        const std::string old_path =
            tree_.nodes.empty() ? std::string("R") : tree_.nodes[cur_].path;

        // Drop the solver first: it holds a reference into the tree we are
        // about to replace, and its node ids are about to become meaningless.
        S_.reset();
        solved_ = false;
        tree_ = TreeBuilder(tc_).build();

        const int back = tree_.find_path(old_path);
        cur_ = (back >= 0) ? back : tree_.root;

        // Drop locks whose node no longer exists under the new geometry.
        size_t dropped = 0;
        std::vector<LockSpec> keep;
        for (const LockSpec& L : locks_) {
            if (tree_.find_path(L.path) >= 0) keep.push_back(L);
            else ++dropped;
        }
        locks_.swap(keep);

        if (!quiet) {
            std::printf("  tree rebuilt: %d nodes, %d doubles per buffer\n",
                        static_cast<int>(tree_.nodes.size()), tree_.mem_size);
            if (back < 0 && old_path != "R")
                std::printf("  node %s no longer exists, moved to root\n",
                            old_path.c_str());
        }
        if (dropped)
            std::printf("  %d lock(s) dropped: their node does not exist any more\n",
                        static_cast<int>(dropped));
        return true;
    }

    // ---------------------------------------------------------------------
    void do_solve(int iterations) {
        GameTree& t = tree_;
        S_ = std::unique_ptr<DCFRSolver>(new DCFRSolver(t, range_));

        // Locks must be applied to the fresh solver before any iteration runs.
        size_t applied = 0;
        for (const LockSpec& L : locks_) {
            const int nid = t.find_path(L.path);
            if (nid < 0) continue;
            if (S_->lock_hands_kind(nid, L.hands, L.mix)) ++applied;
            else err("could not apply lock at " + L.path +
                     " (action not available at that node)");
        }
        if (applied)
            std::printf("  %d nodelock(s) applied\n", static_cast<int>(applied));

        const int every = std::max(1, iterations / 10);
        S_->run(iterations, every);
        solved_ = true;

        const double ev0 = S_->node_ev(tree_.root);
        std::printf("\n  Game value: OOP %.4f   IP %.4f   (pot %.1f)\n",
                    ev0, cfg::POT0 - ev0, cfg::POT0);
    }

    // ---------------------------------------------------------------------
    bool dispatch(const std::string& line) {
        const std::vector<std::string> tk = tokenize(line);
        if (tk.empty()) return true;
        const std::string cmd = lower(tk[0]);

        if (cmd == "quit" || cmd == "exit" || cmd == "q")   return false;
        if (cmd == "help" || cmd == "?")   { cmd_help();        return true; }
        if (cmd == "show" || cmd == "config") { cmd_show();     return true; }
        if (cmd == "set")                  { cmd_set(tk);       return true; }
        if (cmd == "range")                { cmd_range(tk);     return true; }
        if (cmd == "build")                { rebuild(false);    return true; }
        if (cmd == "tree")                 { cmd_tree();        return true; }
        if (cmd == "solve")                { cmd_solve(tk);     return true; }
        if (cmd == "iterate" || cmd == "it") { cmd_iterate(tk); return true; }
        if (cmd == "ls")                   { cmd_ls();          return true; }
        if (cmd == "cd")                   { cmd_cd(tk);        return true; }
        if (cmd == "pwd")                  { cmd_pwd();         return true; }
        if (cmd == "freq")                 { cmd_freq();        return true; }
        if (cmd == "strategy" || cmd == "s") { cmd_strategy();  return true; }
        if (cmd == "buckets" || cmd == "b") { cmd_buckets();    return true; }
        if (cmd == "ev")                   { cmd_ev();          return true; }
        if (cmd == "lock")                 { cmd_lock(tk);      return true; }
        if (cmd == "unlock")               { cmd_unlock();      return true; }
        if (cmd == "locks")                { cmd_locks();       return true; }
        if (cmd == "report")               { cmd_report(tk);    return true; }
        if (cmd == "csv")                  { cmd_csv(tk);       return true; }

        err("unknown command '" + tk[0] + "'  (try `help`)");
        return true;
    }

    // ---------------------------------------------------------------------
    void cmd_help() const {
        std::printf(
"\n  SPOT\n"
"    show                       current configuration, ranges, locks\n"
"    set pot <x>                dead money in the pot        (rebuilds tree)\n"
"    set stack <x>              effective stack per player   (rebuilds tree)\n"
"    set bets <f,f,...>         bet sizings as fraction of pot     e.g. 0.33,0.75\n"
"    set raises <f,f,...>       raise sizings as fraction of pot after call\n"
"    set maxraises <n>          betting rounds allowed (a bet counts as one)\n"
"    set iters <n>              default iteration count for `solve`\n"
"    set alpha|beta|gamma <x>   DCFR discount hyper-parameters\n"
"    set buckets <n,v,b>        lowest card index of NUTS, VALUE, BLUFF-CATCHER\n"
"    range oop|ip uniform       reset a range to all 1.0\n"
"    range oop|ip <SEL:w,...>   set weights, e.g. `range ip AIR:0,A:1,2-5:0.5`\n"
"    build                      force a tree rebuild\n"
"\n  SOLVING\n"
"    solve [n]                  fresh solve, n iterations (default `set iters`)\n"
"    iterate <n>                n more iterations on top of the current solution\n"
"\n  NAVIGATION\n"
"    tree                       print the whole betting tree\n"
"    ls                         actions and children of the current node\n"
"    cd <code|path|..|/>        move, e.g. `cd B15`, `cd R/X/B15`, `cd ..`\n"
"    pwd                        where you are\n"
"\n  READING THE SOLUTION\n"
"    freq                       action frequencies at every decision node\n"
"    strategy                   per-hand frequencies and EVs at current node\n"
"    buckets                    the same aggregated by hand type\n"
"    ev                         EV of Bet vs Check by hand type\n"
"    report [file]              full report to screen or to a file\n"
"    csv <file>                 current node's per-hand table as CSV\n"
"\n  NODELOCKING\n"
"    lock <SEL> <act=p,...>     freeze hands at the current node,\n"
"                               e.g. `lock AIR C=1`, `lock 2-5 F=0.5,C=0.5`\n"
"    unlock                     release the locks at the current node\n"
"    locks                      list every lock\n"
"    Selectors: AIR VALUE NUTS BC, a rank (A, T, 7), a range (2-5), all.\n"
"    Actions:   F/fold X/check C/call B/bet R/raise\n"
"    A lock only takes effect on the next `solve`.\n"
"\n  quit\n");
    }

    // ---------------------------------------------------------------------
    void cmd_show() const {
        std::printf("\n  SPOT\n");
        std::printf("    pot            %.2f\n", cfg::POT0);
        std::printf("    stack          %.2f\n", cfg::STACK);
        std::printf("    bet sizings    ");
        for (double f : tc_.bet_fracs) std::printf("%g ", f);
        std::printf("(x pot)\n    raise sizings  ");
        for (double f : tc_.raise_fracs) std::printf("%g ", f);
        std::printf("(x pot after call)\n");
        std::printf("    max raises     %d\n", tc_.max_raises);
        std::printf("    iterations     %d   (done on current solution: %d)\n",
                    iters_, S_ ? S_->iterations_done() : 0);
        std::printf("    DCFR           alpha %.2f  beta %.2f  gamma %.2f\n",
                    cfg::DCFR_ALPHA, cfg::DCFR_BETA, cfg::DCFR_GAMMA);

        std::printf("\n  BUCKETS\n");
        for (int t = HT_COUNT - 1; t >= 0; --t) {
            std::printf("    %-14s ", HT_NAME[t]);
            for (int h = NH - 1; h >= 0; --h)
                if (classify(h) == static_cast<HandType>(t))
                    std::printf("%s ", RANK_NAME[h]);
            std::printf("\n");
        }

        std::printf("\n  RANGES\n");
        for (int p = 0; p < 2; ++p) {
            std::printf("    %-4s", p == 0 ? "OOP" : "IP");
            for (int h = 0; h < NH; ++h)
                std::printf(" %s:%.2f", RANK_NAME[h], range_[p][h]);
            std::printf("\n");
        }

        std::printf("\n  TREE   %d nodes, %d doubles per buffer%s\n",
                    static_cast<int>(tree_.nodes.size()), tree_.mem_size,
                    solved_ ? "" : "   (not solved yet)");
        std::printf("  NODE   %s\n", tree_.nodes[cur_].path.c_str());
        if (!locks_.empty()) {
            std::printf("\n  LOCKS\n");
            print_locks();
        }
    }

    void print_locks() const {
        for (const LockSpec& L : locks_) {
            std::printf("    %-16s ", L.path.c_str());
            for (size_t i = 0; i < L.hands.size(); ++i)
                std::printf("%s%s", RANK_NAME[L.hands[i]],
                            i + 1 < L.hands.size() ? "," : "");
            std::printf("  ->  ");
            for (size_t i = 0; i < L.mix.size(); ++i)
                std::printf("%s=%.2f%s", kind_name(L.mix[i].first), L.mix[i].second,
                            i + 1 < L.mix.size() ? " " : "");
            std::printf("\n");
        }
    }

    // ---------------------------------------------------------------------
    void cmd_set(const std::vector<std::string>& tk) {
        if (tk.size() < 3) { err("usage: set <key> <value>"); return; }
        const std::string key = lower(tk[1]);
        const std::string val = tk[2];
        double d;
        int i;

        if (key == "pot") {
            if (!parse_double(val, d) || d <= 0.0) { err("pot must be > 0"); return; }
            cfg::POT0 = d; rebuild(false);
        } else if (key == "stack") {
            if (!parse_double(val, d) || d <= 0.0) { err("stack must be > 0"); return; }
            cfg::STACK = d; rebuild(false);
        } else if (key == "bets" || key == "raises") {
            std::vector<double> fr;
            for (const std::string& s : split(val, ',')) {
                if (s.empty()) continue;
                if (!parse_double(s, d) || d <= 0.0) {
                    err("sizings must be positive numbers, got '" + s + "'");
                    return;
                }
                fr.push_back(d);
            }
            if (fr.empty()) { err("need at least one sizing"); return; }
            std::vector<double> back = (key == "bets") ? tc_.bet_fracs : tc_.raise_fracs;
            if (key == "bets") tc_.bet_fracs = fr; else tc_.raise_fracs = fr;
            if (!rebuild(false)) {   // too wide: put the old sizings back
                if (key == "bets") tc_.bet_fracs = back; else tc_.raise_fracs = back;
                rebuild(true);
            }
        } else if (key == "maxraises") {
            if (!parse_int(val, i) || i < 0) { err("maxraises must be >= 0"); return; }
            tc_.max_raises = i; cfg::MAX_RAISES = i; rebuild(false);
        } else if (key == "iters" || key == "iterations") {
            if (!parse_int(val, i) || i <= 0) { err("iters must be > 0"); return; }
            iters_ = i; ok("iterations = " + std::to_string(iters_));
        } else if (key == "alpha" || key == "beta" || key == "gamma") {
            if (!parse_double(val, d)) { err("not a number: " + val); return; }
            if (key == "alpha") cfg::DCFR_ALPHA = d;
            else if (key == "beta") cfg::DCFR_BETA = d;
            else cfg::DCFR_GAMMA = d;
            solved_ = false;
            ok("DCFR " + key + " = " + val + "  (re-solve to apply)");
        } else if (key == "buckets") {
            const std::vector<std::string> p = split(val, ',');
            int a, b, c;
            if (p.size() != 3 || !parse_int(p[0], a) || !parse_int(p[1], b) ||
                !parse_int(p[2], c)) {
                err("usage: set buckets <nuts>,<value>,<bluffcatcher>   e.g. 11,8,4");
                return;
            }
            if (!(a > b && b > c && c >= 1 && a <= NH - 1)) {
                err("need NH-1 >= nuts > value > bluffcatcher >= 1");
                return;
            }
            cfg::TH_NUTS = a; cfg::TH_VALUE = b; cfg::TH_BC = c;
            ok("buckets updated");
        } else {
            err("unknown key '" + tk[1] + "'  (see `help`)");
        }
    }

    // ---------------------------------------------------------------------
    void cmd_range(const std::vector<std::string>& tk) {
        if (tk.size() < 3) {
            err("usage: range oop|ip uniform | <SEL:weight,...>");
            return;
        }
        const std::string who = lower(tk[1]);
        int p;
        if (who == "oop" || who == "0") p = 0;
        else if (who == "ip" || who == "1") p = 1;
        else { err("first argument must be oop or ip"); return; }

        // Everything after the player name, so commas may carry spaces.
        std::string spec;
        for (size_t i = 2; i < tk.size(); ++i) spec += tk[i];

        const HandVec backup = range_[p];

        if (lower(spec) == "uniform") {
            for (int h = 0; h < NH; ++h) range_[p][h] = 1.0;
        } else {
            for (const std::string& item : split(spec, ',')) {
                if (item.empty()) continue;
                const size_t colon = item.find(':');
                if (colon == std::string::npos) {
                    err("expected SEL:weight, got '" + item + "'");
                    range_[p] = backup;
                    return;
                }
                double w;
                if (!parse_double(item.substr(colon + 1), w) || w < 0.0) {
                    err("weight must be >= 0 in '" + item + "'");
                    range_[p] = backup;
                    return;
                }
                std::vector<int> hands;
                std::string e;
                if (!parse_hands(item.substr(0, colon), hands, e)) {
                    err(e);
                    range_[p] = backup;
                    return;
                }
                for (int h : hands) range_[p][h] = w;
            }
        }

        double tot = 0.0;
        for (int h = 0; h < NH; ++h) tot += range_[p][h];
        if (tot < 1e-9) {
            err("that would empty the range completely -- reverted");
            range_[p] = backup;
            return;
        }
        solved_ = false;
        std::printf("  %s range set (total weight %.2f) -- re-solve to apply\n",
                    p == 0 ? "OOP" : "IP", tot);
    }

    // ---------------------------------------------------------------------
    void cmd_tree() { print_tree(tree_, tree_.root, 1); }

    void cmd_solve(const std::vector<std::string>& tk) {
        int n = iters_;
        if (tk.size() > 1 && (!parse_int(tk[1], n) || n <= 0)) {
            err("usage: solve [iterations]");
            return;
        }
        do_solve(n);
    }

    void cmd_iterate(const std::vector<std::string>& tk) {
        if (!S_) { err("nothing to continue -- run `solve` first"); return; }
        int n = iters_;
        if (tk.size() > 1 && (!parse_int(tk[1], n) || n <= 0)) {
            err("usage: iterate <iterations>");
            return;
        }
        S_->run(n, std::max(1, n / 10));
        solved_ = true;
        const double ev0 = S_->node_ev(tree_.root);
        std::printf("\n  Game value: OOP %.4f   IP %.4f   (%d iterations total)\n",
                    ev0, cfg::POT0 - ev0, S_->iterations_done());
    }

    // ---------------------------------------------------------------------
    void cmd_pwd() const {
        const Node& n = tree_.nodes[cur_];
        std::printf("  %s   ", n.path.c_str());
        if (n.type == NT_DECISION)
            std::printf("%s to act, pot %.1f, %d actions\n",
                        n.player == 0 ? "OOP" : "IP", n.pot, n.num_actions);
        else if (n.type == NT_SHOWDOWN)
            std::printf("showdown, pot %.1f\n", n.pot);
        else
            std::printf("%s folds, pot %.1f\n", n.player == 0 ? "OOP" : "IP", n.pot);
    }

    void cmd_ls() const {
        const Node& n = tree_.nodes[cur_];
        if (n.type != NT_DECISION) { std::printf("  (terminal node)\n"); return; }
        std::printf("  %s -- %s to act, pot %.1f\n", n.path.c_str(),
                    n.player == 0 ? "OOP" : "IP", n.pot);
        for (int a = 0; a < n.num_actions; ++a) {
            const int    c  = tree_.child(n, a);
            const Node&  cn = tree_.nodes[c];
            const char*  ty = cn.type == NT_DECISION ? "->"
                            : cn.type == NT_SHOWDOWN ? "showdown" : "fold";
            std::printf("    %-5s %-14s %-9s %s\n", tree_.act(n, a).code.c_str(),
                        tree_.act(n, a).label.c_str(), ty,
                        cn.type == NT_DECISION ? cn.path.c_str() : "");
        }
    }

    void cmd_cd(const std::vector<std::string>& tk) {
        if (tk.size() < 2) { err("usage: cd <code|path|..|/>"); return; }
        const std::string arg = tk[1];

        if (arg == "/" || lower(arg) == "root") { cur_ = tree_.root; cmd_pwd(); return; }
        if (arg == "..") {
            const std::string p = GameTree::parent_path(tree_.nodes[cur_].path);
            const int nid = tree_.find_path(p);
            if (nid < 0) { err("already at the root"); return; }
            cur_ = nid; cmd_pwd(); return;
        }
        // Absolute path?
        int nid = tree_.find_path(arg);
        if (nid < 0) {
            // Relative: an action code at the current node.
            const Node& n = tree_.nodes[cur_];
            if (n.type == NT_DECISION) {
                for (int a = 0; a < n.num_actions; ++a) {
                    if (lower(tree_.act(n, a).code) == lower(arg)) {
                        nid = tree_.child(n, a);
                        break;
                    }
                }
            }
        }
        if (nid < 0) { err("no such node or action: '" + arg + "'"); return; }
        cur_ = nid;
        cmd_pwd();
    }

    // ---------------------------------------------------------------------
    void cmd_freq() {
        if (!need_solution()) return;
        report_frequencies(*S_);
    }

    void cmd_strategy() {
        if (!need_solution() || !need_decision(cur_)) return;
        report_hand_table(*S_, cur_, "Per-hand strategy and EV");
    }

    void cmd_buckets() {
        if (!need_solution() || !need_decision(cur_)) return;
        report_buckets(*S_, cur_, "By hand type");
    }

    void cmd_ev() {
        if (!need_solution() || !need_decision(cur_)) return;
        report_bet_vs_check(*S_, cur_);
    }

    // ---------------------------------------------------------------------
    void cmd_lock(const std::vector<std::string>& tk) {
        if (!need_decision(cur_)) return;
        if (tk.size() < 3) {
            err("usage: lock <hands> <action=prob,...>   e.g. lock AIR C=1");
            return;
        }
        std::vector<int> hands;
        std::string e;
        if (!parse_hands(tk[1], hands, e)) { err(e); return; }

        std::string mixspec;
        for (size_t i = 2; i < tk.size(); ++i) mixspec += tk[i];

        std::vector<std::pair<ActionKind, double>> mix;
        double total = 0.0;
        for (const std::string& item : split(mixspec, ',')) {
            if (item.empty()) continue;
            const size_t eq = item.find('=');
            if (eq == std::string::npos) {
                err("expected action=prob, got '" + item + "'");
                return;
            }
            ActionKind k;
            double p;
            if (!parse_action_kind(item.substr(0, eq), k)) {
                err("unknown action '" + item.substr(0, eq) + "'");
                return;
            }
            if (!parse_double(item.substr(eq + 1), p) || p < 0.0) {
                err("probability must be >= 0 in '" + item + "'");
                return;
            }
            mix.push_back(std::make_pair(k, p));
            total += p;
        }
        if (mix.empty() || total <= 1e-12) { err("empty action mix"); return; }

        // Reject up front if the node simply has no such action.
        const Node& n = tree_.nodes[cur_];
        for (const auto& pr : mix) {
            if (tree_.action_index(n, pr.first) < 0) {
                err(std::string("node ") + n.path + " has no " +
                    kind_name(pr.first) + " action");
                return;
            }
        }

        const std::string path = n.path;
        locks_.erase(std::remove_if(locks_.begin(), locks_.end(),
                                    [&](const LockSpec& L) { return L.path == path; }),
                     locks_.end());
        LockSpec L;
        L.path  = path;
        L.hands = hands;
        L.mix   = mix;
        locks_.push_back(L);

        std::printf("  locked %d hand(s) at %s  ->  ",
                    static_cast<int>(hands.size()), path.c_str());
        for (size_t i = 0; i < mix.size(); ++i)
            std::printf("%s=%.2f%s", kind_name(mix[i].first), mix[i].second / total,
                        i + 1 < mix.size() ? " " : "");
        std::printf("\n  run `solve` to apply it\n");
        solved_ = false;
    }

    void cmd_unlock() {
        const std::string path = tree_.nodes[cur_].path;
        const size_t before = locks_.size();
        locks_.erase(std::remove_if(locks_.begin(), locks_.end(),
                                    [&](const LockSpec& L) { return L.path == path; }),
                     locks_.end());
        if (locks_.size() == before) { err("no lock at " + path); return; }
        if (S_) S_->clear_locks(cur_);
        solved_ = false;
        ok("lock removed at " + path + " -- run `solve`");
    }

    void cmd_locks() const {
        if (locks_.empty()) { std::printf("  (no locks)\n"); return; }
        print_locks();
    }

    // ---------------------------------------------------------------------
    void cmd_report(const std::vector<std::string>& tk) {
        if (!need_solution()) return;
        FILE* f = nullptr;
        if (tk.size() > 1) {
            f = std::fopen(tk[1].c_str(), "w");
            if (!f) { err("cannot write to '" + tk[1] + "'"); return; }
            g_out = f;
        }

        hr("BETTING TREE");
        print_tree(tree_, tree_.root, 1);
        hr("ACTION FREQUENCIES");
        report_frequencies(*S_);
        hr("BY HAND TYPE");
        for (int nid = 0; nid < static_cast<int>(tree_.nodes.size()); ++nid)
            if (tree_.nodes[nid].type == NT_DECISION)
                report_buckets(*S_, nid, "Node");
        hr("PER-HAND STRATEGY AND EV");
        for (int nid = 0; nid < static_cast<int>(tree_.nodes.size()); ++nid)
            if (tree_.nodes[nid].type == NT_DECISION)
                report_hand_table(*S_, nid, "Node");
        hr("BET vs CHECK BY HAND TYPE (root)");
        report_bet_vs_check(*S_, tree_.root);
        report_game_value(*S_);

        if (f) {
            g_out = stdout;
            std::fclose(f);
            ok("report written to " + tk[1]);
        }
    }

    void cmd_csv(const std::vector<std::string>& tk) {
        if (!need_solution() || !need_decision(cur_)) return;
        if (tk.size() < 2) { err("usage: csv <file>"); return; }
        FILE* f = std::fopen(tk[1].c_str(), "w");
        if (!f) { err("cannot write to '" + tk[1] + "'"); return; }

        const Node& n = tree_.nodes[cur_];
        std::fprintf(f, "node,%s\nplayer,%s\npot,%.4f\n\n", n.path.c_str(),
                     n.player == 0 ? "OOP" : "IP", n.pot);
        std::fprintf(f, "hand,type,locked,combo_weight");
        for (int a = 0; a < n.num_actions; ++a)
            std::fprintf(f, ",freq_%s,ev_%s", tree_.act(n, a).code.c_str(),
                         tree_.act(n, a).code.c_str());
        std::fprintf(f, ",ev_node\n");

        for (int h = NH - 1; h >= 0; --h) {
            std::fprintf(f, "%s,%s,%d,%.6f", RANK_NAME[h], HT_NAME[classify(h)],
                         S_->is_hand_locked(cur_, h) ? 1 : 0,
                         S_->combo_weight(cur_, h));
            for (int a = 0; a < n.num_actions; ++a)
                std::fprintf(f, ",%.6f,%.6f", S_->avg_strategy(cur_, h, a),
                             S_->hand_action_ev(cur_, h, a));
            std::fprintf(f, ",%.6f\n", S_->hand_node_ev(cur_, h));
        }
        std::fclose(f);
        ok("wrote " + tk[1]);
    }
};

// =============================================================================
//  10. ENTRY POINT
// =============================================================================
static void usage() {
    std::printf(
"DCFR River Solver\n"
"\n"
"  solver                 interactive console (default)\n"
"  solver --demo          the scripted GTO-vs-nodelock showcase\n"
"  solver --script FILE   run console commands from FILE, then exit\n"
"  solver --help\n"
"\n"
"Commands can also be piped:   echo solve | solver\n");
}

int main(int argc, char** argv) {
    // A REPL must not sit on a full stdout buffer: when the output is a pipe or
    // a file, prompts and progress would only appear at exit.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string a = lower(args[i]);
        if (a == "--help" || a == "-h") { usage(); return 0; }
        if (a == "--demo")              { return run_demo(); }
        if (a == "--script") {
            if (i + 1 >= args.size()) {
                std::printf("--script needs a file name\n");
                return 2;
            }
            std::ifstream f(args[i + 1]);
            if (!f) {
                std::printf("cannot open script '%s'\n", args[i + 1].c_str());
                return 2;
            }
            Console c;
            return c.run(f, false);
        }
        std::printf("unknown option '%s'\n\n", args[i].c_str());
        usage();
        return 2;
    }

    Console c;
    return c.run(std::cin, true);
}
