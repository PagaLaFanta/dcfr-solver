#pragma once
// =============================================================================
//  Text reports.
//
//  Everything a report needs about one node is gathered once into NodeStats:
//  reaching a flop node costs a full subtree evaluation, so it must not be
//  repeated per table row.
// =============================================================================

#include "msg.hpp"
#include "solver.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

inline void hr(const char* title) {
    std::fprintf(cfg::out,
        "\n=============================================================================\n"
        "  %s\n"
        "=============================================================================\n",
        title);
}

// -----------------------------------------------------------------------------
struct NodeStats {
    int         ci = 0, nid = 0;
    long long   inst = 0;
    int         player = 0, A = 0, nh = 0;
    std::string label, path;
    double      pot = 0.0;
    bool        locked = false;
    std::vector<std::string> codes, labels;
    std::vector<double>      strat;    // A*nh, average over this instance
    std::vector<double>      weight;   // per combo: own reach * compatible villain
    // La equity CONTRA EL RANGO DE ESTE NODO, no contra el de partida.
    //
    // Son dos numeros distintos en cuanto te mueves del inicio: el rival que
    // paga una apuesta llega con un rango mas fuerte, y un trio top que valia
    // 85% en el flop vale 74% cuando le han pagado. Antes se ensenaba el de
    // partida en todos los nodos, asi que a partir del segundo nodo la columna
    // decia "equity" y no lo era. La referencia lo recalcula por nodo, y es lo
    // unico util: la equity contra un rango que ya no existe no la mira nadie.
    //
    // Vacio si no se pidio (cuesta 13 ms en un flop y hay bucles que llaman a
    // gather 48 veces sin ensenar equity).
    std::vector<double>      eq;
    double                   wtot = 0.0;
    NodeView                 v;
    // With suit isomorphism the stored node belongs to a representative runout;
    // `back` maps a combo in the runout the user asked for onto the stored one.
    std::vector<int>         back;
    bool ok = false;

    int stored(int h) const { return back.empty() ? h : back[static_cast<size_t>(h)]; }

    double freq(int h, int a) const { return strat[static_cast<size_t>(a) * nh + h]; }
    double ev_action(int h, int a) const {
        const double c = v.compat[static_cast<size_t>(h)];
        if (c < 1e-12) return 0.0;
        return v.action_cfv[static_cast<size_t>(a)][static_cast<size_t>(h)] / c + cfg::POT0 * 0.5;
    }
    double ev_node(int h) const {
        const double c = v.compat[static_cast<size_t>(h)];
        if (c < 1e-12) return 0.0;
        return v.cfv[static_cast<size_t>(h)] / c + cfg::POT0 * 0.5;
    }
    // La equity de este nodo si se pidio; si no, la de partida que trae el
    // solver. Un solo sitio donde decidirlo, para que ninguna vista se quede
    // con la vieja por descuido.
    double equity(const DCFRSolver& S, int h) const {
        if (!eq.empty()) return eq[static_cast<size_t>(h)];
        return S.equity(player, h);
    }

    double node_freq(int a) const {
        double num = 0.0;
        for (int h = 0; h < nh; ++h) num += weight[static_cast<size_t>(h)] * freq(h, a);
        return wtot > 1e-12 ? num / wtot : 0.0;
    }
    // Cuantos COMBOS llegan a este nodo: el alcance propio sumado, sin
    // multiplicar por el del rival.
    //
    // No es `wtot`, y confundirlos ya ha salido caro dos veces. `wtot` es masa
    // de PAREJAS -- mi alcance por el del rival -- y sirve para promediar, pero
    // como numero de combos esta multiplicado por el tamano del rango contrario:
    // en un rango de 699 combos daba 88.889. Cualquier cosa que se ensene con la
    // palabra "combos" al lado tiene que salir de aqui.
    double combos() const {
        double n = 0.0;
        for (int h = 0; h < nh; ++h) n += v.own_reach[static_cast<size_t>(h)];
        return n;
    }

    double node_ev() const {
        double num = 0.0;
        for (int h = 0; h < nh; ++h) num += weight[static_cast<size_t>(h)] * ev_node(h);
        return wtot > 1e-12 ? num / wtot : 0.0;
    }
};

inline void permute(const std::vector<int>& back, std::vector<double>& v) {
    std::vector<double> t(v.size());
    for (size_t h = 0; h < v.size(); ++h) t[h] = v[static_cast<size_t>(back[h])];
    v.swap(t);
}

// Que hay en cada columna, porque dos de ellas se parecen y NO son lo mismo:
//
//   weight  alcance propio POR el rival compatible, o sea masa de PAREJAS.
//           Es el peso correcto para promediar -- la probabilidad de estar
//           en este nodo con esta mano es la del par, no la de la mano --
//           pero NO es el rango. En la raiz de un spot normal sale del orden
//           de 500 para una mano de peso 1, porque son quinientas y pico
//           manos del rival las que no la bloquean.
//   reach   alcance propio a secas: cuanto de esa mano llega aqui. ESTO es
//           el rango, y es lo que hay que mirar para leer un rango o para
//           comparar con el show_range de otro solver.
//
// La columna `reach` no estaba, y sin ella el CSV no permitia sacar el rango
// de un nodo: quien abria la hoja y leia "weight" leia masa de parejas
// creyendo leer pesos. Es la misma confusion que ya se arreglo en el panel de
// mano hecha y que no se habia propagado hasta aqui.
//
// One decision point as a table, for taking somewhere else. It lived inside
// the console, where nothing could get at it to check it -- and an export that
// quietly writes the wrong column is exactly the kind of thing nobody notices,
// because by the time you are looking at it you are in a spreadsheet.
// Lo que se dice al acabar un solve.
//
// Esta aparte para poder comprobarlo, porque decia una mentira de diez veces:
// imprimia las iteraciones PEDIDAS. Con el objetivo de precision puesto -- que
// viene puesto de fabrica -- el solve para en cuanto lo alcanza, asi que
// `solve 20000` ensenaba "20000 iterations in 79.80s" cuando habia corrido
// 2000. El numero que importa es el que se hizo, y si paro por el objetivo hay
// que decirlo, que es una buena noticia y no un recorte.
inline std::string solve_summary(long long hechas, int pedidas, double secs,
                                 int threads, bool paro_precision,
                                 bool paro_tiempo = false) {
    char b[300];
    // El tope de tiempo manda sobre el de precision en el mensaje: si salto el
    // reloj, la precision NO se alcanzo, y decir "pare porque ya estaba" seria
    // justo lo contrario de lo que paso.
    if (paro_tiempo)
        std::snprintf(b, sizeof b,
                      M("  %lld iteraciones en %.2fs (%d hilos)\n"
                        "  -- paro al llegar al tope de tiempo, sin alcanzar la "
                        "precision pedida",
                        "  %lld iterations in %.2fs (%d threads)\n"
                        "  -- stopped on the time limit, without reaching the "
                        "accuracy you asked for"),
                      hechas, secs, threads);
    else if (paro_precision && hechas < static_cast<long long>(pedidas))
        std::snprintf(b, sizeof b,
                      M("  %lld iteraciones en %.2fs (%d hilos)\n"
                        "  -- paro al alcanzar la precision pedida, de las %d "
                        "que le diste",
                        "  %lld iterations in %.2fs (%d threads)\n"
                        "  -- stopped on reaching the accuracy you asked for, out "
                        "of the %d you gave it"),
                      hechas, secs, threads, pedidas);
    else
        std::snprintf(b, sizeof b, M("  %lld iteraciones en %.2fs (%d hilos)",
                                     "  %lld iterations in %.2fs (%d threads)"),
                      hechas, secs, threads);
    return b;
}

inline bool write_csv(const std::string& path, DCFRSolver& sol, const Deal& D,
                      const NodeStats& N, const std::string& board,
                      const std::string& where, std::string& e) {
    if (!N.ok) { e = "nothing to write: that is not a solved decision point"; return false; }
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) { e = "cannot write to '" + path + "'"; return false; }

    std::fprintf(f, "board,%s\nnode,%s %s\nrunout,%s\nplayer,%s\npot,%.4f\n\n",
                 board.c_str(), N.label.c_str(), N.path.c_str(), where.c_str(),
                 N.player == 0 ? "OOP" : "IP", N.pot);
    std::fprintf(f, "combo,class,equity,locked,weight,reach");
    for (int a = 0; a < N.A; ++a)
        std::fprintf(f, ",freq_%s,ev_%s", N.codes[static_cast<size_t>(a)].c_str(),
                     N.codes[static_cast<size_t>(a)].c_str());
    std::fprintf(f, ",ev_node\n");

    for (int h = 0; h < N.nh; ++h) {
        const double w = N.weight[static_cast<size_t>(h)];
        if (w <= 1e-12) continue;
        const Combo& k = D.combos[static_cast<size_t>(h)];
        std::fprintf(f, "%s%s,%s,%.4f,%d,%.6f,%.6f", card_str(k.c1).c_str(),
                     card_str(k.c2).c_str(), class_name(k.cls).c_str(),
                     N.equity(sol, h),
                     sol.is_hand_locked(N.ci, N.nid, N.inst, h) ? 1 : 0, w,
                     N.v.own_reach[static_cast<size_t>(h)]);
        for (int a = 0; a < N.A; ++a)
            std::fprintf(f, ",%.6f,%.6f", N.freq(h, a), N.ev_action(h, a));
        std::fprintf(f, ",%.6f\n", N.ev_node(h));
    }
    const bool okflag = std::fflush(f) == 0;
    std::fclose(f);
    if (!okflag) { e = "write failed -- out of disk?"; return false; }
    return true;
}

inline NodeStats gather(DCFRSolver& S, int ci, int nid, long long inst, int perm,
                        bool with_equity = false) {
    NodeStats N;
    const GameTree& T  = S.tree();
    if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return N;
    const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
    if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) return N;
    const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
    if (n.type != NT_DECISION) return N;

    N.ci = ci; N.nid = nid; N.inst = inst;
    N.player = n.player;
    N.A  = n.num_actions;
    N.nh = S.num_hands();
    N.label = rc.label;
    N.path  = n.path;
    N.pot   = n.pot;
    N.locked = (n.lock_for(inst) != nullptr);   // en ESTE runout, no en el nodo
    for (int a = 0; a < N.A; ++a) {
        N.codes.push_back(rc.tree.act(n, a).code);
        N.labels.push_back(rc.tree.act(n, a).label);
    }
    if (!S.query(ci, nid, inst, N.v)) return N;

    N.strat.assign(static_cast<size_t>(N.A) * N.nh, 0.0);
    S.avg_strategy_inst(ci, nid, inst, N.strat.data());

    // Antes de rotar los marcos: las tablas del reparto estan en el del
    // representante, asi que la equity se calcula ahi y se rota con el resto.
    if (with_equity) {
        // Las cartas que ya salieron, y las del REPRESENTANTE: las tablas de
        // fuerza estan en su marco, igual que todo lo de aqui arriba, y la
        // rotacion al runout pedido viene despues.
        std::vector<int> ya;
        const int levels = rc.street - T.start;
        if (levels > 0) {
            std::vector<long long> ci_chain;
            std::vector<int> slots;
            if (S.deal().decompose(inst, levels, ci_chain, slots))
                for (int sl : slots) ya.push_back(S.deal().deck[static_cast<size_t>(sl)]);
        }
        compute_equity(S.deal(), N.v.opp_reach, N.eq, ya);
    }

    // Rotate every per-combo vector into the requested runout's frame.
    const Deal& D = S.deal();
    if (perm != D.identity()) {
        N.back = D.perm_combo[static_cast<size_t>(D.perm_inv[perm])];
        permute(N.back, N.v.own_reach);
        permute(N.back, N.v.opp_reach);
        permute(N.back, N.v.compat);
        permute(N.back, N.v.cfv);
        if (!N.eq.empty()) permute(N.back, N.eq);
        for (std::vector<double>& x : N.v.action_cfv) permute(N.back, x);
        std::vector<double> ns(static_cast<size_t>(N.A) * N.nh, 0.0);
        for (int a = 0; a < N.A; ++a)
            for (int h = 0; h < N.nh; ++h)
                ns[static_cast<size_t>(a) * N.nh + h] =
                    N.strat[static_cast<size_t>(a) * N.nh + N.back[static_cast<size_t>(h)]];
        N.strat.swap(ns);
    }

    N.weight.assign(static_cast<size_t>(N.nh), 0.0);
    for (int h = 0; h < N.nh; ++h) {
        const double w = N.v.own_reach[static_cast<size_t>(h)] * N.v.compat[static_cast<size_t>(h)];
        N.weight[static_cast<size_t>(h)] = w > 0.0 ? w : 0.0;
        N.wtot += N.weight[static_cast<size_t>(h)];
    }
    N.ok = true;
    return N;
}

// Both players' EV at a node.
//
// Without rake the game is zero sum, so one traversal answers for both and the
// other player is just pot minus the first. Rake breaks that: the house takes
// chips out, the two EVs no longer add up to the pot, and every display that
// took the shortcut would quietly show the wrong number for IP. So when rake
// is on, both sides are evaluated properly -- twice the work, but only for the
// people who asked for rake.
inline void node_ev_both(DCFRSolver& S, int ci, int nid, long long inst,
                         int player, double own_ev, double& oop, double& ip) {
    if (cfg::RAKE_PCT <= 0.0) {
        oop = (player == 0) ? own_ev : cfg::POT0 - own_ev;
        ip  = (player == 1) ? own_ev : cfg::POT0 - own_ev;
        return;
    }
    oop = (player == 0) ? own_ev : S.node_ev_player(ci, nid, inst, 0);
    ip  = (player == 1) ? own_ev : S.node_ev_player(ci, nid, inst, 1);
}

// What fraction of all starting (my hand, villain hand) pairs actually arrive at
// this node under the solved strategy.
inline double node_reach_pct(const DCFRSolver& S, const NodeStats& N) {
    if (!N.ok) return 0.0;
    double den = 0.0;
    for (int h = 0; h < N.nh; ++h)
        den += S.range(N.player)[static_cast<size_t>(h)] * S.start_compat(N.player, h);
    return den > 1e-12 ? 100.0 * N.wtot / den : 0.0;
}

// -----------------------------------------------------------------------------
enum AggKind { AG_MADE = 0, AG_DRAW = 1 };

struct ClassAgg {
    int                 cls = 0;
    // De que familia es la fila, porque hay DOS y no se cruzan:
    //
    //   AG_MADE  `cls` es una MadeCat   (flush, set, top_pair...)
    //   AG_DRAW  `cls` es una DrawCat   (flush_draw, combo_draw...)
    //
    // Antes habia una fila por CADA PAR (mano hecha, proyecto) y de ahi salian
    // cosas como "set + flush_draw". Eso no es de la referencia: tiene las dos listas
    // por separado y un combo cae en una de cada. Lo suyo se ve en su lenguaje
    // de filtros, donde `set` y `flush_draw` son nombres del mismo nivel y "un
    // set con proyecto de color" se PIDE con `set & flush_draw` -- no existe
    // como familia. Cruzarlas convertia 17 familias en 54 filas.
    int                 kind = AG_MADE;
    double              w = 0.0, eq = 0.0, node_ev = 0.0;
    std::vector<double> freq, ev;
    bool                locked = false;
};

// Como se llama una fila.
inline const char* agg_name(const ClassAgg& g) {
    return g.kind == AG_DRAW ? DR_NAME[g.cls] : MC_NAME[g.cls];
}

// Lo mismo pero por CATEGORIA, no por las 169 casillas.
//
// La rejilla 13x13 es un mapa de que cartas tienes; esto es un mapa de que
// tienes hecho. Son preguntas distintas y la segunda es la que se hace una
// persona mirando un board: "con cuanta frecuencia apuesto mis trios", no "con
// cuanta frecuencia apuesto 99". Y en un board dado los trios estan repartidos
// por toda la rejilla.
//
// El `cls` de cada fila guarda la categoria (MadeCat), no la casilla.
inline std::vector<ClassAgg> aggregate_by_made(const DCFRSolver& S, const NodeStats& N,
                                               const std::vector<int>& board) {
    const Deal& D = S.deal();
    // Una casilla por (mano hecha, proyecto). Casi todas se quedan vacias -- no
    // hay tantos colores con doble pareja -- y las vacias no salen.
    // Dos bloques seguidos, no un producto: primero las MC_COUNT familias de
    // mano hecha y detras las DR_COUNT de proyecto. Cada bloque reparte el
    // rango entero por su cuenta, asi que cada uno suma el 100%.
    std::vector<ClassAgg> by(static_cast<size_t>(MC_COUNT) + DR_COUNT);
    for (int c = 0; c < MC_COUNT; ++c) {
        ClassAgg& g = by[static_cast<size_t>(c)];
        g.kind = AG_MADE;
        g.cls = c;
        g.freq.assign(static_cast<size_t>(N.A), 0.0);
        g.ev.assign(static_cast<size_t>(N.A), 0.0);
    }
    for (int d = 0; d < DR_COUNT; ++d) {
        ClassAgg& g = by[static_cast<size_t>(MC_COUNT) + static_cast<size_t>(d)];
        g.kind = AG_DRAW;
        g.cls = d;
        g.freq.assign(static_cast<size_t>(N.A), 0.0);
        g.ev.assign(static_cast<size_t>(N.A), 0.0);
    }
    for (int h = 0; h < N.nh; ++h) {
        const double w = N.weight[static_cast<size_t>(h)];
        if (w <= 1e-12) continue;
        const Combo& k = D.combos[static_cast<size_t>(h)];
        const int hecho = made_cat(k.c1, k.c2, board);
        const int proy  = draw_cat(k.c1, k.c2, board);
        // El combo cuenta en su familia de mano hecha Y en la de proyecto.
        ClassAgg* dos[2] = { &by[static_cast<size_t>(hecho)],
                             &by[static_cast<size_t>(MC_COUNT) +
                                 static_cast<size_t>(proy)] };
        for (ClassAgg* gp : dos) {
        ClassAgg& g = *gp;
        g.w += w;
        g.eq += w * N.equity(S, h);
        g.node_ev += w * N.ev_node(h);
        if (S.is_hand_locked(N.ci, N.nid, N.inst, N.stored(h))) g.locked = true;
        for (int a = 0; a < N.A; ++a) {
            g.freq[static_cast<size_t>(a)] += w * N.freq(h, a);
            g.ev[static_cast<size_t>(a)]   += w * N.ev_action(h, a);
        }
        }
    }
    // Que familias son POSIBLES en este board, tenga uno lo que tenga. La referencia las
    // lista aunque esten a cero: en A-K-6 ensena "set 0.0 combos", que dice algo
    // -- aqui hay sets y tu no llevas ninguno -- y en cambio no ensena color ni
    // escalera, que en ese board no existen. La lista sale del BOARD y no del
    // rango.
    std::vector<char> cabe(static_cast<size_t>(MC_COUNT) + DR_COUNT, 0);
    for (int h = 0; h < N.nh; ++h) {
        const Combo& k = D.combos[static_cast<size_t>(h)];
        if (k.c1 < 0) continue;
        bool choca = false;
        for (int b : board) if (b == k.c1 || b == k.c2) choca = true;
        if (choca) continue;
        cabe[static_cast<size_t>(made_cat(k.c1, k.c2, board))] = 1;
        cabe[static_cast<size_t>(MC_COUNT) +
             static_cast<size_t>(draw_cat(k.c1, k.c2, board))] = 1;
    }
    std::vector<ClassAgg> out;
    for (size_t i = 0; i < by.size(); ++i) {
        ClassAgg& g = by[i];
        if (!cabe[i]) continue;
        if (g.w <= 1e-12) {
            out.push_back(g);          // posible en el board, vacia en el rango
            continue;
        }
        g.eq /= g.w;
        g.node_ev /= g.w;
        for (int a = 0; a < N.A; ++a) {
            g.freq[static_cast<size_t>(a)] /= g.w;
            g.ev[static_cast<size_t>(a)]   /= g.w;
        }
        out.push_back(g);
    }
    // Primero el bloque de mano hecha y detras el de proyecto, y dentro de cada
    // uno de la mas fuerte a la mas floja. El enum de mano hecha va de fuerte a
    // flojo y el de proyecto al reves, de ahi que uno se ordene al derecho y el
    // otro al reves.
    std::sort(out.begin(), out.end(),
              [](const ClassAgg& a, const ClassAgg& b) {
                  if (a.kind != b.kind) return a.kind < b.kind;
                  return a.kind == AG_MADE ? a.cls < b.cls : a.cls > b.cls;
              });
    return out;
}

inline std::vector<ClassAgg> aggregate_by_class(const DCFRSolver& S, const NodeStats& N) {
    const Deal& D = S.deal();
    std::vector<ClassAgg> by(169);
    for (int c = 0; c < 169; ++c) {
        by[static_cast<size_t>(c)].cls = c;
        by[static_cast<size_t>(c)].freq.assign(static_cast<size_t>(N.A), 0.0);
        by[static_cast<size_t>(c)].ev.assign(static_cast<size_t>(N.A), 0.0);
    }
    for (int h = 0; h < N.nh; ++h) {
        const double w = N.weight[static_cast<size_t>(h)];
        if (w <= 1e-12) continue;
        ClassAgg& g = by[static_cast<size_t>(D.combos[static_cast<size_t>(h)].cls)];
        g.w += w;
        g.eq += w * N.equity(S, h);
        g.node_ev += w * N.ev_node(h);
        if (S.is_hand_locked(N.ci, N.nid, N.inst, N.stored(h))) g.locked = true;
        for (int a = 0; a < N.A; ++a) {
            g.freq[static_cast<size_t>(a)] += w * N.freq(h, a);
            g.ev[static_cast<size_t>(a)]   += w * N.ev_action(h, a);
        }
    }
    std::vector<ClassAgg> out;
    for (int c = 0; c < 169; ++c) {
        ClassAgg& g = by[static_cast<size_t>(c)];
        if (g.w <= 1e-12) continue;
        g.eq /= g.w;
        g.node_ev /= g.w;
        for (int a = 0; a < N.A; ++a) { g.freq[static_cast<size_t>(a)] /= g.w; g.ev[static_cast<size_t>(a)] /= g.w; }
        out.push_back(g);
    }
    std::sort(out.begin(), out.end(),
              [](const ClassAgg& a, const ClassAgg& b) { return a.eq > b.eq; });
    return out;
}

// -----------------------------------------------------------------------------
inline void report_spot(const DCFRSolver& S) {
    const Deal& D = S.deal();
    const GameTree& T = S.tree();
    std::fprintf(cfg::out, "  Board  %s   (%s solve)\n",
                 board_str(D.board).c_str(), STREET_NAME[D.start]);
    std::fprintf(cfg::out, "  Pot %.2f   Stack %.2f   Combos %d   Runouts %d\n",
                 cfg::POT0, cfg::STACK, D.num(), D.num_runouts);
    std::fprintf(cfg::out, "  Tree: %d contexts, %d template nodes, %lld instanced nodes, "
                 "%.3f GB\n",
                 static_cast<int>(T.ctx.size()), T.num_decision_nodes(),
                 T.num_instanced_nodes(), T.total_gb());
    for (int p = 0; p < 2; ++p) {
        int live = 0;
        double tot = 0.0;
        for (int h = 0; h < S.num_hands(); ++h)
            if (S.range(p)[static_cast<size_t>(h)] > 0.0) { ++live; tot += S.range(p)[static_cast<size_t>(h)]; }
        std::fprintf(cfg::out, "  %-4s %4d combos, weight %.2f\n", p == 0 ? "OOP" : "IP", live, tot);
    }
}

inline void report_node_header(const NodeStats& N) {
    std::fprintf(cfg::out, "\n  %s %s   [%s to act, pot %.2f]%s\n",
                 N.label.c_str(), N.path.c_str(),
                 N.player == 0 ? "OOP" : "IP", N.pot, N.locked ? "   NODELOCKED" : "");
}

inline void report_frequencies(DCFRSolver& S, const NodeStats& N) {
    if (!N.ok) { std::fprintf(cfg::out, "  (no data)\n"); return; }
    report_node_header(N);
    // Both sides, always: the two add up to the pot at every node, so the split
    // says at a glance who this spot belongs to.
    const double evOwn = N.node_ev();
    double evOOP = 0.0, evIP = 0.0;
    node_ev_both(S, N.ci, N.nid, N.inst, N.player, evOwn, evOOP, evIP);
    std::fprintf(cfg::out, "  EV  OOP %8.4f (%5.1f%%)   IP %8.4f (%5.1f%%)   "
                 "reaches %.2f%% of hands\n",
                 evOOP, 100.0 * evOOP / cfg::POT0,
                 evIP, 100.0 * evIP / cfg::POT0,
                 node_reach_pct(S, N));
    std::fprintf(cfg::out, "  strategy         ");
    for (int a = 0; a < N.A; ++a)
        std::fprintf(cfg::out, "%s %5.2f%%   ", N.codes[static_cast<size_t>(a)].c_str(),
                     100.0 * N.node_freq(a));
    std::fprintf(cfg::out, "\n");
}


inline void report_hand_table(const DCFRSolver& S, const NodeStats& N, int limit) {
    if (!N.ok) { std::fprintf(cfg::out, "  (no data)\n"); return; }
    report_node_header(N);
    std::fprintf(cfg::out, "  %-6s %-4s %-14s %7s %6s", "HAND", "LOCK", "TYPE", "COMBO%", "EQ%");
    for (int a = 0; a < N.A; ++a)
        std::fprintf(cfg::out, " | %-6s %6s %8s", N.codes[static_cast<size_t>(a)].c_str(), "freq", "EV");
    std::fprintf(cfg::out, " | %8s\n", "EV(node)");

    const std::vector<ClassAgg> rows = aggregate_by_class(S, N);
    int shown = 0;
    for (const ClassAgg& g : rows) {
        if (limit > 0 && shown >= limit) {
            std::fprintf(cfg::out, "  ... %d more classes (raise the limit, or `csv`)\n",
                         static_cast<int>(rows.size()) - shown);
            break;
        }
        ++shown;
        std::fprintf(cfg::out, "  %-6s %-4s %6.2f%% %6.1f", class_name(g.cls).c_str(),
                     g.locked ? "LOCK" : "", 100.0 * g.w / N.wtot, g.eq);
        for (int a = 0; a < N.A; ++a)
            std::fprintf(cfg::out, " | %-6s %5.1f%% %8.3f", "",
                         100.0 * g.freq[static_cast<size_t>(a)], g.ev[static_cast<size_t>(a)]);
        std::fprintf(cfg::out, " | %8.3f\n", g.node_ev);
    }
}

// Individual combos, strongest first. On a board where suits matter -- three to
// a flush, say -- a class row averages a nut flush together with air.
inline void report_combo_table(const DCFRSolver& S, const NodeStats& N, int limit) {
    if (!N.ok) { std::fprintf(cfg::out, "  (no data)\n"); return; }
    const Deal& D = S.deal();
    report_node_header(N);
    std::fprintf(cfg::out, "  %-6s %-5s %-4s %-14s %7s %6s", "COMBO", "CLASS", "LOCK",
                 "TYPE", "WEIGHT", "EQ%");
    for (int a = 0; a < N.A; ++a)
        std::fprintf(cfg::out, " | %-6s %6s %8s", N.codes[static_cast<size_t>(a)].c_str(), "freq", "EV");
    std::fprintf(cfg::out, " | %8s\n", "EV(node)");

    std::vector<int> idx;
    for (int h = 0; h < N.nh; ++h) if (N.weight[static_cast<size_t>(h)] > 1e-12) idx.push_back(h);
    const int p = N.player;
    std::sort(idx.begin(), idx.end(),
              [&S, p](int a, int b) { return S.equity(p, a) > S.equity(p, b); });

    int shown = 0;
    for (int h : idx) {
        if (limit > 0 && shown >= limit) {
            std::fprintf(cfg::out, "  ... %d more combos (`combos 0` for all, or `csv`)\n",
                         static_cast<int>(idx.size()) - shown);
            break;
        }
        ++shown;
        const Combo& k = D.combos[static_cast<size_t>(h)];
        std::fprintf(cfg::out, "  %-6s %-5s %-4s %6.2f%% %6.1f",
                     (card_str(k.c1) + card_str(k.c2)).c_str(), class_name(k.cls).c_str(),
                     S.is_hand_locked(N.ci, N.nid, N.inst, N.stored(h)) ? "LOCK" : "",
                     100.0 * N.weight[static_cast<size_t>(h)] / N.wtot, N.equity(S, h));
        for (int a = 0; a < N.A; ++a)
            std::fprintf(cfg::out, " | %-6s %5.1f%% %8.3f", "",
                         100.0 * N.freq(h, a), N.ev_action(h, a));
        std::fprintf(cfg::out, " | %8.3f\n", N.ev_node(h));
    }
}

// The 13x13 grid as text: one action's frequency per starting-hand class.
inline void report_grid(const DCFRSolver& S, const NodeStats& N, int action) {
    if (!N.ok || action < 0 || action >= N.A) return;
    std::vector<double> freq(169, -1.0);
    for (const ClassAgg& g : aggregate_by_class(S, N))
        freq[static_cast<size_t>(g.cls)] = g.freq[static_cast<size_t>(action)];

    std::fprintf(cfg::out, "\n  %s frequency at %s %s  (suited above the diagonal, "
                 "offsuit below; '.' = not in range)\n\n",
                 N.labels[static_cast<size_t>(action)].c_str(), N.label.c_str(), N.path.c_str());
    std::fprintf(cfg::out, "       ");
    for (int j = 0; j < 13; ++j) std::fprintf(cfg::out, "  %c  ", RANK_CH[12 - j]);
    std::fprintf(cfg::out, "\n");
    for (int i = 0; i < 13; ++i) {
        std::fprintf(cfg::out, "    %c  ", RANK_CH[12 - i]);
        for (int j = 0; j < 13; ++j) {
            const double f = freq[static_cast<size_t>(i * 13 + j)];
            if (f < 0.0) std::fprintf(cfg::out, "  .  ");
            else         std::fprintf(cfg::out, " %3.0f ", 100.0 * f);
        }
        std::fprintf(cfg::out, "\n");
    }
}

// The whole text report. It used to be assembled inside the console, one
// fprintf at a time, where nothing could get at it -- the same place the CSV
// was, and for the same reason nobody had checked it.
inline void write_report(std::FILE* f, DCFRSolver& sol, const GameTree& T,
                         const Deal& D) {
    std::FILE* const prev = cfg::out;
    if (f) cfg::out = f;

    hr("SPOT");         report_spot(sol);
    hr("BETTING TREE"); print_tree(T, D);
    hr("STRATEGY, FIRST DECISION OF EACH ROUND");
    for (size_t ci = 0; ci < T.ctx.size(); ++ci) {
        const NodeStats N = gather(sol, static_cast<int>(ci),
                                   T.ctx[ci].tree.root, 0, D.identity(), true);
        if (!N.ok) continue;
        report_frequencies(sol, N);
    }
    hr("ROOT GRID");
    {
        const NodeStats N = gather(sol, 0, T.ctx[0].tree.root, 0, D.identity(), true);
        for (int a = 0; a < N.A; ++a) report_grid(sol, N, a);
    }
    const double ev0 = sol.root_ev(0);
    std::fprintf(cfg::out, "\n  Game value: OOP %.4f   IP %.4f   (pot %.1f)\n",
                 ev0, (cfg::RAKE_PCT > 0.0) ? sol.root_ev(1) : cfg::POT0 - ev0,
                 cfg::POT0);
    cfg::out = prev;
}

// -----------------------------------------------------------------------------
//  Aggregate across runouts.
//
//  Browsing a solve one node at a time tells you what happens on the Ts. It
//  does not tell you that OOP bets every low card and checks every broadway,
//  which is the thing worth learning. This walks every card that can still come
//  at a chance node and reports what the player to act does after each one.
//
//  It is exact: every runout was solved, so nothing here is sampled or
//  interpolated. The cost is one node query per card, which on a turn is about
//  the cost of a single iteration for the whole table.
// -----------------------------------------------------------------------------
struct RunoutRow {
    int         slot   = -1;    // deck slot of the card that came
    int         card   = -1;
    std::string name;           // "Ts"
    int         player = 0;     // who acts after it
    double      pot    = 0.0;
    double      ev_oop = 0.0;
    double      ev_ip  = 0.0;
    double      reach  = 0.0;   // % of the acting player's range that gets here
    std::vector<double> freq;   // per action, weighted over the range
    bool        ok     = false;
};

struct RunoutTable {
    bool                     ok = false;
    std::string              note;
    int                      player = 0;        // who acts on the next street
    std::string              street;            // the street being dealt
    std::vector<std::string> codes, labels;     // the actions they choose between
    std::vector<int>         kinds;             // ActionKind, so the UI can colour them
    std::vector<RunoutRow>   rows;
    // Range-wide averages, weighted by how often each runout is actually
    // reached -- a card that blocks most of the range should not count as much
    // as one that blocks none.
    std::vector<double>      avg_freq;
    double                   avg_ev_oop = 0.0;
    double                   avg_ev_ip  = 0.0;
};

inline RunoutTable aggregate_runouts(DCFRSolver& S, int ci, int nid,
                                     const std::vector<int>& dealt) {
    RunoutTable out;
    const GameTree& T = S.tree();
    const Deal&     D = S.deal();
    if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) { out.note = "no such node"; return out; }
    const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
    if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) { out.note = "no such node"; return out; }
    const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
    if (n.type != NT_CONT) { out.note = "not a chance node"; return out; }

    const int child = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
    if (child < 0) { out.note = "no street after this one"; return out; }
    const RoundCtx& crc = T.ctx[static_cast<size_t>(child)];
    const int croot = crc.tree.root;
    const Node& cn = crc.tree.nodes[static_cast<size_t>(croot)];

    out.street = STREET_NAME[crc.street];
    out.player = cn.player;
    for (int a = 0; a < cn.num_actions; ++a) {
        out.codes.push_back(crc.tree.act(cn, a).code);
        out.labels.push_back(crc.tree.act(cn, a).label);
        out.kinds.push_back(static_cast<int>(crc.tree.act(cn, a).kind));
    }
    out.avg_freq.assign(static_cast<size_t>(cn.num_actions), 0.0);

    double wsum = 0.0;
    for (int slot = 0; slot < static_cast<int>(D.deck.size()); ++slot) {
        bool taken = false;
        for (int s : dealt) if (s == slot) { taken = true; break; }
        if (taken) continue;

        std::vector<int> path = dealt;
        path.push_back(slot);
        long long inst = 0;
        int perm = 0;
        RunoutRow row;
        row.slot = slot;
        row.card = D.deck[static_cast<size_t>(slot)];
        row.name = card_str(row.card);
        if (!D.locate(path, inst, perm)) { out.rows.push_back(row); continue; }

        const NodeStats N = gather(S, child, croot, inst, perm);
        if (!N.ok) { out.rows.push_back(row); continue; }

        row.ok     = true;
        row.player = N.player;
        row.pot    = N.pot;
        const double ev = N.node_ev();
        node_ev_both(S, child, croot, inst, N.player, ev, row.ev_oop, row.ev_ip);
        row.reach  = node_reach_pct(S, N);
        for (int a = 0; a < N.A; ++a) row.freq.push_back(N.node_freq(a));

        const double w = N.wtot;
        wsum += w;
        for (int a = 0; a < N.A && a < static_cast<int>(out.avg_freq.size()); ++a)
            out.avg_freq[static_cast<size_t>(a)] += w * row.freq[static_cast<size_t>(a)];
        out.avg_ev_oop += w * row.ev_oop;
        out.avg_ev_ip  += w * row.ev_ip;
        out.rows.push_back(row);
    }

    if (wsum > 1e-12) {
        for (double& f : out.avg_freq) f /= wsum;
        out.avg_ev_oop /= wsum;
        out.avg_ev_ip  /= wsum;
    }
    out.ok = true;
    return out;
}

// Console rendering: one line per card, highest first.
inline void report_runouts(DCFRSolver& S, int ci, int nid, const std::vector<int>& dealt) {
    const RunoutTable t = aggregate_runouts(S, ci, nid, dealt);
    if (!t.ok) { std::fprintf(cfg::out, "  %s\n", t.note.c_str()); return; }

    std::fprintf(cfg::out, "\n  Every %s card, and what %s does with it\n\n",
                 t.street.c_str(), t.player == 0 ? "OOP" : "IP");
    std::fprintf(cfg::out, "    card   reach");
    for (const std::string& c : t.codes) std::fprintf(cfg::out, " %8s", c.c_str());
    std::fprintf(cfg::out, "    EV OOP    EV IP\n");

    std::vector<const RunoutRow*> sorted;
    for (const RunoutRow& r : t.rows) if (r.ok) sorted.push_back(&r);
    std::sort(sorted.begin(), sorted.end(),
              [](const RunoutRow* a, const RunoutRow* b) { return a->card > b->card; });

    for (const RunoutRow* r : sorted) {
        std::fprintf(cfg::out, "    %-4s %6.1f%%", r->name.c_str(), r->reach);
        for (double f : r->freq) std::fprintf(cfg::out, " %7.1f%%", 100.0 * f);
        std::fprintf(cfg::out, " %9.3f %8.3f\n", r->ev_oop, r->ev_ip);
    }
    std::fprintf(cfg::out, "    %-4s %6s ", "avg", "");
    for (double f : t.avg_freq) std::fprintf(cfg::out, " %7.1f%%", 100.0 * f);
    std::fprintf(cfg::out, " %9.3f %8.3f\n\n", t.avg_ev_oop, t.avg_ev_ip);
}
