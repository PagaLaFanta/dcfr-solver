// Multi-street engine check.
//   g++ -std=c++17 -O3 -march=native -Isrc -static -o test_solve src/test_solve.cpp
#include "solver.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

static double secs(std::chrono::steady_clock::time_point a,
                   std::chrono::steady_clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

static void run_case(const char* boardspec, const char* oopspec, const char* ipspec,
                     int iters, bool show_tree) {
    std::printf("\n=============================================================\n");
    std::printf("  BOARD %s\n", boardspec);
    std::printf("=============================================================\n");

    std::vector<int> b;
    std::string e;
    if (!parse_board(boardspec, b, e)) { std::printf("  board: %s\n", e.c_str()); return; }

    Deal d;
    auto t0 = std::chrono::steady_clock::now();
    if (!d.build(b, e)) { std::printf("  deal: %s\n", e.c_str()); return; }
    auto t1 = std::chrono::steady_clock::now();
    std::printf("  street %-5s  combos %d  deck %d  runouts %d  (tables %.2fs)\n",
                STREET_NAME[d.start], d.num(), d.deckN(), d.num_runouts, secs(t0, t1));

    std::vector<double> oop, ip;
    if (!parse_range(oopspec, d, oop, e)) { std::printf("  oop: %s\n", e.c_str()); return; }
    if (!parse_range(ipspec, d, ip, e))   { std::printf("  ip: %s\n", e.c_str()); return; }
    int lo = 0, li = 0;
    for (double x : oop) if (x > 0) ++lo;
    for (double x : ip)  if (x > 0) ++li;
    std::printf("  ranges: OOP %d combos, IP %d combos\n", lo, li);

    d.build_orbits(oop, ip, cfg::ISO);
    // The denominator is how many runouts there actually are, which the deal
    // already knows. It used to be hardcoded as 49*49 for a flop, counting the
    // turn and river being the same card.
    std::printf("  isomorphism: %s (board group %d, runout instances %lld of %d)\n",
                d.iso_on ? "on" : "off", static_cast<int>(d.base_group.size()),
                d.inst[d.start == ST_FLOP ? 2 : (d.start == ST_TURN ? 1 : 0)],
                d.num_runouts);

    TreeConfig tc;
    TreeBuilder tb(tc, d);
    GameTree T = tb.build();
    if (!tb.ok()) { std::printf("  tree: %s\n", tb.error().c_str()); return; }
    std::printf("  tree: %d contexts, %d template nodes, %lld instanced nodes\n",
                static_cast<int>(T.ctx.size()), T.num_decision_nodes(), T.num_instanced_nodes());
    std::printf("  memory if every combo were stored: %.3f GB   max depth %d\n",
                T.total_gb(), T.max_depth);

    if (T.total_gb() > 6.0) {
        std::printf("  SKIPPED: too big for this check\n");
        return;
    }
    if (show_tree) print_tree(T, d);

    auto t2 = std::chrono::steady_clock::now();
    DCFRSolver S(T, d, oop, ip);
    auto t3 = std::chrono::steady_clock::now();
    std::printf("  solver ready in %.2fs, %d threads\n", secs(t2, t3), S.threads());
    std::printf("  memory actually laid out: %.3f GB (regret + strategy, float)\n",
                T.total_gb());

    auto t4 = std::chrono::steady_clock::now();
    S.run(iters, 0);
    auto t5 = std::chrono::steady_clock::now();

    const double ev0 = S.root_ev(0);
    const double ev1 = S.root_ev(1);
    std::printf("  %d iterations in %.2fs  (%.1f ms/iter)\n",
                iters, secs(t4, t5), 1000.0 * secs(t4, t5) / iters);
    {
        const double a = S.t_traverse(), b = S.t_discount(), tot = a + b;
        std::printf("  breakdown: traversal %.2fs (%.0f%%)   discount %.2fs (%.0f%%)\n",
                    a, tot > 0 ? 100.0 * a / tot : 0.0,
                    b, tot > 0 ? 100.0 * b / tot : 0.0);
    }
    std::printf("  game value: OOP %.4f  IP %.4f   sum %.4f (pot %.1f)\n",
                ev0, ev1, ev0 + ev1, cfg::POT0);

    auto t6 = std::chrono::steady_clock::now();
    const double expl = S.exploitability();
    auto t7 = std::chrono::steady_clock::now();
    std::printf("  exploitability %.5f chips (%.3f%% pot)  [%.2fs]\n",
                expl, 100.0 * expl / cfg::POT0, secs(t6, t7));

    // The two players' EVs must add up to the pot at EVERY node, not just the
    // root: the pairwise weights are symmetric and the utilities are zero sum.
    {
        std::printf("  EV split per node (OOP + IP must equal the pot):\n");
        int checked = 0;
        for (size_t ci = 0; ci < T.ctx.size() && checked < 6; ++ci) {
            const RoundCtx& rc = T.ctx[ci];
            for (size_t ni = 0; ni < rc.tree.nodes.size() && checked < 6; ++ni) {
                if (rc.tree.nodes[ni].type != NT_DECISION) continue;
                const double a = S.node_ev_player(static_cast<int>(ci), static_cast<int>(ni), 0, 0);
                const double b = S.node_ev_player(static_cast<int>(ci), static_cast<int>(ni), 0, 1);
                std::printf("    %-22s %-12s OOP %8.4f  IP %8.4f  sum %8.4f\n",
                            rc.label.c_str(), rc.tree.nodes[ni].path.c_str(), a, b, a + b);
                ++checked;
            }
        }
    }

    // There used to be a table here of the root strategy by "hand type" --
    // nuts, value, bluff-catcher, air, cut at three equity percentages. Those
    // buckets are gone: they were this solver's invention, the cuts were three
    // numbers somebody picked, and every row was really a statement about those
    // numbers rather than about the hand.
}

int main(int argc, char** argv) {
    const std::string which = (argc > 1) ? argv[1] : "all";
    if (argc > 3 && std::string(argv[3]) == "noiso") cfg::ISO = false;
    int nit = 0;
    if (argc > 2) nit = std::atoi(argv[2]);
    const char* OOP = "22+,A2s+,K9s+,Q9s+,J9s+,T9s,98s,ATo+,KJo+";
    const char* IP  = "22+,A2s+,K9s+,QTs+,JTs,T9s,AJo+,KQo";

    if (which == "all" || which == "river")
        run_case("Ah9h4hKd2s", OOP, IP, nit ? nit : 2000, false);
    if (which == "all" || which == "turn")
        run_case("Ah9h4hKd", OOP, IP, nit ? nit : 800, false);
    if (which == "all" || which == "flop")
        run_case("Ah9h4h", OOP, IP, nit ? nit : 200, false);
    if (which == "tree")
        run_case("Ah9h4h", OOP, IP, 1, true);
    return 0;
}
