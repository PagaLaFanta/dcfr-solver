#pragma once
#include <cctype>
// =============================================================================
//  Multi-street betting tree.
//
//  The betting tree of a street is IDENTICAL for every runout that reaches it --
//  same pot, same sizings, same shape -- so it is built once as a template and
//  the information-set memory is indexed by (template node, runout instance).
//  Building 2352 copies of the river subtree instead would cost hundreds of
//  megabytes in node metadata alone, before a single regret is stored.
//
//  A "context" is one betting round reached by one line of continuations:
//    ctx 0            the starting street
//    ctx 1..          the next street, one per continuation of ctx 0
//    ...
//  Instances inside a context are runouts:
//    instance(child) = instance(parent) * deckN + card_slot
//  so the instance index of any node follows straight from the cards dealt.
// =============================================================================

#include "msg.hpp"
#include "config.hpp"
#include "deal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum NodeType  { NT_DECISION = 0, NT_CONT, NT_SHOWDOWN, NT_FOLD };
enum ActionKind { AK_FOLD = 0, AK_CHECK, AK_CALL, AK_BET, AK_RAISE };

inline const char* kind_name(ActionKind k) {
    switch (k) {
        case AK_FOLD:  return "Fold";
        case AK_CHECK: return "Check";
        case AK_CALL:  return "Call";
        case AK_BET:   return "Bet";
        case AK_RAISE: return "Raise";
    }
    return "?";
}

// The one-letter form `parse_action_kind` reads back, so a mix can be written
// to a config file and returned unchanged.
// Vive aqui, con ActionKind, porque el solver tambien lo necesita y se compila
// antes que session.hpp. Sin `trim`/`upper`, que viven alla: el recorte y las
// mayusculas se hacen a mano, que son cuatro lineas.
inline bool parse_action_kind(const std::string& s, ActionKind& out) {
    std::string u;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        u += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (u == "F" || u == "FOLD")  { out = AK_FOLD;  return true; }
    if (u == "X" || u == "CHECK") { out = AK_CHECK; return true; }
    if (u == "C" || u == "CALL")  { out = AK_CALL;  return true; }
    if (u == "B" || u == "BET")   { out = AK_BET;   return true; }
    if (u == "R" || u == "RAISE") { out = AK_RAISE; return true; }
    return false;
}

inline const char* kind_code(ActionKind k) {
    switch (k) {
        case AK_FOLD:  return "F";
        case AK_CHECK: return "X";
        case AK_CALL:  return "C";
        case AK_BET:   return "B";
        case AK_RAISE: return "R";
    }
    return "?";
}

struct ActionInfo {
    ActionKind  kind;
    double      to_amount;   // total chips invested in this round after the action
    std::string code;
    std::string label;
};

struct Node {
    NodeType type          = NT_DECISION;
    int      player        = -1;   // decision: actor.  NT_FOLD: the folding player.
    int      num_actions   = 0;
    int      action_offset = -1;
    int      child_offset  = -1;
    int      block_offset  = -1;   // element offset of this node's block in the round
    int      dec_slot      = -1;   // ordinal among the round's decision nodes
    int      cont_id       = -1;   // NT_CONT: which continuation
    int      depth         = 0;
    double   terminal_W    = 0.0;
    double   rake          = 0.0;   // chips the house takes from this terminal
    double   pot           = 0.0;
    std::string path;              // local to the round, e.g. "R/X/B7"

    // ---- nodelocking, POR RUNOUT ------------------------------------------
    //
    // Un nodo del turn se decide UNA vez, antes de que caiga el river, asi que
    // ahi el runout no significa nada y el lock vale para el nodo entero. Pero
    // un nodo del river existe una vez POR CARTA, y bloquear "que sobreapueste
    // en el 3s" no es lo mismo que bloquear "que sobreapueste en cualquier
    // river": son dos preguntas distintas y en un turn de 48 cartas se llevan
    // tres fichas sobre un bote de 100 (medido contra la referencia).
    //
    // Antes esto vivia en la plantilla y salia en los 48 runouts, y encima la
    // interfaz te ensenaba la carta que habias elegido antes de ignorarla.
    //
    // `blk` guarda los bloques distintos y `slot` dice, por instancia, cual le
    // toca (-1 si esa instancia no esta bloqueada). Con `slot` VACIO el bloque
    // 0 vale para todas: es la semantica vieja, y hace falta para poder leer
    // las configuraciones guardadas antes de este cambio.
    struct LockBlock {
        std::vector<uint8_t> hand;       // por combo: bloqueado o no
        std::vector<double>  strategy;   // num_actions * combos
    };
    bool                   is_locked = false;
    std::vector<LockBlock> lock_blk;
    std::vector<int>       lock_slot;

    // El bloque que le toca a una instancia, o nullptr si ahi no hay lock.
    // Se resuelve una vez por visita al nodo y el bucle interior usa el
    // puntero, asi que cuesta lo mismo que costaba el bool de antes.
    const LockBlock* lock_for(long long inst) const {
        if (!is_locked || lock_blk.empty()) return nullptr;
        if (lock_slot.empty()) return &lock_blk[0];
        if (inst < 0 || inst >= static_cast<long long>(lock_slot.size())) return nullptr;
        const int s = lock_slot[static_cast<size_t>(inst)];
        return (s >= 0) ? &lock_blk[static_cast<size_t>(s)] : nullptr;
    }
};

struct BetTree {
    std::vector<Node>       nodes;
    std::vector<ActionInfo> actions;
    std::vector<int>        children;
    std::vector<double>     cont_inv;   // per continuation: chips each player put in
    std::vector<int>        cont_aggr;  // per continuation: who bet/raised last (-1 none)
    int root     = 0;
    int blocks   = 0;   // total action-blocks (sum of num_actions over decisions)
    int ndec     = 0;   // decision nodes, which is how many discount stamps a round needs
    int num_cont = 0;

    const ActionInfo& act(const Node& n, int a) const { return actions[n.action_offset + a]; }
    int child(const Node& n, int a) const             { return children[n.child_offset + a]; }
    int action_index(const Node& n, ActionKind k) const {
        for (int a = 0; a < n.num_actions; ++a)
            if (act(n, a).kind == k) return a;
        return -1;
    }
};

struct RoundCtx {
    int         street      = ST_RIVER;
    int         parent      = -1;
    int         parent_cont = -1;
    double      pot         = 0.0;   // pot at the start of the round
    double      spent       = 0.0;   // chips each player put in on earlier streets
    std::string label       = "R";   // the line of continuations that gets here
    BetTree     tree;
    long long   instances   = 1;
    long long   stride      = 0;     // elements one runout instance of this round takes
    long long   mem_offset  = 0;
    long long   stamp_offset = 0;    // same, counted in decision nodes
    std::vector<int> cont_ctx;       // continuation -> child context, -1 on the last street
};

struct GameTree {
    std::vector<RoundCtx> ctx;
    long long mem_size   = 0;   // entries per buffer
    long long stamp_size = 0;   // decision-node instances
    int       nh         = 0;
    int       deckN      = 0;
    int       start      = ST_RIVER;
    int       max_depth  = 0;   // deepest recursion, for sizing scratch frames
    int       nlive[2]   = { 0, 0 };   // combos each player actually holds

    // The shape of the tree does not depend on the ranges, but the size of its
    // memory does: a node only ever stores regrets for the combos its owner can
    // actually hold. A board offers 1176 of them and a real range has a couple
    // of hundred, so laying the memory out over the live ones instead of all of
    // them is worth roughly a 6x cut. Called again whenever a range changes.
    void layout(int nl0, int nl1) {
        nlive[0] = nl0;
        nlive[1] = nl1;
        long long off = 0, soff = 0;
        for (RoundCtx& rc : ctx) {
            long long e = 0;
            int slot = 0;
            for (Node& n : rc.tree.nodes) {
                if (n.type != NT_DECISION) continue;
                n.block_offset = static_cast<int>(e);
                n.dec_slot     = slot++;
                e += static_cast<long long>(n.num_actions) * nlive[n.player];
            }
            rc.tree.ndec  = slot;
            rc.stride     = e;
            rc.mem_offset = off;
            rc.stamp_offset = soff;
            off  += rc.stride * rc.instances;
            soff += static_cast<long long>(slot) * rc.instances;
        }
        mem_size   = off;
        stamp_size = soff;
    }

    long long bytes_per_buffer() const {
        return mem_size * static_cast<long long>(sizeof(float));
    }
    // regret + strategy sum
    double total_gb() const {
        return 2.0 * static_cast<double>(bytes_per_buffer()) / (1024.0 * 1024.0 * 1024.0);
    }
    int num_decision_nodes() const {
        int n = 0;
        for (const RoundCtx& c : ctx)
            for (const Node& nd : c.tree.nodes) if (nd.type == NT_DECISION) ++n;
        return n;
    }
    long long num_instanced_nodes() const {
        long long n = 0;
        for (const RoundCtx& c : ctx)
            for (const Node& nd : c.tree.nodes)
                if (nd.type == NT_DECISION) n += c.instances;
        return n;
    }
    int find_ctx(const std::string& label) const {
        for (size_t i = 0; i < ctx.size(); ++i) if (ctx[i].label == label) return static_cast<int>(i);
        return -1;
    }
    int find_node(int c, const std::string& path) const {
        const BetTree& t = ctx[static_cast<size_t>(c)].tree;
        for (size_t i = 0; i < t.nodes.size(); ++i) if (t.nodes[i].path == path) return static_cast<int>(i);
        return -1;
    }
};

// -----------------------------------------------------------------------------
// A raise size is read one of two ways: as a fraction of the pot after calling
// (0.5, 1.0, ...) or, with an "x" suffix, as a multiple of the bet being faced
// (2x = min-raise over a first bet, 3x, 4x, ...).
struct Sizing {
    double v    = 1.0;
    bool   xbet = false;
    // La subida MINIMA legal, que no es un multiplo: sobre una primera apuesta
    // es 2x, pero sobre una subida a 24 despues de una apuesta de 12 es 36 y no
    // 48. O sea que "el minimo" no se puede escribir con un numero que valga en
    // todos los niveles, y por eso tiene bandera propia en vez de ser un 1x.
    bool   minraise = false;
};

struct TreeConfig {
    // Indexed [street][player], street 0 flop / 1 turn / 2 river, player 0 OOP
    // / 1 IP. Per player because the two of them do not bet the same sizes and
    // never did -- one shared list was a simplification nobody asked for, and
    // it is not how any solver presents this.
    // 30% en flop, 70% en turn y river, para los dos. Se escriben como
    // fracciones porque es la escala de dentro; el usuario los teclea "30" y
    // "70", que es lo que dicen y lo que sale en pantalla.
    std::vector<double> bets[3][2] = { { { 0.30 }, { 0.30 } },
                                       { { 0.70 }, { 0.70 } },
                                       { { 0.70 }, { 0.70 } } };
    // Empty means the action does not exist. There used to be a separate
    // bet+raise cap per street, and it was the same silent contradiction we
    // keep finding elsewhere: a list of raise sizes could sit in the config,
    // printed by `show`, filled in on screen, and be ignored, because the cap
    // said one thing and the sizings said another. The list is now the only
    // thing that decides. Depth ends where the all-in threshold ends it, which
    // is where it was ending anyway -- past 3 levels the cap changed nothing at
    // 100bb, and past 10 nothing at 2000bb.
    // Por defecto, una subida a 3x la apuesta que tienes delante, en las tres
    // calles y para los dos. Antes no habia ninguna: un arbol recien abierto no
    // dejaba subir, que no es un juego de poker. Cuesta arbol -- el arbol por
    // defecto pasa a ser bastante mayor -- y esa es la contrapartida.
    std::vector<Sizing> raises[3][2] = {
        { { { 3.0, true } }, { { 3.0, true } } },
        { { { 3.0, true } }, { { 3.0, true } } },
        { { { 3.0, true } }, { { 3.0, true } } } };
    // OOP leading into the player who was aggressive on the PREVIOUS street.
    // Its own list, because it is its own decision -- you do not lead into the
    // raiser with the size you continuation-bet. Empty means it does not
    // happen, which is the same rule as everywhere else and is exactly what the
    // old on/off switch did when it was off. Never read on the street the solve
    // starts on: there is no previous street there, so nobody to lead into.
    std::vector<double> donks[3];
    bool                add_allin[3][2] = { { false, false },
                                            { false, false },
                                            { false, false } };
    // IP only, as is standard. It stops IP making the THIRD aggressive action of a
    // street -- the classic one being IP c-bets, OOP check-raises, IP 3-bets.
    // OOP has no such box there and does not have one here either.
    bool                no_3bet[3] = { false, false, false };
};

class TreeBuilder {
public:
    TreeBuilder(const TreeConfig& c, const Deal& d) : c_(c), d_(d) {}

    GameTree build() {
        g_ = GameTree();
        g_.nh    = d_.num();
        g_.deckN = d_.deckN();
        g_.start = d_.start;
        ok_ = true;

        // On the starting street there is no in-tree aggressor to lead into, so
        // OOP's opening bet is always available; the bet sizings govern it.
        add_ctx(d_.start, -1, -1, cfg::POT0, 0.0, "R", false);

        // Contexts are appended as they are discovered, so a plain sweep visits
        // parents before children.
        for (size_t i = 0; i < g_.ctx.size(); ++i) expand(static_cast<int>(i));

        // A worst-case layout, so tree size can be reported before anyone has
        // typed a range. Session re-lays it out over the live combos.
        g_.layout(g_.nh, g_.nh);

        // Deepest recursion: a child round starts one level below its parent's
        // deepest node (the chance node in between costs a frame too).
        const size_t NC = g_.ctx.size();
        std::vector<int> local(NC, 0), base(NC, 0);
        for (size_t i = 0; i < NC; ++i)
            for (const Node& n : g_.ctx[i].tree.nodes)
                local[i] = std::max(local[i], n.depth);
        int maxd = 0;
        for (size_t i = 0; i < NC; ++i) {   // parents always precede children
            const int par = g_.ctx[i].parent;
            base[i] = (par < 0) ? 0 : base[static_cast<size_t>(par)] + local[static_cast<size_t>(par)] + 2;
            maxd = std::max(maxd, base[i] + local[i]);
        }
        g_.max_depth = maxd;
        return g_;
    }

    bool ok() const { return ok_; }
    const std::string& error() const { return err_; }

private:
    TreeConfig  c_;
    const Deal& d_;
    GameTree    g_;
    bool        ok_ = true;
    std::string err_;

    static std::string num(double v) {
        char b[32];
        std::snprintf(b, sizeof(b), "%g", v);
        return std::string(b);
    }

    void add_ctx(int street, int parent, int cont, double pot, double spent,
                 const std::string& label, bool block_donk) {
        RoundCtx rc;
        rc.street      = street;
        rc.parent      = parent;
        rc.parent_cont = cont;
        rc.pot         = pot;
        rc.spent       = spent;
        rc.label       = label;
        // How many runouts this round is actually stored for. With suit
        // isomorphism that is the orbit count, not the raw card count.
        rc.instances   = d_.inst[street - d_.start];
        rc.tree        = build_round(street, pot, spent, block_donk);
        rc.cont_ctx.assign(static_cast<size_t>(rc.tree.num_cont), -1);
        g_.ctx.push_back(rc);
    }

    // Creates the next street's contexts for every continuation of ctx `ci`.
    // Everything needed is copied out first: add_ctx() pushes onto the same
    // vector, so any reference into it would dangle after a reallocation.
    void expand(int ci) {
        const size_t I = static_cast<size_t>(ci);
        if (g_.ctx[I].street >= ST_RIVER) return;
        const int                next  = g_.ctx[I].street + 1;
        const double             spent = g_.ctx[I].spent;
        const std::string        label = g_.ctx[I].label;
        const int                ncont = g_.ctx[I].tree.num_cont;
        const std::vector<double> inv  = g_.ctx[I].tree.cont_inv;
        const std::vector<int>    aggr = g_.ctx[I].tree.cont_aggr;
        std::vector<std::string> lbls;
        for (int k = 0; k < ncont; ++k) lbls.push_back(cont_label(g_.ctx[I].tree, k));

        for (int k = 0; k < ncont; ++k) {
            const double s = spent + inv[static_cast<size_t>(k)];
            const int child = static_cast<int>(g_.ctx.size());
            // A donk is OOP betting into the player who was last aggressive.
            // After a checked-through street nobody was, so leading is normal.
            const bool donk_spot = (aggr[static_cast<size_t>(k)] == 1);
            add_ctx(next, ci, k, cfg::POT0 + 2.0 * s, s,
                    label + "|" + lbls[static_cast<size_t>(k)], donk_spot);
            g_.ctx[I].cont_ctx[static_cast<size_t>(k)] = child;
        }
    }

    static std::string cont_label(const BetTree& t, int cont) {
        for (const Node& n : t.nodes)
            if (n.type == NT_CONT && n.cont_id == cont) return n.path;
        return "?";
    }

    // ---- one betting round ------------------------------------------------
    struct Succ {
        bool     terminal;
        NodeType ty;
        int      folder;
        double   W;
        double   ni0, ni1;
        int      nlevel;
        bool     nfacing;
        bool     nprev;
        int      naggr;
    };

    // `donk_spot` says OOP's opening action this round is a lead into the
    // previous street's aggressor, so it is priced from the donk list rather
    // than from OOP's bet list -- and does not happen at all when that list is
    // empty. It applies only to the round's first node; everything below it is
    // a normal continuation.
    BetTree build_round(int street, double pot0, double spent, bool donk_spot) {
        BetTree t;
        // Both already all-in: there is nothing to decide this street.
        //
        // The recursion below would still build it -- neither player can bet
        // with no chips behind, so each gets a lone "Check" and the two chain
        // into the showdown. Those are decision nodes with one action, which
        // carry no information and cost real memory and real traversal: on a
        // flop tree with pot-sized bets they were 24% of the instanced nodes
        // and 14% of the regret blocks. The reference solver goes straight to the runout,
        // and so does this now.
        if (cfg::STACK - spent <= 1e-9) {
            Node tn;
            tn.type       = (street == ST_RIVER) ? NT_SHOWDOWN : NT_CONT;
            tn.pot        = pot0;
            tn.terminal_W = pot0 * 0.5;
            tn.depth      = 0;
            tn.path       = "R";
            if (tn.type == NT_SHOWDOWN)
                tn.rake = cfg::rake_on_pot(2.0 * tn.terminal_W);
            else {
                tn.cont_id = t.num_cont++;
                t.cont_inv.push_back(0.0);
                t.cont_aggr.push_back(-1);   // nobody was aggressive here
            }
            t.nodes.push_back(tn);
            return t;
        }
        rec(t, street, pot0, spent, 0, 0.0, 0.0, 0, false, false, -1, donk_spot, 0, "R");
        // Only the count, for reporting. The real offsets come from
        // GameTree::layout(), which needs to know the ranges first.
        int blk = 0;
        for (const Node& n : t.nodes)
            if (n.type == NT_DECISION) blk += n.num_actions;
        t.blocks = blk;
        return t;
    }

    void add_sizing(BetTree& t, std::vector<ActionInfo>& acts, std::vector<Succ>& succ,
                    ActionKind kind, double to, int player, double inv0, double inv1,
                    int level) {
        (void)t;
        double ni[2] = { inv0, inv1 };
        ni[player] = to;
        const double from = (player == 0) ? inv0 : inv1;
        acts.push_back(ActionInfo{ kind, to,
            (kind == AK_BET ? "B" : "R") + num(to),
            (kind == AK_BET ? "Bet " + num(to - from) : "Raise to " + num(to)) });
        succ.push_back(Succ{ false, NT_DECISION, -1, 0.0, ni[0], ni[1], level + 1,
                             true, false, player });
    }

    // A sizing that commits more than the threshold of the starting effective
    // stack is worth more as a clean all-in than as its own branch, so it gets
    // promoted to one and dedup() then collapses it with any other sizing that
    // landed on the same number. `spent` is what went in on earlier streets, so
    // `spent + to` is the whole commitment for the hand -- the quantity every solver
    // measures, and the one a player means by "I am in for two thirds".
    // Fichas enteras.
    //
    // En una mesa no se apuesta media ficha. Un 25% del bote sobre 250 son 62,5
    // y eso no existe: la referencia construye 62, y hasta ahora nosotros
    // construiamos 62,5, con lo que los dos programas resolvian juegos
    // distintos y la comparacion al 25% no se podia ni hacer. Peor todavia para
    // quien lo usa: la estrategia que salia era para una partida que nadie
    // puede jugar.
    //
    // Se REDONDEA al entero mas cercano, no se trunca.
    //
    // Aqui me equivoque antes y conviene dejar escrito por que, porque la
    // evidencia que me llevo al error era correcta. Si le pasas a la referencia una
    // cantidad explicita con `add_line`, la trunca: 87.78 construye 87. Eso es
    // cierto y esta comprobado. Pero cuando la referencia construye el arbol desde
    // PORCENTAJES no hace eso, redondea, y es otro camino distinto:
    //
    //     IP turn, 66% de un bote de 116 = 76,56  ->  la referencia construye 77
    //     IP flop, 25% de un bote de 50  = 12,50  ->  la referencia construye 12
    //     OOP turn, 65% de un bote de 116 = 75,40 ->  la referencia construye 75
    //
    // O sea al mas cercano, con el medio hacia el par -- que es lo que hace
    // nearbyint con el modo por defecto. Truncando, esa apuesta del turn salia
    // 76 en vez de 77, los dos programas resolvian juegos distintos con el
    // mismo aspecto en casi todos los nodos, y el valor de juego se iba medio
    // punto del bote.
    //
    // Y redondear es ademas lo correcto de por si: truncar sesga TODAS las
    // apuestas hacia abajo, siempre en la misma direccion.
    //
    // Lo que NO se toca es el all-in. El tope es el stack que tu has puesto, y
    // ese numero es tuyo: si tienes 250,5 detras, el all-in son 250,5. Por eso
    // el truncado va sobre el tamano recien calculado y antes del tope, nunca
    // sobre el tope.
    static double whole_chips(double to) { return std::nearbyint(to); }

    double snap_allin(double to, double cap, double spent) const {
        const double behind = cap - to;
        if (behind <= 1e-9) return cap;
        if (spent + to > cfg::ALLIN_THRESH * cfg::STACK + 1e-9) return cap;
        return to;
    }

    // Sorted ascending, then collapsed: anything within MERGE_PCT of the sizing
    // above it is dropped. At 0 this is a plain exact dedup.
    void merge_sizings(std::vector<double>& v) const {
        std::sort(v.begin(), v.end());
        const double m = cfg::MERGE_PCT;
        std::vector<double> o;
        for (double x : v) {
            if (!o.empty() && x - o.back() <= m * x + 1e-9) continue;
            o.push_back(x);
        }
        v.swap(o);
    }

    // The one bound on how deep a round may go. Nothing the user sets: it only
    // fires for a sizing whose raise chain does not converge, and it says so
    // rather than recursing until the machine gives out.
    bool room_to_raise(int level) {
        if (level < cfg::MAX_RAISE_LEVELS) return true;
        if (ok_) {
            ok_  = false;
            err_ = M("las subidas no paran: ", "the raises do not stop: ") +
                   std::to_string(cfg::MAX_RAISE_LEVELS) +
                   M(" niveles y todavía cabe otra. Una apuesta tan pequeña se sube al "
                     "mínimo, de ficha en ficha, y no llega al stack nunca -- usa una "
                     "más grande, o baja el umbral de all-in",
                     " deep and another one is still legal. A bet that small is "
                     "min-raised one chip at a time and never reaches the stack -- use a "
                     "bigger one, or lower the all-in threshold");
        }
        return false;
    }

    int rec(BetTree& t, int street, double pot0, double spent, int player,
            double inv0, double inv1, int level, bool facing, bool prev_check,
            int aggr, bool donk_spot, int depth, const std::string& path) {
        const double inv[2]   = { inv0, inv1 };
        const int    opp      = 1 - player;
        const double to_call  = inv[opp] - inv[player];
        const double pot      = pot0 + inv0 + inv1;
        const double cap      = cfg::STACK - spent;   // chips left this round
        const bool   is_river = (street == ST_RIVER);

        std::vector<ActionInfo> acts;
        std::vector<Succ>       succ;

        if (facing && to_call > 1e-9) {
            acts.push_back(ActionInfo{ AK_FOLD, inv[player], "F", "Fold" });
            succ.push_back(Succ{ true, NT_FOLD, player, pot0 * 0.5 + inv[player],
                                 inv0, inv1, 0, false, false, aggr });
            {
                double ni[2] = { inv0, inv1 };
                ni[player] = inv[opp];
                acts.push_back(ActionInfo{ AK_CALL, inv[opp], "C", "Call " + num(to_call) });
                succ.push_back(Succ{ true, is_river ? NT_SHOWDOWN : NT_CONT, -1,
                                     pot0 * 0.5 + inv[opp], ni[0], ni[1], 0, false, false, aggr });
            }
            // IP's "don't 3-bet": no third aggressive action of the street, and
            // none past it either. `level` counts what has already gone in, so
            // the action about to be taken is the (level+1)-th.
            const bool gagged = (player == 1 && c_.no_3bet[street] && level >= 2);
            if (!gagged && inv[opp] < cap - 1e-9 && room_to_raise(level)) {
                std::vector<double> tos;
                for (const Sizing& rz : c_.raises[street][player]) {
                    // "3x" sube POR tres veces lo que hay que pagar, sobre lo
                    // que uno ya llevaba puesto. Un numero suelto anade esa
                    // fraccion del bote de despues de igualar.
                    //
                    // Aqui ponia `rz.v * inv[opp]` -- tres veces el TOTAL del
                    // rival -- y no es lo que hace la referencia. Las dos cuentas
                    // dan lo mismo mientras uno no lleve nada puesto, que es la
                    // primera subida de cada calle, y por eso 610 de 611 nodos
                    // coincidian. Se separan del 3-bet en adelante.
                    //
                    // Sacado de las 611 lineas de un arbol suyo: son los tres
                    // unicos sitios donde su "5x" no cae en la minima ni en el
                    // all-in, y los tres cuadran con esto y ninguno con lo otro.
                    //
                    //    llevo 24, el 36 -> 84   =  24 + 5*12   (5*36 = 180 no)
                    //    llevo 48, el 60 -> 108  =  48 + 5*12   (5*60 = 300 no)
                    //    llevo 72, el 84 -> 132  =  72 + 5*12   (5*84 = 420 no)
                    //
                    // De paso, "2x" pasa a ser la subida minima en CUALQUIER
                    // nivel y no solo sobre una primera apuesta.
                    double to = rz.minraise ? inv[opp] + to_call
                              : rz.xbet     ? inv[player] + rz.v * to_call
                                            : inv[opp] + rz.v * (pot0 + 2.0 * inv[opp]);
                    to = whole_chips(to);
                    // El minimo legal va DESPUES del truncado: truncar por
                    // debajo del minimo daria una subida que no sube.
                    const double min_to = inv[opp] + to_call;
                    if (to < min_to) to = min_to;
                    if (to > cap) to = cap;
                    to = snap_allin(to, cap, spent);
                    if (to > inv[opp] + 1e-9) tos.push_back(to);
                }
                // El all-in como SUBIDA solo existe si ese jugador tiene
                // lista de subidas en esa calle.
                //
                // Es la regla de la referencia, y se lee contrastando dos nodos
                // suyos del mismo arbol. En el river, OOP tiene subidas (3x) y
                // el interruptor de all-in puesto: ofrece b114 y b220. IP tiene
                // el interruptor puesto tambien pero NINGUNA subida: ofrece
                // solo pagar o tirarse. O sea que el interruptor anade el
                // all-in A LA LISTA DE SUBIDAS, y sin lista no hay donde
                // anadirlo -- no inventa una subida que no existe.
                //
                // Sin esta condicion teniamos ocho nodos de mas en este spot, y
                // un all-in de mas cambia el juego en toda la rama que cuelga
                // de el. Con ella pero sin el resto, nos faltaba uno.
                if (c_.add_allin[street][player] &&
                    !c_.raises[street][player].empty() &&
                    cap > inv[opp] + 1e-9)
                    tos.push_back(cap);
                merge_sizings(tos);
                for (double to : tos)
                    add_sizing(t, acts, succ, AK_RAISE, to, player, inv0, inv1, level);
            }
        } else {
            acts.push_back(ActionInfo{ AK_CHECK, inv[player], "X", "Check" });
            if (prev_check) {
                succ.push_back(Succ{ true, is_river ? NT_SHOWDOWN : NT_CONT, -1,
                                     pot0 * 0.5 + inv[player], inv0, inv1, 0, false, false, aggr });
            } else {
                succ.push_back(Succ{ false, NT_DECISION, -1, 0.0, inv0, inv1, level,
                                     false, true, aggr });
            }
            // A lead into last street's aggressor is priced from the donk list;
            // anything else from the player's own bet list. No sizings, no
            // action -- which for an empty donk list is exactly what the old
            // on/off switch did when it was off.
            const std::vector<double>& blist =
                donk_spot ? c_.donks[street] : c_.bets[street][player];
            if (!blist.empty() && inv[player] < cap - 1e-9) {
                std::vector<double> tos;
                for (double f : blist) {
                    double to = whole_chips(inv[player] + f * pot);
                    // Con fichas enteras, un tamano por debajo de una ficha se
                    // trunca a nada y la apuesta desaparece. Antes existia,
                    // valia 0,02 fichas y no se podia poner en una mesa; ahora
                    // no existe, y desaparecer en silencio es peor que las dos
                    // cosas. Solo pasa aqui: una subida tiene minimo legal por
                    // debajo del cual no baja, asi que nunca se queda en nada.
                    if (to <= inv[player] + 1e-9 && ok_) {
                        ok_  = false;
                        err_ = M("una apuesta de ", "a bet of ") + num(f * pot) +
                               M(" sobre un bote de ", " on a pot of ") + num(pot) +
                               M(" no llega a una ficha. Las apuestas son fichas enteras, "
                                 "como en una mesa: el tamaño más pequeño que permite este "
                                 "bote es el ",
                                 " is less than one chip. Bets are whole chips, like at a "
                                 "table: the smallest size this pot allows is ") +
                               num(std::ceil(100.0 / pot)) + "%";
                    }
                    if (to > cap) to = cap;
                    to = snap_allin(to, cap, spent);
                    if (to > inv[player] + 1e-9) tos.push_back(to);
                }
                if (c_.add_allin[street][player] && cap > inv[player] + 1e-9) tos.push_back(cap);
                merge_sizings(tos);
                for (double to : tos)
                    add_sizing(t, acts, succ, AK_BET, to, player, inv0, inv1, level);
            }
        }

        if (static_cast<int>(acts.size()) > cfg::MAX_ACTIONS) {
            ok_ = false;
            // El mensaje decia "branching factor 12 exceeds MAX_ACTIONS 8", que es
            // exacto y no ayuda a nadie: no dice que hacer ni de que calle habla.
            err_ = M("son ", "that is ") + std::to_string(acts.size()) +
                   M(" acciones en un nodo y caben ", " actions at one node and only ") +
                   std::to_string(cfg::MAX_ACTIONS) +
                   M(". Contando pasar, pagar y retirarse: quita algún tamaño de "
                     "apuesta o de subida de esa calle",
                     " fit. Counting check, call and fold: drop a bet or raise size "
                     "on that street");
            acts.resize(cfg::MAX_ACTIONS);
            succ.resize(cfg::MAX_ACTIONS);
        }

        Node n;
        n.type          = NT_DECISION;
        n.player        = player;
        n.num_actions   = static_cast<int>(acts.size());
        n.pot           = pot;
        n.depth         = depth;
        n.path          = path;
        n.action_offset = static_cast<int>(t.actions.size());
        n.child_offset  = static_cast<int>(t.children.size());
        for (const ActionInfo& a : acts) t.actions.push_back(a);
        t.children.resize(static_cast<size_t>(n.child_offset) + n.num_actions, -1);

        const int nid = static_cast<int>(t.nodes.size());
        t.nodes.push_back(n);

        for (int a = 0; a < static_cast<int>(succ.size()); ++a) {
            const Succ& s = succ[a];
            const std::string cpath = path + "/" + acts[a].code;
            int cid;
            if (s.terminal) {
                Node tn;
                tn.type       = s.ty;
                tn.player     = s.folder;
                tn.terminal_W = s.W;
                tn.pot        = pot0 + s.ni0 + s.ni1;
                // terminal_W is half the starting pot plus whatever the loser
                // put in, so twice it is exactly the matched pot -- the same
                // formula at a fold, where the uncalled part goes back, and at
                // a showdown, where there is nothing uncalled.
                if (s.ty == NT_FOLD || s.ty == NT_SHOWDOWN)
                    tn.rake = cfg::rake_on_pot(2.0 * s.W);
                tn.depth      = depth + 1;
                tn.path       = cpath;
                if (s.ty == NT_CONT) {
                    tn.cont_id = t.num_cont++;
                    // Both players have matched, so either investment will do.
                    t.cont_inv.push_back(s.ni0);
                    t.cont_aggr.push_back(s.naggr);
                }
                cid = static_cast<int>(t.nodes.size());
                t.nodes.push_back(tn);
            } else {
                cid = rec(t, street, pot0, spent, 1 - player, s.ni0, s.ni1, s.nlevel,
                          s.nfacing, s.nprev, s.naggr, false, depth + 1, cpath);
            }
            t.children[t.nodes[static_cast<size_t>(nid)].child_offset + a] = cid;
        }
        return nid;
    }
};

// -----------------------------------------------------------------------------
// Todas las lineas del arbol en la notacion de la referencia, para poder comparar
// dos arboles con un diff en vez de mirando nodos a mano.
//
// Hace falta porque cinco veces esta semana un "desacuerdo entre motores"
// resulto ser que los dos programas no estaban resolviendo el mismo juego, y
// cada vez se tardo horas en verlo: se comprueban seis o siete nodos elegidos
// por uno mismo, coinciden todos, y el que difiere esta en los otros
// seiscientos. Esto los enumera enteros.
//
// El formato es el de `show_all_lines`: r:0:b33:c:b108... donde el numero es la
// INVERSION ACUMULADA del que actua -- no lo que pone en esta calle -- y las
// cartas no aparecen, porque lo que se compara es la estructura de apuestas.
// Un paso de la linea: "f", "c", o "b" mas la inversion ACUMULADA del que
// actua. Esta fuera de collect_lines porque lo usan dos recorridos -- las
// lineas y sus frecuencias -- y si cada uno lo escribiera a su manera las dos
// listas no se podrian cruzar por el nombre.
inline std::string line_step(const RoundCtx& rc, const ActionInfo& ai) {
    if (ai.kind == AK_FOLD) return "f";
    if (ai.kind == AK_CHECK || ai.kind == AK_CALL) return "c";
    char buf[40];
    std::snprintf(buf, sizeof buf, "b%g", rc.spent + ai.to_amount);
    return buf;
}

inline void collect_lines(const GameTree& T, int ci, int nid,
                          const std::string& pre, std::vector<std::string>& out) {
    const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
    const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
    out.push_back(pre);
    if (n.type == NT_CONT) {
        // El reparto de una carta no es una accion: se cruza y se sigue.
        const int cc = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
        if (cc >= 0) collect_lines(T, cc, T.ctx[static_cast<size_t>(cc)].tree.root, pre, out);
        return;
    }
    if (n.type != NT_DECISION) return;
    for (int a = 0; a < n.num_actions; ++a)
        collect_lines(T, ci, rc.tree.child(n, a),
                      pre + ":" + line_step(rc, rc.tree.act(n, a)), out);
}

// El nombre de linea de cada nodo, en vez de la lista de nombres.
//
// Hace falta para cruzar por nombre lo que se sabe de un nodo con lo que se
// sabe de su linea -- por ejemplo su frecuencia total. Un nodo de reparto y la
// raiz de la calle siguiente comparten nombre con la accion que los trajo,
// igual que en collect_lines, porque repartir una carta no es un paso.
inline void collect_node_lines(const GameTree& T, int ci, int nid, const std::string& pre,
                               std::map<std::pair<int, int>, std::string>& out) {
    const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
    const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
    out[std::make_pair(ci, nid)] = pre;
    if (n.type == NT_CONT) {
        const int cc = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
        if (cc >= 0) collect_node_lines(T, cc, T.ctx[static_cast<size_t>(cc)].tree.root, pre, out);
        return;
    }
    if (n.type != NT_DECISION) return;
    for (int a = 0; a < n.num_actions; ++a)
        collect_node_lines(T, ci, rc.tree.child(n, a),
                           pre + ":" + line_step(rc, rc.tree.act(n, a)), out);
}

inline std::map<std::pair<int, int>, std::string> node_lines(const GameTree& T) {
    std::map<std::pair<int, int>, std::string> out;
    if (!T.ctx.empty()) collect_node_lines(T, 0, T.ctx[0].tree.root, "r:0", out);
    return out;
}

inline std::vector<std::string> all_lines(const GameTree& T) {
    std::vector<std::string> out;
    if (!T.ctx.empty()) collect_lines(T, 0, T.ctx[0].tree.root, "r:0", out);
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

inline void print_tree(const GameTree& T, const Deal& D) {
    for (size_t ci = 0; ci < T.ctx.size(); ++ci) {
        const RoundCtx& rc = T.ctx[ci];
        std::fprintf(cfg::out, "\n  [ctx %2d] %-6s  line %-24s pot %7.2f  "
                     "instances %lld  blocks %d\n",
                     static_cast<int>(ci), STREET_NAME[rc.street], rc.label.c_str(),
                     rc.pot, rc.instances, rc.tree.blocks);
        for (const Node& n : rc.tree.nodes) {
            const std::string ind(static_cast<size_t>(n.depth) * 2 + 4, ' ');
            if (n.type == NT_DECISION) {
                std::fprintf(cfg::out, "%s%-16s %-3s  pot %7.2f  ->", ind.c_str(),
                             n.path.c_str(), n.player == 0 ? "OOP" : "IP", n.pot);
                for (int a = 0; a < n.num_actions; ++a)
                    std::fprintf(cfg::out, " %s%s", rc.tree.act(n, a).label.c_str(),
                                 a + 1 < n.num_actions ? " |" : "");
                if (n.is_locked) std::fprintf(cfg::out, "   [LOCK]");
                std::fprintf(cfg::out, "\n");
            } else if (n.type == NT_CONT) {
                const int cc = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
                std::fprintf(cfg::out, "%s%-16s deal %-5s pot %7.2f  -> ctx %d\n",
                             ind.c_str(), n.path.c_str(),
                             STREET_NAME[rc.street + 1], n.pot, cc);
            } else if (n.type == NT_SHOWDOWN) {
                std::fprintf(cfg::out, "%s%-16s SHOWDOWN    pot %7.2f  (winner nets %+.2f)\n",
                             ind.c_str(), n.path.c_str(), n.pot, n.terminal_W);
            } else {
                std::fprintf(cfg::out, "%s%-16s %-3s FOLDS  pot %7.2f  (forfeits %.2f)\n",
                             ind.c_str(), n.path.c_str(),
                             n.player == 0 ? "OOP" : "IP", n.pot, n.terminal_W);
            }
        }
    }
    (void)D;
}
