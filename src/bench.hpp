#pragma once
// =============================================================================
//  `solver --bench` -- the performance claims, reproducible.
//
//  Every figure in the README came from me timing things by hand: run this,
//  read that, write it down. Which means nobody else can check them, and I
//  cannot check them again on a different machine without repeating the whole
//  ritual. This is the ritual, written down.
//
//  It reports what a user actually waits for -- building the deal, building the
//  solver, an iteration, reading a node -- rather than microbenchmarks of
//  things nobody waits for. Burst and sustained are both printed: they differ,
//  and quoting the burst number is how you end up promising a speed you do not
//  deliver.
//
//  The iteration counts are FIXED, not chosen to fill a time budget. A budget
//  makes the count depend on how fast the machine is, which makes everything
//  downstream -- the exploitability especially -- depend on it too, and then
//  two runs cannot be compared. A slower machine simply takes longer.
//
//    solver --bench [board]              measure
//    solver --bench --save NAME          measure and keep it
//    solver --bench --vs NAME            measure and say what moved
//
//  What moved is reported in two kinds. Sizes, memory and accuracy are exact
//  and deterministic, so any movement is worth a line. Times are another
//  matter, and it is worth being blunt about how much they are worth:
//
//    Measured on this laptop, against a baseline taken minutes earlier, the
//    tool reliably catches a build with the optimiser turned down (+15% and
//    +17%) and reliably says nothing when the code has not changed. It does
//    NOT resolve an 8.5% regression -- reverting a real optimisation from
//    tonight showed +10% once and vanished into the noise on the next two
//    attempts. Averaging more repeats did not fix it: a machine that has been
//    benchmarking for an hour is thermally saturated, and saturation squeezes
//    the difference between builds as well as the builds themselves.
//
//  So: trust it for the exact figures and for large changes. For anything
//  under about fifteen percent, it will honestly tell you it cannot tell.
// =============================================================================

#include "session.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <system_error>
#include <vector>

class Bench {
public:
    int run(const std::string& board_spec, const std::string& save,
            const std::string& against) {
        using clock = std::chrono::steady_clock;
        auto ms = [](clock::time_point a, clock::time_point b) {
            return std::chrono::duration<double, std::milli>(b - a).count();
        };

        std::printf("\n  solver --bench   %s\n", board_spec.c_str());
        std::printf("  ---------------------------------------------------------\n");

        cfg::POT0 = 20.0;
        cfg::STACK = 100.0;
        cfg::ISO = true;
        cfg::RAKE_PCT = 0.0;
        cfg::RAKE_CAP = 0.0;

        Session S;
        std::string e;

        // A fresh Session already has a flop dealt, and set_board skips the
        // deal when the board has not changed -- so timing it straight would
        // report zero for the most expensive thing here. Move away first.
        if (!S.set_board("2c3d4h5s7c", e)) { std::printf("  board: %s\n\n", e.c_str()); return 1; }
        const auto t0 = clock::now();
        if (!S.set_board(board_spec, e)) { std::printf("  board: %s\n\n", e.c_str()); return 1; }
        const auto t1 = clock::now();

        if (!S.set_range(0, "22+,A2s+,K9s+,Q9s+,J9s+,T9s,98s,ATo+,KJo+", e) ||
            !S.set_range(1, "22+,A2s+,K9s+,QTs+,JTs,T9s,AJo+,KQo", e)) {
            std::printf("  ranges: %s\n\n", e.c_str());
            return 1;
        }
        for (int st = 0; st < 3; ++st)
            if (!S.set_sizings(false, st, std::vector<Sizing>(), e) ||
                !S.set_allin(st, false, e) ||
                !S.set_donks(st, std::vector<Sizing>(), e)) {
                std::printf("  tree: %s\n\n", e.c_str()); return 1; }

        std::map<std::string, double> r;
        const Session::MemUse m = S.memory_use();
        r["combos.oop"]   = S.live_combos(0);
        r["combos.ip"]    = S.live_combos(1);
        r["nodes"]        = static_cast<double>(S.tree().num_instanced_nodes());
        r["suitgroup"]    = static_cast<double>(S.deal().base_group.size());
        r["mem.total"]    = m.total;
        r["mem.buffers"]  = m.buffers;
        r["mem.tables"]   = m.tables;
        r["mem.frames"]   = m.scratch;
        r["ms.deal"]      = ms(t0, t1);
        r["ms.reference"] = reference_ms();

        const auto t2 = clock::now();
        S.solve(1, 0);
        const auto t3 = clock::now();
        if (!S.solved()) { std::printf("  build failed\n\n"); return 1; }
        r["ms.build"] = ms(t2, t3);

        const int burst = counts(S, true), sust = counts(S, false);
        // Five short runs, keeping the fastest. One run of a couple of
        // seconds carries whatever the machine did during those seconds; the
        // minimum of several is the closest cheap approximation to an
        // undisturbed measurement, and it is what makes a comparison between
        // two builds mean anything at all.
        double bbest = 1e30;
        for (int rep = 0; rep < 5; ++rep) {
            const auto t4 = clock::now();
            S.iterate(burst, 0);
            const auto t5 = clock::now();
            bbest = std::min(bbest, ms(t4, t5) / burst);
        }
        r["ms.iter.burst"] = bbest;

        // Five blocks, keeping the fastest, and the spread of the five
        // reported next to it. A single timing carries whatever the machine
        // was doing during it, and the minimum of repeats is the cheapest
        // thing close to the undisturbed number.
        //
        // Five rather than two because two was not enough to tell anything:
        // measuring the same binary from separate launches came back 64.6,
        // 66.8 and 68.0 on the same machine, a 5% spread, while the changes
        // worth trying are 2-3%. A number you cannot resolve is not a
        // measurement, so the spread is now printed -- if it is wider than the
        // difference between two builds, the comparison says nothing and the
        // tool should say so rather than let somebody read a win into noise.
        const int blocks = 5;
        const int per = (sust * 2) / blocks;      // same total work as before
        double best = 1e30, worst = 0.0;
        for (int rep = 0; rep < blocks; ++rep) {
            const auto t6 = clock::now();
            S.iterate(per, 0);
            const auto t7 = clock::now();
            const double one = ms(t6, t7) / per;
            best  = std::min(best, one);
            worst = std::max(worst, one);
        }
        r["ms.iter.sustained"] = best;
        r["pct.iter.spread"]   = 100.0 * (worst / best - 1.0);
        r["iters"] = 1 + 5 * burst + blocks * per;

        r["ms.node.root"] = node_ms(S, 0);
        int deep = -1;
        for (size_t c = 1; c < S.tree().ctx.size(); ++c)
            if (S.tree().ctx[c].instances > 1) { deep = static_cast<int>(c); break; }
        r["ms.node.deep"] = (deep >= 0) ? node_ms(S, deep) : 0.0;

        const auto t8 = clock::now();
        // A proposito en la convencion de la SUMA y no en la de la referencia: esto es
        // una linea base de desarrollo que se compara contra tiradas guardadas
        // con `bench --against`, y cambiarle la escala invalidaria las viejas.
        // Lo que el usuario lee pasa por expl_shown(); esto no lo lee el usuario.
        r["exploitability"] = S.solver()->exploitability();
        const auto t9 = clock::now();
        r["ms.exploitability"] = ms(t8, t9);

        report(S, r, burst, sust);

        int rc = 0;
        if (!against.empty()) {
            std::map<std::string, double> base;
            if (!load(path_of(against), base)) {
                std::printf("  no baseline called '%s' to compare against\n\n", against.c_str());
                rc = 1;
            } else {
                compare(base, r, against);
            }
        }
        if (!save.empty()) {
            if (store(path_of(save), r))
                std::printf("  kept as '%s' -- compare later with --bench --vs %s\n\n",
                            save.c_str(), save.c_str());
            else
                std::printf("  could not write the baseline\n\n");
        }
        return rc;
    }

private:
    // A fixed lump of work, timed. It measures nothing about the solver --
    // it measures the machine right now: clocks, thermal state, whatever
    // else is running. Comparing two runs taken in different machine states
    // is comparing nothing, and without this the tool cannot notice.
    //
    // It has to contend for what the solver contends for. The first version
    // was single-threaded scalar arithmetic, and it sat there reporting a
    // steady 17 ms while the solver itself ran four times slower under load:
    // one core doing register maths keeps its clocks whatever else is
    // happening. So this streams a buffer from every core instead, which is
    // memory bandwidth and core availability -- the two things a traversal
    // is actually waiting for.
    static double reference_ms() {
        using clock = std::chrono::steady_clock;
        int nt = static_cast<int>(std::thread::hardware_concurrency());
        if (cfg::THREADS > 0) nt = cfg::THREADS;
        if (nt < 1) nt = 1;
        if (nt > 64) nt = 64;
        const size_t N = 2u << 20;                 // 8 MB of floats per thread
        std::vector<std::vector<float>> buf(static_cast<size_t>(nt));
        for (int t = 0; t < nt; ++t) buf[static_cast<size_t>(t)].assign(N, 1.0f);
        std::vector<double> out(static_cast<size_t>(nt), 0.0);
        const auto a = clock::now();
        {
            std::vector<std::thread> th;
            for (int t = 0; t < nt; ++t)
                th.push_back(std::thread([&buf, &out, t, N]() {
                    float* p = buf[static_cast<size_t>(t)].data();
                    double acc = 0.0;
                    for (int pass = 0; pass < 24; ++pass) {
                        for (size_t k = 0; k < N; ++k) p[k] = p[k] * 1.0000001f + 1e-7f;
                        acc += p[pass];
                    }
                    out[static_cast<size_t>(t)] = acc;
                }));
            for (std::thread& x : th) x.join();
        }
        const double took = std::chrono::duration<double, std::milli>(clock::now() - a).count();
        double sink = 0.0;
        for (double v : out) sink += v;
        return (sink > -1e308) ? took : took;   // keep the work
    }

    static std::string path_of(const std::string& name) {
        return exe_dir() + "/saves/bench/" + name + ".txt";
    }

    // Fixed counts, sized so a flop takes roughly half a minute here. A slower
    // machine takes longer and measures exactly the same thing, which is why
    // they are counts and not a time budget.
    static int counts(Session& S, bool burst) {
        const long long n = S.tree().num_instanced_nodes();
        if (n > 100000) return burst ? 40 : 400;
        if (n > 10000)  return burst ? 200 : 2500;
        return burst ? 2000 : 60000;
    }

    static double node_ms(Session& S, int ci) {
        using clock = std::chrono::steady_clock;
        const int root = S.tree().ctx[static_cast<size_t>(ci)].tree.root;
        NodeView v;
        S.solver()->query(ci, root, 0, v);
        const auto a = clock::now();
        for (int k = 0; k < 5; ++k) S.solver()->query(ci, root, 0, v);
        const auto b = clock::now();
        return std::chrono::duration<double, std::milli>(b - a).count() / 5.0;
    }

    static void report(Session& S, std::map<std::string, double>& r, int burst, int sust) {
        std::printf("  spot        %.0f live combos OOP, %.0f IP, %.0f instanced nodes\n",
                    r["combos.oop"], r["combos.ip"], r["nodes"]);
        std::printf("  suits       %s (board group %.0f)\n",
                    S.deal().iso_on ? "collapsed" : "not collapsed", r["suitgroup"]);
        std::printf("  memory      %.3f GB  =  %.3f buffers + %.3f tables + %.3f frames + rest\n",
                    r["mem.total"], r["mem.buffers"], r["mem.tables"], r["mem.frames"]);
        std::printf("  deal built  %.0f ms   solver built %.0f ms\n",
                    r["ms.deal"], r["ms.build"]);
        std::printf("  iteration   %.2f ms over %d   %.2f ms over %d   %+.0f%% sustained\n",
                    r["ms.iter.burst"], burst, r["ms.iter.sustained"], (sust * 2) / 5 * 5,
                    100.0 * (r["ms.iter.sustained"] / r["ms.iter.burst"] - 1.0));
        // What the number is worth. A difference between two builds that is
        // smaller than this is not a difference.
        std::printf("  spread      %.1f%% between the fastest and slowest block of five\n",
                    r["pct.iter.spread"]);
        std::printf("  node read   %.1f ms at the root, %.1f ms a street down\n",
                    r["ms.node.root"], r["ms.node.deep"]);
        std::printf("  accuracy    %.4f%% of the pot per hand after %.0f iterations  (%.0f ms)\n",
                    100.0 * r["exploitability"] / cfg::POT0, r["iters"], r["ms.exploitability"]);
        std::printf("  ---------------------------------------------------------\n");
        std::printf("  %d threads.  Reference loop %.0f ms -- that is the machine today,\n"
                    "  not the solver.  Sizes, memory and accuracy do not move with it.\n",
                    S.solver()->threads(), r["ms.reference"]);
        // Hard-won, and the opposite of what you would expect: do NOT divide
        // the iteration time by the reference loop to compare two builds.
        // Measured over four launches of each of two binaries, the raw
        // sustained minimum spread 2.3% while the same numbers divided by
        // the reference spread 7%. The reference says what kind of day the
        // machine is having; its own noise is larger than the differences
        // worth chasing, so dividing by it buries them.
        std::printf("  To compare two builds: run each a few times and take the LOWEST\n"
                    "  sustained ms. Do not normalise by the reference loop -- its own\n"
                    "  noise is wider than what you are looking for. A difference\n"
                    "  smaller than the spread above is not a difference.\n\n");
    }

    static bool store(const std::string& path, const std::map<std::string, double>& r) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        std::ofstream f(path.c_str());
        if (!f) return false;
        f.precision(12);
        for (std::map<std::string, double>::const_iterator it = r.begin(); it != r.end(); ++it)
            f << it->first << ' ' << it->second << '\n';
        return true;
    }
    static bool load(const std::string& path, std::map<std::string, double>& r) {
        std::ifstream f(path.c_str());
        if (!f) return false;
        std::string k;
        double v;
        while (f >> k >> v) r[k] = v;
        return !r.empty();
    }

    static void compare(const std::map<std::string, double>& base,
                        const std::map<std::string, double>& now,
                        const std::string& name) {
        std::printf("  against '%s'\n", name.c_str());
        std::printf("  ---------------------------------------------------------\n");
        int moved = 0;
        bool trust_times = true;
        {
            std::map<std::string, double>::const_iterator a = base.find("ms.reference"),
                                                          b = now.find("ms.reference");
            if (a != base.end() && b != now.end() && a->second > 1e-9) {
                const double d = 100.0 * (b->second / a->second - 1.0);
                if (std::fabs(d) >= 10.0) {
                    trust_times = false;
                    std::printf("  ! the machine is %.0f%% %s than when the baseline was taken\n"
                                "    (reference loop %.0f ms -> %.0f ms). Every time below moves\n"
                                "    with that, so they are shown but mean little. Sizes,\n"
                                "    memory and accuracy are unaffected and still exact.\n\n",
                                std::fabs(d), d > 0 ? "slower" : "faster",
                                a->second, b->second);
                }
            }
        }
        // Exact first: deterministic, so any movement at all is a real change.
        // Exploitability is deterministic for a given build, but -O1 and
        // -O3 -march=native do not associate floating point the same way and
        // differ in the twelfth digit. A real change to the strategy moves it
        // enormously more than that.
        const char* exact[] = { "combos.oop", "combos.ip", "nodes", "suitgroup",
                                "mem.total", "mem.buffers", "mem.tables", "mem.frames",
                                "iters", "exploitability" };
        for (int i = 0; i < 10; ++i) {
            const std::string k = exact[i];
            std::map<std::string, double>::const_iterator a = base.find(k), b = now.find(k);
            if (a == base.end() || b == now.end()) continue;
            const double tol = (k == "exploitability") ? 1e-6 : 1e-9;
            if (std::fabs(a->second - b->second) <= tol * std::max(1.0, std::fabs(a->second)))
                continue;
            std::printf("  %-18s %12.6g  ->  %-12.6g\n", k.c_str(), a->second, b->second);
            ++moved;
        }
        // Then times -- but only the two that are worth comparing. The
        // iteration figures are averages over thousands of repeats, and the
        // sustained one is the faster of two runs, so they settle. Building
        // the deal, building the solver, reading a node and measuring
        // accuracy all happen once or a handful of times, and on a laptop
        // they wander 10-20% between identical runs: flagging those reported
        // a change almost every time, which is worse than reporting nothing.
        // They are printed above; they are not judged here.
        struct Timed { const char* key; double band; };
        const Timed timed[] = {
            { "ms.iter.burst",      6.0 },
            { "ms.iter.sustained",  5.0 },
            { "pct.iter.spread",  1e9 },   // reported, never a regression itself
        };
        for (int i = 0; i < 2; ++i) {
            const std::string k = timed[i].key;
            std::map<std::string, double>::const_iterator a = base.find(k), b = now.find(k);
            if (a == base.end() || b == now.end() || a->second <= 1e-9) continue;
            const double d = 100.0 * (b->second / a->second - 1.0);
            if (std::fabs(d) < (trust_times ? timed[i].band : 1e9)) continue;
            std::printf("  %-18s %8.2f ms  ->  %-8.2f ms   %+.0f%%%s\n", k.c_str(),
                        a->second, b->second, d, d > 0 ? "  slower" : "  faster");
            ++moved;
        }
        if (!moved)
            std::printf("  nothing moved beyond the noise.\n");
        else if (!trust_times)
            std::printf("  (times withheld: the machine is not in the same state)\n");
        std::printf("  ---------------------------------------------------------\n\n");
    }
};
