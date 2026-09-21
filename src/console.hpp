#pragma once
// =============================================================================
//  Interactive console: a thin REPL over Session.
//  Same grammar whether typed, piped, or fed with --script.
// =============================================================================

#include "session.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// El texto de `help`, aparte y como dato, por dos razones.
//
// Se imprime con fputs y NO con printf: no es un formato, asi que los por
// ciento van solos. MEDIDO: una linea decia "less than this % of the pot" dentro
// de un printf sin argumentos, y el "% o" se leia como una conversion a octal --
// en pantalla salia "less than this 25235616201f the pot". Lo primero que lee
// quien abre la consola, y llevaba ahi quien sabe cuanto.
//
// Y como dato se puede comprobar: una comprobacion lee este texto y se planta si
// vuelve a aparecer un %% (que ahora saldria tal cual en pantalla) o una tirada
// de digitos que solo puede venir de un por ciento comido.
inline const char* const CONSOLE_HELP =
"\n  SPOT\n"
"    show                       board, ranges, sizings, tree size, locks\n"
"    lines [file]               every line of the tree, in the usual notation\n"
"    freqs [n|all|file]        how often each line is reached, over every runout\n"
"    made                       the range here grouped by made hand\n"
"    br [n]                     the best response here: how a perfect\n"
"                               exploiter answers this node\n"
"    board <cards>              3 cards = flop solve, 4 = turn, 5 = river\n"
"                               e.g. `board Ah9h4h`  or  `board Ah9h4hKd2s`\n"
"    range oop|ip <spec>        e.g. `range ip QQ+,AKs,A5s:0.5`  or  `range oop random`\n"
"    set pot|stack <x>\n"
"    set bets   [oop|ip] [street] <pct,..>\n"
"    set raises [oop|ip] [street] <Nx|min,..>\n"
"                               Sizings in per cent of the pot: 30 is a third\n"
"                               of it and 300 is three pots. Both the\n"
"                               player and the street may be left out, and\n"
"                               each one left out means all of them.\n"
"                               Raises are different: ONLY N times what\n"
"                               you have to call, on top of what you have in\n"
"                               -- `3x`, `2,5x` -- or `min`, which is the\n"
"                               smallest legal raise and comes out the same\n"
"                               as `2x`. That is the standard count.\n"
"                               `none` removes the action: a street with no\n"
"                               raise sizes has no raises, and there is no\n"
"                               separate cap -- the chain ends where the\n"
"                               all-in threshold ends it\n"
"    set donks [street] <pct,..> OOP leading into last street's aggressor.\n"
"                               Its own sizes, not the bet ones. Not on the\n"
"                               street the solve starts on: nobody was\n"
"                               aggressive before it\n"
"    set no3bet [street] on|off  IP does not make the third aggressive action\n"
"                               of the street (IP bets, OOP raises, IP stops)\n"
"    set allin [oop|ip] [street] on|off   add a bare all-in branch\n"
"    set allinpct <f>            a bet committing more than f of the starting\n"
"                               stack becomes an all-in. The usual rule\n"
"                               and the usual value: 0.67 or 67%.\n"
"                               1 turns it off\n"
"    set rake <pct>[,<cap>]      house cut of the matched pot; 0 turns it off\n"
"    set maxmem <gb>             refuse to build a tree bigger than this\n"
"    set threads <n>             0 = every core; lower it to keep some free\n"
"    set iso on|off              collapse runouts equivalent under a suit swap\n"
"    set accuracy <pct>          stop when exploitable for less than this % of the pot\n"
"    set stopacc on|off          whether that target stops the solve at all\n"
"    set iters <n> | alpha|beta|gamma <x> | eq <n,v,b>\n"
"    build | estimate           rebuild, or just report the tree size\n"
"    echo <text>                write a line in the log, with the time of day.\n"
"                               For the many-board scripts: by morning the log\n"
"                               is a thousand lines and you want to know which\n"
"                               board it was on, and when\n"
"    flops [n]                  n random flops, one per line, no two of them the\n"
"                               same board with the suits renamed. To paste into\n"
"                               a script and leave it running\n"
"    set timeout <secs>         stop a solve after that long, even if it has not\n"
"                               reached the accuracy target. 0 = no limit\n"
"    runouts                    at a chance node: every card that can come,\n"
"                               with the next street's frequencies and EV\n"
"\n  SAVING\n"
"    saves                      list saved configs and saved trees\n"
"    save range <name>          keep BOTH ranges, OOP and IP, to reuse in\n"
"                               another spot: a range is a spot, not a side\n"
"    save config <name>         the setup only: board, ranges, sizings (a few KB)\n"
"    save tree <name>           the setup AND the solution, resumable (big)\n"
"    load config|tree <name>    restore one\n"
"    delete config|tree <name>  remove one\n"
"\n  SOLVING\n"
"    solve [n]                  fresh solve\n"
"    iterate <n>                more iterations on the current solution\n"
"    expl                       exploitability (a full extra traversal)\n"
"\n  NAVIGATION\n"
"    tree | ls | pwd | up\n"
"    cd <action>                follow an action, e.g. `cd B12`\n"
"    cd <card>                  at a chance node, deal that card: `cd Ts`\n"
"    cd /                       back to the start of the tree\n"
"\n  READING\n"
"    freq                       action frequencies here\n"
"    hands [n]                  per starting-hand class (default 25, 0 = all)\n"
"    combos [n]                 per individual combo, strongest first\n"
"    grid [code]                13x13 matrix of one action's frequency\n"
"    report [file] | csv <file>\n"
"\n  NODELOCKING\n"
"    lock <sel> <act=p,...>     e.g. `lock AIR C=1`, `lock QQ+ B=0.5,X=0.5`\n"
"    unlock [all] | locks\n"
"    A lock belongs to the exact node you stand on, DEALT CARD AND ALL --\n"
"    locking the 3s river does not lock the 3h. So `cd` to\n"
"    the runout you mean first. Selectors: NUTS VALUE BC AIR, a made-hand\n"
"    or draw CATEGORY -- `lock two_pair B=90%`, `lock flush_draw C=1` --\n"
"    or any range expression at all.\n"
"    Actions: F X C B R, or the exact code when the street has two sizes\n"
"    -- `B33=0.9` -- which is the only way to say which one.\n"
"    Takes effect on solve.\n"
"\n  quit\n";

class Console {
public:
    explicit Console(Session& s) : S(s) {}

    int run(std::istream& in, bool interactive) {
        if (interactive) banner();
        std::string line;
        while (true) {
            if (interactive) {
                std::printf("\n%s> ", S.prompt().c_str());
                std::fflush(stdout);
            }
            if (!std::getline(in, line)) break;
            // Editors save UTF-8 with a BOM and PowerShell prepends one when
            // piping; either way it would poison the first command.
            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF)
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (!interactive) std::printf("\n%s> %s\n", S.prompt().c_str(), line.c_str());
            if (!dispatch(line)) break;
        }
        return 0;
    }

private:
    Session& S;

    static void banner() {
        std::printf(
"#############################################################################\n"
"#   DCFR SOLVER  --  flop, turn or river, 52 cards                          #\n"
"#   `help` for commands, `solve` to run, `quit` to exit.                    #\n"
"#############################################################################\n");
    }
    void err(const std::string& m) const { std::printf("  ! %s\n", m.c_str()); }
    void ok(const std::string& m)  const { std::printf("  %s\n", m.c_str()); }

    bool need_solution() const {
        if (S.solved()) return true;
        std::printf("  ! no solution yet -- run `solve` first\n");
        return false;
    }
    NodeStats here() {
        long long inst = 0;
        int perm = S.deal().identity();
        S.cur_addr(inst, perm);
        // Con equity: todo lo que la consola ensena de un nodo lleva su
        // columna, y tiene que ser la de ese nodo.
        return gather(*S.solver(), S.cur_ctx(), S.cur_node(), inst, perm, true);
    }
    bool need_decision() const {
        if (S.node().type == NT_DECISION) return true;
        std::printf("  ! %s is not a decision node\n", S.where().c_str());
        return false;
    }
    void after_rebuild(bool okflag, const std::string& e) {
        if (!okflag) { err(e); return; }
        print_size();
        if (S.dropped_locks())
            std::printf("  %d lock(s) dropped: their node no longer exists\n", S.dropped_locks());
        if (S.iso_off_by_lock())
            std::printf("  ! suit collapsing is off: a lock names cards, so the suits\n"
                        "    are no longer interchangeable. Correct, but slower.\n");
    }
    void print_size() const {
        const GameTree& T = S.tree();
        const Session::MemUse m = S.memory_use();
        std::printf("  tree: %d contexts, %d template nodes, %lld instanced nodes, "
                    "%.3f GB\n",
                    static_cast<int>(T.ctx.size()), T.num_decision_nodes(),
                    T.num_instanced_nodes(), m.total);
        // Where it goes, because "shrink the sizings" is useless advice when
        // the sizings are not what is costing you.
        std::printf("        regrets+strategy %.3f  ·  sellos %.3f  ·  tablas del "
                    "reparto %.3f\n"
                    "        orden de showdown %.3f  ·  frames por hilo %.3f\n",
                    m.buffers, m.stamps, m.tables, m.sweep, m.scratch);
        // The warning used to sit at a hardcoded 4 GB while the build refuses
        // at MAX_MEM_GB: it complained about sizes that were fine and stayed
        // quiet about the one that would actually fail.
        if (m.total > cfg::MAX_MEM_GB)
            std::printf("  ! over the %.2f GB limit -- this will not build. %s, "
                        "or `set maxmem`\n", cfg::MAX_MEM_GB,
                        m.buffers > 0.6 * m.total
                            ? "Use fewer sizings, or drop a street's raises"
                            : "The betting tree is not what costs here -- narrow a "
                              "range or start a street later");
        else if (m.total > 0.5 * cfg::MAX_MEM_GB)
            std::printf("  ! that is a lot of memory, and the limit is %.2f GB\n",
                        cfg::MAX_MEM_GB);
        // Not a warning, a fact people keep having to deduce: no raise sizes
        // anywhere means no raises anywhere, and an empty field is a quiet way
        // to say it.
        if (S.no_raises_anywhere())
            std::printf("        (no raises in this tree: no street has raise sizes -- "
                        "`set raises turn 3x`)\n");
    }

    // ---------------------------------------------------------------------
    bool dispatch(const std::string& line) {
        const std::vector<std::string> tk = tokenize(line);
        if (tk.empty()) return true;
        const std::string cmd = lower(tk[0]);
        std::string e;

        if (cmd == "quit" || cmd == "exit" || cmd == "q")      return false;
        if (cmd == "help" || cmd == "?")      { cmd_help();      return true; }
        if (cmd == "show" || cmd == "config") { cmd_show();      return true; }
        if (cmd == "board")                   { cmd_board(tk);   return true; }
        if (cmd == "set")                     { cmd_set(tk);     return true; }
        if (cmd == "range")                   { cmd_range(tk);   return true; }
        if (cmd == "build")   { after_rebuild(S.rebuild(e), e); return true; }
        if (cmd == "estimate" || cmd == "size") { print_size();  return true; }
        if (cmd == "tree")    { print_tree(S.tree(), S.deal());  return true; }
        if (cmd == "solve")                   { cmd_solve(tk);   return true; }
        if (cmd == "iterate" || cmd == "it")  { cmd_iterate(tk); return true; }
        if (cmd == "expl")                    { cmd_expl();      return true; }
        if (cmd == "ls")                      { cmd_ls();        return true; }
        if (cmd == "cd")                      { cmd_cd(tk);      return true; }
        if (cmd == "up")   { if (!S.go_up(e)) err(e); else cmd_pwd(); return true; }
        if (cmd == "pwd")                     { cmd_pwd();       return true; }
        if (cmd == "freq") {
            if (need_solution() && need_decision()) report_frequencies(*S.solver(), here());
            return true;
        }
        if (cmd == "hands" || cmd == "strategy") { cmd_hands(tk);  return true; }
        if (cmd == "combos")                     { cmd_combos(tk); return true; }
        if (cmd == "grid")                       { cmd_grid(tk);   return true; }
        if (cmd == "made")   { cmd_made(tk);   return true; }
        if (cmd == "br")     { cmd_br(tk);     return true; }
        if (cmd == "lines")  { cmd_lines(tk);  return true; }
        if (cmd == "freqs")  { cmd_freqs(tk);  return true; }
        if (cmd == "lock")   { cmd_lock(tk);   return true; }
        if (cmd == "unlock") { cmd_unlock(tk); return true; }
        if (cmd == "locks")  { cmd_locks();    return true; }
        if (cmd == "report") { cmd_report(tk); return true; }
        if (cmd == "csv")    { cmd_csv(tk);    return true; }
        if (cmd == "save" || cmd == "load" || cmd == "delete") { cmd_store(cmd, tk); return true; }
        if (cmd == "saves")  { cmd_saves();    return true; }
        if (cmd == "runouts" || cmd == "agg") { cmd_runouts(); return true; }
        if (cmd == "flops")  { cmd_flops(tk);  return true; }
        if (cmd == "echo")   { cmd_echo(line); return true; }

        err("unknown command '" + tk[0] + "'  (try `help`)");
        return true;
    }

    // ---------------------------------------------------------------------
    //  A config is the recipe; a tree is the recipe plus the solved numbers.
    void cmd_store(const std::string& verb, const std::vector<std::string>& tk) {
        if (tk.size() < 3) {
            err(verb + " wants a kind and a name, e.g. `" + verb + " config btn-vs-bb`");
            return;
        }
        const std::string kind = lower(tk[1]);
        if (kind != "config" && kind != "tree" && kind != "range") {
            err("the kind is `config`, `tree` or `range`, not '" + tk[1] + "'");
            return;
        }
        // Un rango guardado son los dos, OOP e IP. El lado solo se nombra al
        // cargar uno de los ficheros de antes, que llevaban un rango suelto.
        if (kind == "range") {
            int who = 0;
            if (tk.size() > 3) {
                const std::string w = lower(tk[3]);
                if (w == "ip") who = 1;
                else if (w != "oop") { err("say `oop` or `ip`, not '" + tk[3] + "'"); return; }
                if (verb == "save") {
                    err("a saved range is the pair now, OOP and IP together: `save "
                        "range " + tk[2] + "` saves both");
                    return;
                }
            }
            std::string e2;
            bool ok2 = false;
            if (verb == "save")        ok2 = S.save_range(tk[2], e2);
            else if (verb == "load")   ok2 = S.load_range(tk[2], who, e2);
            else                       ok2 = S.delete_range(tk[2], e2);
            if (!ok2) { err(e2); return; }
            if (verb == "delete") { ok("deleted range '" + tk[2] + "'"); return; }
            // "save" + "d" sale "saved", pero "load" + "d" sale "loadd". Cada
            // verbo dice su palabra.
            ok(std::string(verb == "save" ? "saved" : "loaded") + " range '" +
               tk[2] + "' (OOP + IP)");
            return;
        }
        const bool tree = (kind == "tree");
        const std::string name = tk[2];
        std::string e;
        bool ok = false;
        if (verb == "save")        ok = tree ? S.save_tree(name, e)   : S.save_config(name, e);
        else if (verb == "load")   ok = tree ? S.load_tree(name, e)   : S.load_config(name, e);
        else                       ok = S.delete_save(tree, name, e);
        if (!ok) { err(e); return; }

        if (verb == "delete") { std::printf("  deleted %s '%s'\n", kind.c_str(), name.c_str()); return; }
        if (verb == "save") {
            const long long b = S.save_size(tree, name);
            std::printf("  saved %s '%s'  (%s)\n", kind.c_str(), name.c_str(),
                        human_size(b).c_str());
            return;
        }
        std::printf("  loaded %s '%s'\n", kind.c_str(), name.c_str());
        if (!S.load_note().empty()) std::printf("  ! %s\n", S.load_note().c_str());
        if (tree) std::printf("  %d iterations already done\n", S.solver()->iterations_done());
        if (!S.locks().empty())
            std::printf("  %d nodelock(s) restored\n", static_cast<int>(S.locks().size()));
        if (S.dropped_locks())
            std::printf("  %d nodelock(s) in the file no longer fit this tree\n",
                        S.dropped_locks());
        print_size();
    }

    static std::string human_size(long long b) {
        char s[48];
        if (b < 0)              std::snprintf(s, sizeof s, "?");
        else if (b < 1024)      std::snprintf(s, sizeof s, "%lld B", b);
        else if (b < 1048576)   std::snprintf(s, sizeof s, "%.1f KB", b / 1024.0);
        else if (b < 1073741824)std::snprintf(s, sizeof s, "%.1f MB", b / 1048576.0);
        else                    std::snprintf(s, sizeof s, "%.2f GB", b / 1073741824.0);
        return std::string(s);
    }

    // Only meaningful standing on a chance node: `cd X` down to one, then this.
    // Escribir una linea en el log, con la hora.
    //
    // Existe por los scripts de varios boards. Una lista de veinte flops
    // deja por la mañana un log de mil lineas, y lo primero que quieres saber
    // es por cual iba y a que hora -- si se murio a las tres, lo que importa
    // es QUE board era y que llevaba hecho.
    void cmd_echo(const std::string& line) {
        const std::string txt = trim(line.substr(4));
        const std::time_t t = std::time(nullptr);
        char hora[16] = "--:--:--";
        std::tm tmv;
#ifdef _WIN32
        if (localtime_s(&tmv, &t) == 0) std::strftime(hora, sizeof hora, "%H:%M:%S", &tmv);
#else
        if (localtime_r(&t, &tmv)) std::strftime(hora, sizeof hora, "%H:%M:%S", &tmv);
#endif
        std::printf("  [%s] %s\n", hora, txt.c_str());
    }

    // Flops al azar, uno por linea, para llenar un script.
    //
    // Sin repetir y SIN EQUIVALENTES: A\u2665 9\u2665 4\u2665 y A\u2660 9\u2660 4\u2660 son el mismo
    // problema con los palos cambiados de nombre, y resolver los dos es pagar dos
    // veces por la misma respuesta. Se guarda el que sale primero.
    void cmd_flops(const std::vector<std::string>& tk) {
        int n = 10;
        if (tk.size() > 1 && (!parse_int(tk[1], n) || n <= 0)) {
            err("usage: flops [how many]");
            return;
        }
        if (n > 500) n = 500;
        std::vector<std::string> salen;
        std::set<std::string> vistos;
        unsigned seed = static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        for (int intentos = 0; intentos < n * 200 && 
             static_cast<int>(salen.size()) < n; ++intentos) {
            int c[3];
            for (int k = 0; k < 3; ++k) {
                bool rep;
                do {
                    seed = seed * 1664525u + 1013904223u;
                    c[k] = static_cast<int>((seed >> 8) % 52u);
                    rep = false;
                    for (int j = 0; j < k; ++j) if (c[j] == c[k]) rep = true;
                } while (rep);
            }
            std::sort(c, c + 3, [](int a, int b) { return a > b; });
            const std::string clave = flop_canon(c[0], c[1], c[2]);
            if (!vistos.insert(clave).second) continue;
            salen.push_back(card_str(c[0]) + card_str(c[1]) + card_str(c[2]));
        }
        for (const std::string& f : salen) std::printf("  %s\n", f.c_str());
        std::printf("  %d flops\n", static_cast<int>(salen.size()));
    }

    void cmd_runouts() {
        if (!need_solution()) return;
        const Node& n = S.node();
        if (n.type != NT_CONT) {
            err("stand on a chance node first -- walk down to where a card is dealt");
            return;
        }
        report_runouts(*S.solver(), S.cur_ctx(), S.cur_node(), S.slots());
    }

    void cmd_saves() {
        for (int k = 0; k < 2; ++k) {
            const bool tree = (k == 1);
            const std::vector<std::string> v = S.list_saves(tree);
            std::printf("\n  %s\n", tree ? "TREES (setup + solucion)" : "CONFIGS (solo el setup)");
            if (v.empty()) { std::printf("    (ninguno)\n"); continue; }
            for (const std::string& n : v)
                std::printf("    %-32s %s\n", n.c_str(),
                            human_size(S.save_size(tree, n)).c_str());
        }
        {
            const std::vector<std::string> v = S.list_ranges();
            std::printf("\n  RANGOS (para reutilizar entre spots)\n");
            if (v.empty()) std::printf("    (ninguno)\n");
            for (const std::string& r : v)
                std::printf("    %-32s %s\n", r.c_str(),
                            human_size(S.range_size(r)).c_str());
        }
        std::printf("\n");
    }

    void cmd_help() const { std::fputs(CONSOLE_HELP, stdout); }

    void cmd_show() const {
        const Deal& D = S.deal();
        std::printf("\n  BOARD   %s   -> %s solve, %d runouts\n",
                    board_str(D.board).c_str(), STREET_NAME[D.start], D.num_runouts);
        std::printf("  COMBOS  %d possible on this board\n", D.num());
        for (int p = 0; p < 2; ++p)
            std::printf("  %-4s    %4d combos, weight %7.2f   %s\n",
                        p == 0 ? "OOP" : "IP", S.live_combos(p), S.range_weight(p),
                        S.range_spec(p).c_str());
        std::printf("\n  SPOT\n");
        std::printf("    pot %.2f    stack %.2f    all-in past %.4g%% of the "
                    "stack\n", cfg::POT0, cfg::STACK, 100.0 * cfg::ALLIN_THRESH);
        if (cfg::RAKE_PCT > 0.0)
            std::printf("    rake %.4g%%%s\n", 100.0 * cfg::RAKE_PCT,
                        cfg::RAKE_CAP > 0.0
                            ? ("  capped at " + fmt_num(cfg::RAKE_CAP)).c_str() : "  uncapped");
        for (int s = 0; s < 3; ++s) {
            if (s < D.start) continue;
            // One line per player: they share nothing per street any more.
            // OOP carries the donk sizes, IP carries "don't 3-bet".
            std::printf("    %s\n", STREET_NAME[s]);
            std::printf("      OOP  bets %-12s raises %-12s donks %-12s allin %s\n",
                        fmt_sizings(S.tc().bets[s][0]).c_str(),
                        fmt_sizings(S.tc().raises[s][0]).c_str(),
                        // Nobody was aggressive before the starting street, so
                        // there is nothing to lead into and no list to read.
                        s == D.start ? "n/a" : fmt_sizings(S.tc().donks[s]).c_str(),
                        S.tc().add_allin[s][0] ? "on" : "off");
            std::printf("      IP   bets %-12s raises %-12s allin %-4s no3bet %s\n",
                        fmt_sizings(S.tc().bets[s][1]).c_str(),
                        fmt_sizings(S.tc().raises[s][1]).c_str(),
                        S.tc().add_allin[s][1] ? "on" : "off",
                        S.tc().no_3bet[s] ? "on" : "off");
        }
        std::printf("    iterations %d   (done: %d)\n", S.iters(),
                    S.solver() ? S.solver()->iterations_done() : 0);
        // The three DCFR discounts, spelled out: three bare Greek letters tell
        // nobody anything, and they are not this solver's invention -- they are
        // the parameters of the algorithm it runs.
        std::printf("    DCFR  alpha %.2f (olvido del arrepentimiento bueno)  "
                    "beta %.2f (del malo)  gamma %.2f (peso de las iteraciones "
                    "viejas)\n",
                    cfg::DCFR_ALPHA, cfg::DCFR_BETA, cfg::DCFR_GAMMA);
        print_size();
        std::printf("  NODE    %s%s\n", S.where().c_str(), S.solved() ? "" : "   (not solved)");
        if (!S.locks().empty()) { std::printf("\n  LOCKS\n"); print_locks(); }
    }

    void print_locks() const {
        for (const LockSpec& L : S.locks()) {
            std::printf("    %-22s %-12s %-6s %-14s ->  ", L.ctx_label.c_str(),
                        L.node_path.c_str(),
                        L.runout.empty() ? "todos" : L.runout.c_str(),
                        L.hand_spec.c_str());
            for (size_t i = 0; i < L.mix.size(); ++i)
                std::printf("%s=%.2f%s", L.mix[i].first.c_str(), L.mix[i].second,
                            i + 1 < L.mix.size() ? " " : "");
            std::printf("\n");
        }
    }

    // ---------------------------------------------------------------------
    void cmd_board(const std::vector<std::string>& tk) {
        if (tk.size() < 2) { err("usage: board <3, 4 or 5 cards>"); return; }
        std::string spec;
        for (size_t i = 1; i < tk.size(); ++i) spec += tk[i];
        std::string e;
        if (!S.set_board(spec, e)) { err(e); return; }
        std::printf("  board %s  -> %s solve, %d combos, %d runouts\n",
                    board_str(S.deal().board).c_str(), STREET_NAME[S.deal().start],
                    S.deal().num(), S.deal().num_runouts);
        // Moving the board can strand a setting that only meant something
        // on the old one, and a setting dropped in silence is the bug this
        // whole file spends its time on.
        if (!S.load_note().empty()) std::printf("  ! %s\n", S.load_note().c_str());
        print_size();
    }

    void cmd_range(const std::vector<std::string>& tk) {
        if (tk.size() < 3) { err("usage: range oop|ip <spec>"); return; }
        const std::string who = lower(tk[1]);
        int p;
        if (who == "oop" || who == "0") p = 0;
        else if (who == "ip" || who == "1") p = 1;
        else { err("first argument must be oop or ip"); return; }
        std::string spec;
        for (size_t i = 2; i < tk.size(); ++i) spec += tk[i];
        std::string e;
        if (!S.set_range(p, spec, e)) { err(e); return; }
        std::printf("  %s: %d combos, weight %.2f -- re-solve to apply\n",
                    p == 0 ? "OOP" : "IP", S.live_combos(p), S.range_weight(p));
    }

    void cmd_set(const std::vector<std::string>& tk) {
        if (tk.size() < 3) { err("usage: set <key> [street] <value>"); return; }
        const std::string key = lower(tk[1]);
        std::string e;
        double d; int i;

        // An optional player and an optional street name may sit between the
        // key and the value, in either order: `set bets ip turn 0.5` and
        // `set bets turn ip 0.5` both read the way somebody would say them.
        int street = -1, player = -1;
        size_t vi = 2;
        for (int pass = 0; pass < 2 && tk.size() > vi + 1; ++pass) {
            const std::string t = lower(tk[vi]);
            const int st = street_of_name(tk[vi]);
            if (st >= 0)                        { street = st; ++vi; }
            else if (t == "oop" || t == "ip")   { player = (t == "ip") ? 1 : 0; ++vi; }
            else break;
        }
        const std::string val = tk[vi];
        // Los tamanos pueden venir separados por espacios, y la consola parte
        // la linea por espacios: quedarse solo con tk[vi] tiraba el resto sin
        // decir nada. `set bets flop 0.6 0.3` daba un tamano, no dos.
        std::string val_all = tk[vi];
        for (size_t k = vi + 1; k < tk.size(); ++k) val_all += " " + tk[k];

        if (key == "pot") {
            if (!parse_double(val, d)) { err("not a number"); return; }
            after_rebuild(S.set_pot(d, e), e);
        } else if (key == "stack") {
            if (!parse_double(val, d)) { err("not a number"); return; }
            after_rebuild(S.set_stack(d, e), e);
        } else if (key == "bets" || key == "raises") {
            std::vector<Sizing> sz;
            const bool leido = (key == "raises") ? parse_raises(val_all, sz, e)
                                                 : parse_sizings(val_all, sz, e);
            if (!leido) { err(e); return; }
            after_rebuild(S.set_sizings_for(key == "bets", player, street, sz, e), e);
        } else if (key == "donks") {
            // OOP only: it is OOP leading into last street's aggressor. There
            // is no IP version of that, so naming a player is a mistake worth
            // catching rather than quietly ignoring.
            if (player == 1) { err("donks are OOP's -- IP never leads into itself"); return; }
            std::vector<Sizing> sz;
            if (!parse_sizings(val_all, sz, e)) { err(e); return; }
            after_rebuild(S.set_donks(street, sz, e), e);
        } else if (key == "no3bet") {
            if (player == 0) { err("`don't 3-bet` is IP's; OOP has no such switch"); return; }
            bool on3 = false;
            if (!parse_onoff(val, on3)) { err("no3bet on|off"); return; }
            after_rebuild(S.set_no3bet(street, on3, e), e);
        } else if (key == "donk") {
            err("`donk` is a list of sizes now, not a switch: `set donks <street> 50` "
                "gives OOP a lead, `set donks <street> none` takes it away");
        } else if (key == "maxraises" || key == "cap") {
            // Gone, and answered rather than rejected as an unknown key: it is
            // in every old script and every old habit, and "unknown key" would
            // not tell anybody what replaced it.
            if (!parse_int(val, i)) { err("not a number"); return; }
            if (i <= 1) {
                const std::vector<Sizing> none;
                if (!S.set_sizings(false, street, none, e)) { err(e); return; }
                // A cap of 0 meant no bet either, not just no raise.
                if (i <= 0 && !S.set_sizings(true, street, none, e)) { err(e); return; }
                ok(i <= 0
                       ? "there is no cap any more, so that is now an empty street: "
                         "no bets and no raises. `set bets <f>` brings it back"
                       : "there is no cap any more, so that is now `set raises none` "
                         "-- the raises are gone. Give the street sizings to bring "
                         "them back");
                after_rebuild(true, e);
            } else {
                err("there is no bet+raise cap any more: the raise sizings decide, "
                    "and the chain ends at the all-in threshold. To remove raises, "
                    "`set raises <street> none`");
            }
        } else if (key == "threads") {
            if (!parse_int(val, i) || i < 0) { err("threads must be >= 0 (0 = auto)"); return; }
            // Solo si cambia: repetir el valor que ya hay no puede costar el solve.
            if (i != cfg::THREADS) { cfg::THREADS = i; S.invalidate(); }
            ok("threads = " + std::string(i ? std::to_string(i) : "auto") +
               "  (re-solve to apply)");
        } else if (key == "iso") {
            bool oniso = false;
            if (!parse_onoff(val, oniso)) {
                err("iso on|off  (`off` doubles the memory and the time: it stops "
                    "collapsing runouts that a suit swap makes identical)");
                return;
            }
            cfg::ISO = oniso;
            after_rebuild(S.rebuild(e), e);
        } else if (key == "allin") {
            bool onai = false;
            if (!parse_onoff(val, onai)) { err("allin on|off"); return; }
            after_rebuild(S.set_allin_for(player, street, onai, e), e);
        } else if (key == "allinpct" || key == "allinthresh" || key == "thresh") {
            // Takes "0.67" or "67%", because it prints as a percentage.
            if (!parse_fraction(val, d)) {
                err("the all-in threshold is a fraction of the starting stack: "
                    "two thirds is '0.67' or '67%'");
                return;
            }
            after_rebuild(S.set_allin_thresh(d, e), e);
        } else if (key == "maxmem") {
            if (!parse_double(val, d) || d <= 0.0) { err("maxmem must be > 0 (GB)"); return; }
            cfg::MAX_MEM_GB = d;
            std::printf("  memory limit %.2f GB\n", cfg::MAX_MEM_GB);
        } else if (key == "rake") {
            // "set rake 0.05" or "set rake 5%", with an optional cap after a
            // comma. Both spellings, because `show` prints the percentage.
            const std::vector<std::string> pc = split(val, ',');
            double pct = 0.0, cap = 0.0;
            if (pc.empty() || !parse_fraction(pc[0], pct)) { err("usage: set rake <pct>[,<cap>]"); return; }
            if (pc.size() > 1 && !pc[1].empty() && !parse_double(pc[1], cap)) {
                err("bad rake cap"); return;
            }
            after_rebuild(S.set_rake(pct, cap, e), e);
        } else if (key == "merge") {
            // Pinned, not offered, and answered rather than left as an unknown
            // key: it was in scripts and it is worth saying what happened.
            err("sizings closer than " + fmt_num(cfg::MERGE_PCT) + " of each other "
                "are collapsed into one, and that is not a knob: below it a "
                "near-duplicate costs a third of the tree for a bet nobody plays, "
                "above it real sizings start disappearing");
        } else if (key == "accuracy" || key == "acc") {
            double a = 0.0;
            if (!parse_double(val, a) || a < 0.0) { err("accuracy is a % of the pot, >= 0"); return; }
            S.set_acc_target(a);
            ok("accuracy target = " + fmt_num(a) + "% of the pot"
               + std::string(S.acc_stop() ? "" : "  (stopping is off)"));
        } else if (key == "timeout" || key == "maxsecs") {
            // El tope de TIEMPO de cada solve. Es lo que hace que una lista de
            // veinte boards quepa en una noche: sin el, el primero que no alcanza
            // la precision se come las horas de los otros diecinueve.
            if (!parse_double(val, d) || d < 0.0) { err("timeout is seconds, >= 0 (0 = none)"); return; }
            S.set_timeout_secs(d);
            ok(d > 0.0 ? ("timeout = " + fmt_num(d) + "s per solve")
                       : std::string("no timeout"));
        } else if (key == "stopacc") {
            bool onstop = false;
            if (!parse_onoff(val, onstop)) { err("stopacc on|off"); return; }
            S.set_acc_stop(onstop);
            ok(std::string("stop at accuracy ") + (S.acc_stop() ? "on" : "off"));
        } else if (key == "iters" || key == "iterations") {
            if (!parse_int(val, i) || i <= 0) { err("iters must be > 0"); return; }
            S.set_iters(i);
            ok("iterations = " + std::to_string(S.iters()));
        } else if (key == "alpha" || key == "beta" || key == "gamma") {
            if (!parse_double(val, d)) { err("not a number"); return; }
            double* dst = (key == "alpha") ? &cfg::DCFR_ALPHA
                        : (key == "beta")  ? &cfg::DCFR_BETA : &cfg::DCFR_GAMMA;
            if (*dst != d) { *dst = d; S.invalidate(); }
            ok("DCFR " + key + " = " + val + "  (re-solve to apply)");
        } else {
            err("unknown key '" + tk[1] + "'  (see `help`)");
        }
    }

    // ---------------------------------------------------------------------
    void cmd_solve(const std::vector<std::string>& tk) {
        int n = S.iters();
        if (tk.size() > 1 && (!parse_int(tk[1], n) || n <= 0)) { err("usage: solve [n]"); return; }
        std::vector<std::string> notes;
        const auto t0 = std::chrono::steady_clock::now();
        S.solve(n, std::max(1, n / 10), &notes);
        const auto t1 = std::chrono::steady_clock::now();
        for (const std::string& m : notes) std::printf("  %s\n", m.c_str());
        // The solver may have refused to build -- too big for the memory limit,
        // or the allocation failed. The notes say why; do not dereference it.
        if (!S.solved()) { err("nothing was solved"); return; }
        const double ev0 = S.solver()->root_ev(0);
        std::printf("\n%s\n",
                    solve_summary(S.solver()->iterations_done(), n,
                                  std::chrono::duration<double>(t1 - t0).count(),
                                  S.solver()->threads(), S.acc_reached(),
                                  S.timeout_reached()).c_str());
        std::printf("  Game value: OOP %.4f   IP %.4f   (pot %.1f)\n",
                    ev0, ((cfg::RAKE_PCT > 0.0) ? S.solver()->root_ev(1) : cfg::POT0 - ev0), cfg::POT0);
    }

    void cmd_iterate(const std::vector<std::string>& tk) {
        if (!S.solver()) { err("nothing to continue -- run `solve` first"); return; }
        int n = S.iters();
        if (tk.size() > 1 && (!parse_int(tk[1], n) || n <= 0)) { err("usage: iterate <n>"); return; }
        const auto t0 = std::chrono::steady_clock::now();
        S.iterate(n, std::max(1, n / 10));
        const auto t1 = std::chrono::steady_clock::now();
        const double secs = std::chrono::duration<double>(t1 - t0).count();
        const double ev0 = S.solver()->root_ev(0);
        std::printf("\n  %d more in %.2fs (%.3f ms/iter)\n", n, secs, 1000.0 * secs / n);
        std::printf("  Game value: OOP %.4f   IP %.4f   (%d iterations total)\n",
                    ev0, ((cfg::RAKE_PCT > 0.0) ? S.solver()->root_ev(1) : cfg::POT0 - ev0), S.solver()->iterations_done());
    }

    void cmd_expl() {
        if (!need_solution()) return;
        const auto t0 = std::chrono::steady_clock::now();
        const double x = expl_shown(S.solver()->exploitability());
        const auto t1 = std::chrono::steady_clock::now();
        std::printf("  exploitability %.5f chips (%.4f%% pot)   [%.2fs]\n",
                    x, 100.0 * x / cfg::POT0, std::chrono::duration<double>(t1 - t0).count());
    }

    // ---------------------------------------------------------------------
    void cmd_pwd() const {
        const Node& n = S.node();
        std::printf("  %s   ", S.where().c_str());
        if (n.type == NT_DECISION)
            std::printf("%s to act, pot %.2f, %d actions\n",
                        n.player == 0 ? "OOP" : "IP", n.pot, n.num_actions);
        else if (n.type == NT_CONT)
            std::printf("chance node: deal the %s\n",
                        STREET_NAME[S.tree().ctx[static_cast<size_t>(S.cur_ctx())].street + 1]);
        else if (n.type == NT_SHOWDOWN) std::printf("showdown, pot %.2f\n", n.pot);
        else std::printf("%s folds, pot %.2f\n", n.player == 0 ? "OOP" : "IP", n.pot);
    }

    void cmd_ls() const {
        const Node& n = S.node();
        const BetTree& bt = S.btree();
        std::printf("  %s\n", S.where().c_str());
        if (n.type == NT_CONT) {
            std::printf("  chance node -- `cd <card>` to deal one of:\n    ");
            int shown = 0;
            for (size_t i = 0; i < S.deal().deck.size(); ++i) {
                bool used = false;
                for (int s : S.slots()) if (s == static_cast<int>(i)) used = true;
                if (used) continue;
                std::printf("%s ", card_str(S.deal().deck[i]).c_str());
                if (++shown % 13 == 0) std::printf("\n    ");
            }
            std::printf("\n");
            return;
        }
        if (n.type != NT_DECISION) { std::printf("  (terminal node)\n"); return; }
        for (int a = 0; a < n.num_actions; ++a) {
            const Node& cn = bt.nodes[static_cast<size_t>(bt.child(n, a))];
            const char* ty = cn.type == NT_DECISION ? "->"
                           : cn.type == NT_CONT     ? "deal next street"
                           : cn.type == NT_SHOWDOWN ? "showdown" : "fold";
            std::printf("    %-8s %-16s %s\n", bt.act(n, a).code.c_str(),
                        bt.act(n, a).label.c_str(), ty);
        }
    }

    void cmd_cd(const std::vector<std::string>& tk) {
        if (tk.size() < 2) { err("usage: cd <action|card|..|/>"); return; }
        std::string e;
        if (!S.go(tk[1], e)) { err(e); return; }
        cmd_pwd();
    }

    // ---------------------------------------------------------------------
    void cmd_hands(const std::vector<std::string>& tk) {
        if (!need_solution() || !need_decision()) return;
        int limit = 25;
        if (tk.size() > 1 && !parse_int(tk[1], limit)) { err("usage: hands [rows]"); return; }
        report_hand_table(*S.solver(), here(), limit);
    }
    void cmd_combos(const std::vector<std::string>& tk) {
        if (!need_solution() || !need_decision()) return;
        int limit = 25;
        if (tk.size() > 1 && !parse_int(tk[1], limit)) { err("usage: combos [rows]"); return; }
        report_combo_table(*S.solver(), here(), limit);
    }
    void cmd_grid(const std::vector<std::string>& tk) {
        if (!need_solution() || !need_decision()) return;
        const NodeStats N = here();
        if (tk.size() > 1) {
            int a = -1;
            for (int i = 0; i < N.A; ++i)
                if (lower(N.codes[static_cast<size_t>(i)]) == lower(tk[1])) { a = i; break; }
            if (a < 0) { err("no action '" + tk[1] + "' here"); return; }
            report_grid(*S.solver(), N, a);
        } else {
            for (int i = 0; i < N.A; ++i) report_grid(*S.solver(), N, i);
        }
    }

    // ---------------------------------------------------------------------
    // Todas las lineas del arbol, en la notacion de la referencia.
    //
    // Sirve para comparar dos arboles con un diff en vez de mirando nodos a
    // mano. Cinco veces esta semana un "desacuerdo entre motores" resulto ser
    // que los dos programas resolvian juegos distintos, y cada vez se fue en
    // horas porque se comprueban los nodos que uno elige y el que difiere esta
    // en los otros seiscientos.
    // El rango de este nodo agrupado por MANO HECHA.
    //
    // La rejilla 13x13 dice que cartas tienes; esto dice que tienes hecho. Es
    // la pregunta que se hace una persona mirando un board -- "con cuanta
    // frecuencia apuesto mis trios" -- y en un board dado los trios estan
    // repartidos por toda la rejilla, asi que ahi no se ven.
    void cmd_made(const std::vector<std::string>& tk) {
        (void)tk;
        // MEDIDO: sin esta linea, `made` recien abierto el programa era un
        // SEGFAULT. here() hace *S.solver() y antes del primer solve no hay
        // solver, asi que el programa se moria de golpe -- sin mensaje, sin
        // nada -- por escribir el segundo comando de la lista del help. El
        // resto de las vistas ya preguntaban primero; esta se quedo fuera.
        if (!need_solution() || !need_decision()) return;
        const NodeStats N = here();
        if (!N.ok) { err("solve first -- there is no strategy to group"); return; }
        const std::vector<ClassAgg> filas =
            aggregate_by_made(*S.solver(), N, S.board_here());
        if (filas.empty()) { err("no combo reaches this node"); return; }

        std::printf("  %s   %s to act\n", S.where().c_str(),
                    N.player == 0 ? "OOP" : "IP");
        const double combos = N.combos();
        std::printf("  %-24s %8s %7s %6s", "categoria", "combos", "% rango", "eq%");
        for (int a = 0; a < N.A; ++a) std::printf(" %7s", N.codes[(size_t)a].c_str());
        std::printf("     %s\n", "EV");
        int visto = -1;
        for (const ClassAgg& g : filas) {
            // Dos bloques, como los tiene la referencia: mano hecha y proyecto. Cada uno
            // reparte el rango entero, y no se cruzan.
            if (g.kind != visto) {
                visto = g.kind;
                std::printf("  %s\n", g.kind == AG_DRAW
                            ? "-- proyecto --" : "-- mano hecha --");
            }
            const double parte = N.wtot > 1e-12 ? g.w / N.wtot : 0.0;
            // Una familia que cabe en el board pero que tu rango no tiene sale
            // igual, a cero, porque eso dice algo. Lo que no puede salir es un
            // "eq 0,0" ni un "EV 0,000" al lado: ahi no hay manos que valgan
            // nada, y un cero se lee como que valen cero.
            const bool vacia = g.w <= 1e-12;
            std::printf("  %-24s %8.2f %6.1f%%", agg_name(g), parte * combos,
                        100.0 * parte);
            if (vacia) {
                std::printf(" %6s", "-");
                for (int a = 0; a < N.A; ++a) std::printf(" %7s", "-");
                std::printf("   %7s\n", "-");
                continue;
            }
            std::printf(" %6.1f", g.eq);
            for (int a = 0; a < N.A; ++a) std::printf(" %6.1f%%", 100.0 * g.freq[(size_t)a]);
            std::printf("   %7.3f\n", g.node_ev);
        }
    }

    // La mejor respuesta en este nodo: como te castigan si te desvias.
    //
    // La solucion dice QUE hacer; esto dice que pasa si no lo haces. Contra la
    // estrategia de la solucion, un rival que juegue para explotarla al maximo
    // elige para cada mano la accion de mas valor -- y sigue explotando calle
    // abajo, no solo en este nodo.
    //
    // Si la solucion esta bien convergida, la mejor respuesta a ella gana muy
    // poco: eso es lo que mide la explotabilidad. Lo que se ve aqui es DONDE
    // esta ese poco.
    void cmd_br(const std::vector<std::string>& tk) {
        if (!need_decision()) return;
        if (!S.solver() || S.solver()->iterations_done() <= 0) {
            err("solve first -- there is no strategy to respond to");
            return;
        }
        long long inst = 0;
        int perm = S.deal().identity();
        S.cur_addr(inst, perm);
        std::vector<double> vals;
        int A = 0;
        if (!S.solver()->br_at(S.cur_ctx(), S.cur_node(), inst, vals, A)) {
            err("that node is never reached, so nothing responds to it");
            return;
        }
        const NodeStats N2 = here();
        if (!N2.ok) { err("could not read the node"); return; }
        const int nh = S.deal().num();
        int cuantas = 12;
        if (tk.size() > 1) parse_int(tk[1], cuantas);

        // Lo que importa no es que accion elige la mejor respuesta -- casi
        // siempre es la obvia -- sino CUANTO GANA con ella sobre lo que dice la
        // solucion. Si la solucion esta bien, esa ganancia es casi cero en casi
        // todas las manos, y donde no lo es, ahi esta el agujero. Por eso se
        // ordena por ganancia ponderada por alcance: donde de verdad esta el
        // dinero, no donde el porcentaje se ve grande sobre una mano que no
        // llega nunca.
        struct Fila { double gana, peso, sol, br; int h, mejor; };
        std::vector<Fila> filas;
        double total = 0.0;
        for (int h = 0; h < nh; ++h) {
            const double w = N2.weight[(size_t)h];
            if (w <= 1e-9) continue;
            const double c = N2.v.compat[(size_t)h];
            if (c <= 1e-12) continue;
            const int st = N2.stored(h);
            int mejor = 0;
            for (int a = 1; a < A; ++a)
                if (vals[(size_t)a * nh + st] > vals[(size_t)mejor * nh + st]) mejor = a;
            Fila f;
            f.h = h;
            f.mejor = mejor;
            f.peso = w;
            // Contrafactual a EV por mano, igual que ev_action: dividir por las
            // manos compatibles del rival y sumar medio bote.
            f.br = vals[(size_t)mejor * nh + st] / c + cfg::POT0 * 0.5;
            f.sol = N2.ev_node(h);
            f.gana = f.br - f.sol;
            total += w * (f.gana > 0.0 ? f.gana : 0.0);
            filas.push_back(f);
        }
        if (filas.empty()) { err("no combo reaches this node"); return; }
        std::sort(filas.begin(), filas.end(), [](const Fila& x, const Fila& y) {
            return x.peso * x.gana > y.peso * y.gana;
        });

        std::printf("  best response at %s\n", S.where().c_str());
        std::printf("  a perfect exploiter gains %.4f chips here, weighted\n\n",
                    total / (N2.wtot > 1e-12 ? N2.wtot : 1.0));
        std::printf("  %-6s %-5s %9s %9s %9s %9s  %s\n",
                    "combo", "", "reach", "solution", "exploit", "gain", "plays");
        int puestas = 0;
        for (const Fila& f : filas) {
            if (cuantas > 0 && puestas++ >= cuantas) break;
            const Combo& k = S.deal().combos[(size_t)f.h];
            std::printf("  %s%s %-5s %9.3f %9.4f %9.4f %+9.4f  %s\n",
                        card_str(k.c1).c_str(), card_str(k.c2).c_str(),
                        class_name(k.cls).c_str(), f.peso, f.sol, f.br, f.gana,
                        N2.codes[(size_t)f.mejor].c_str());
        }
        if (cuantas > 0 && (int)filas.size() > cuantas)
            std::printf("  ... %d more (`br 0` for all)\n",
                        (int)filas.size() - cuantas);
    }

    void cmd_freqs(const std::vector<std::string>& tk) {
        if (!need_solution()) return;
        std::vector<std::pair<std::string, double>> fr;
        S.solver()->line_freqs(fr);
        if (fr.empty()) { err("no lines"); return; }

        // La raiz es la linea mas corta, y es el 100%.
        double raiz = 0.0;
        for (const std::pair<std::string, double>& e : fr)
            if (e.first == "r:0") raiz = e.second;
        if (raiz <= 1e-12) { err("the root is not reachable"); return; }

        // Un argumento numerico es cuantas ensenar; cualquier otro es un
        // fichero csv. "all" las ensena todas.
        double cuantas = 0.0;
        const bool es_num = tk.size() > 1 && parse_double(tk[1], cuantas) && cuantas >= 1.0;
        if (tk.size() > 1 && tk[1] != "all" && !es_num) {
            std::FILE* f = std::fopen(tk[1].c_str(), "wb");
            if (!f) { err("cannot write " + tk[1]); return; }
            std::fprintf(f, "line,freq_pct\n");
            for (const std::pair<std::string, double>& e : fr)
                std::fprintf(f, "%s,%.6f\n", e.first.c_str(), 100.0 * e.second / raiz);
            std::fclose(f);
            std::printf("  %d lines -> %s\n", (int)fr.size(), tk[1].c_str());
            return;
        }

        std::vector<std::pair<double, std::string>> ord;
        for (const std::pair<std::string, double>& e : fr)
            ord.push_back(std::make_pair(100.0 * e.second / raiz, e.first));
        std::sort(ord.begin(), ord.end(),
                  [](const std::pair<double, std::string>& a,
                     const std::pair<double, std::string>& b) {
                      return a.first != b.first ? a.first > b.first : a.second < b.second;
                  });

        size_t n = 25;
        if (tk.size() > 1 && tk[1] == "all") n = ord.size();
        else if (es_num) n = static_cast<size_t>(cuantas);
        if (n > ord.size()) n = ord.size();

        std::printf("\n  How often each line is reached, over every runout\n\n");
        std::printf("    %8s   %s\n", "freq", "line");
        for (size_t i = 0; i < n; ++i)
            std::printf("    %7.3f%%   %s\n", ord[i].first, ord[i].second.c_str());
        if (n < ord.size())
            std::printf("    (%d more; `freqs all`, or `freqs FILE` for a csv)\n",
                        (int)(ord.size() - n));
        std::printf("\n  %d lines\n", (int)ord.size());
    }

    void cmd_lines(const std::vector<std::string>& tk) {
        const std::vector<std::string> ls = all_lines(S.tree());
        if (tk.size() > 1) {
            std::FILE* f = std::fopen(tk[1].c_str(), "wb");
            if (!f) { err("cannot write " + tk[1]); return; }
            for (const std::string& l : ls) std::fprintf(f, "%s\n", l.c_str());
            std::fclose(f);
            std::printf("  %d lines -> %s\n", (int)ls.size(), tk[1].c_str());
            return;
        }
        for (const std::string& l : ls) std::printf("  %s\n", l.c_str());
        std::printf("  %d lines\n", (int)ls.size());
    }

    void cmd_lock(const std::vector<std::string>& tk) {
        if (!need_decision()) return;
        if (tk.size() < 3) {
            err("usage: lock <hands> current          freeze what the solve did\n"
                "         lock <hands> B=70%           set that action, rest in proportion\n"
                "         lock <hands> B*0.5           scale it, rest in proportion\n"
                "         lock <hands> B+20 / C-100    move it by points\n"
                "         lock <hands> B=0.3,X=0.7     write the whole mix out");
            return;
        }
        std::string mixspec;
        for (size_t i = 2; i < tk.size(); ++i) mixspec += tk[i];

        // `lock <hands> current` freezes what the solve already produced, which
        // is where the work actually starts: you look at the strategy, then you
        // change it. Typing the numbers in from scratch is not looking at it.
        if (lower(mixspec) == "current" || lower(mixspec) == "now" ||
            lower(mixspec) == "asis" || lower(mixspec) == "as-is") {
            int got = 0;
            std::string e2;
            if (!S.add_lock_from_current(S.cur_ctx(), S.cur_node(), tk[1], got, e2)) {
                err(e2); return;
            }
            std::printf("  froze %d combo(s) at %s at what they were doing\n"
                        "  edit any of them with `lock <combo> <action=prob,..>`, "
                        "then `solve`\n", got, S.where().c_str());
            if (S.iso_off_by_lock())
                std::printf("  ! suit collapsing is now off: these locks name cards, so\n"
                            "    the suits are no longer interchangeable. Correct, but slower.\n");
            return;
        }

        // One action moves and the rest of the mix rebalances around it, in
        // proportion to what each already had. Three ways to say where it goes,
        // which are the ones the usual dialogs offer per action plus the relative
        // one you want when you are reading numbers off the screen:
        //
        //   B=70%   fixed        -- betting becomes exactly 70%
        //   B*0.5    proportional -- betting becomes half of whatever it was
        //   B+20     relative     -- twenty points more than it was
        //
        // A comma means the whole mix is being written out by hand instead, and
        // that is the older form further down.
        if (mixspec.find(',') == std::string::npos) {
            const size_t sg = mixspec.find_first_of("+-*=");
            ActionKind k;
            if (sg != std::string::npos && sg > 0 &&
                parse_action_kind(mixspec.substr(0, sg), k)) {
                const char op = mixspec[sg];
                std::string num = mixspec.substr(sg + 1);
                const bool pct = (!num.empty() && num.back() == '%');
                if (pct) num.pop_back();
                double amt = 0.0;
                if (!parse_double(num, amt)) {
                    err("expected `B=70%` (set it), `B*0.5` (scale it) or `B+20` "
                        "(move it by points)");
                    return;
                }
                LockMode mode = LM_ADD;
                double w = 0.0;
                if (op == '=')       { mode = LM_FIXED; w = pct ? amt / 100.0 : amt; }
                else if (op == '*')  { mode = LM_SCALE; w = amt; }
                else                 { mode = LM_ADD;   w = (op == '-' ? -amt : amt) / 100.0; }
                if (mode == LM_FIXED && (w < 0.0 || w > 1.0)) {
                    err("a fixed frequency runs 0 to 1, so seven tenths is "
                        "'B=0.7' or 'B=70%'");
                    return;
                }
                if (mode == LM_SCALE && w < 0.0) { err("a scale cannot be negative"); return; }
                int got = 0;
                std::string e2;
                if (!S.add_lock_moved(S.cur_ctx(), S.cur_node(), tk[1], k, mode, w, got, e2)) {
                    err(e2); return;
                }
                std::printf("  %s on %d combo(s) at %s -- run `solve`\n",
                            (mode == LM_FIXED ? (kind_name(k) + std::string(" set")).c_str()
                           : mode == LM_SCALE ? (kind_name(k) + std::string(" scaled")).c_str()
                                              : (kind_name(k) + std::string(" moved")).c_str()),
                            got, S.where().c_str());
                if (S.iso_off_by_lock())
                    std::printf("  ! suit collapsing is now off: these locks name cards, so\n"
                                "    the suits are no longer interchangeable. Correct, but slower.\n");
                return;
            }
        }

        // El token puede ser un TIPO -- `B=0.5`, que es lo que escribe la
        // gente -- o el CODIGO exacto de la accion -- `B33=0.5` --, que es lo
        // unico que sirve cuando la calle tiene dos tamanos. Se guarda tal cual
        // y lo resuelve el arbol.
        std::vector<std::pair<std::string, double>> mix;
        for (const std::string& item : split(mixspec, ',')) {
            if (item.empty()) continue;
            const size_t eq = item.find('=');
            if (eq == std::string::npos) { err("expected action=prob, got '" + item + "'"); return; }
            double p;
            const std::string tok = trim(item.substr(0, eq));
            if (tok.empty()) { err("empty action in the mix"); return; }
            if (!parse_double(item.substr(eq + 1), p)) { err("bad probability"); return; }
            mix.push_back(std::make_pair(tok, p));
        }
        int matched = 0;
        std::string e;
        if (!S.add_lock(S.cur_ctx(), S.cur_node(), tk[1], mix, matched, e)) { err(e); return; }
        const std::string ro = S.cur_runout();
        const std::string donde = ro.empty() ? std::string(" (todo el nodo)")
                                             : (" (solo con " + ro + " fuera)");
        std::printf("  locked %d combo(s) at %s%s -- run `solve`\n",
                    matched, S.where().c_str(), donde.c_str());
        if (S.iso_off_by_lock())
            std::printf("  ! suit collapsing is now off: this lock names cards, so the\n"
                        "    suits are no longer interchangeable. Correct, but slower.\n");
    }
    void cmd_unlock(const std::vector<std::string>& tk) {
        if (tk.size() > 1 && lower(tk[1]) == "all") {
            const int n = S.clear_all_locks();
            if (!n) { err("there were no locks"); return; }
            ok(std::to_string(n) + " lock(s) removed -- run `solve`");
            return;
        }
        if (!S.remove_lock(S.cur_ctx(), S.cur_node())) { err("no lock here"); return; }
        ok("lock removed -- run `solve`");
    }
    void cmd_locks() const {
        if (S.locks().empty()) { std::printf("  (no locks)\n"); return; }
        print_locks();
    }

    // ---------------------------------------------------------------------
    void cmd_report(const std::vector<std::string>& tk) {
        if (!need_solution()) return;
        FILE* f = nullptr;
        if (tk.size() > 1) {
            f = std::fopen(tk[1].c_str(), "w");
            if (!f) { err("cannot write to '" + tk[1] + "'"); return; }
        }
        write_report(f, *S.solver(), S.tree(), S.deal());
        if (f) { std::fclose(f); ok("report written to " + tk[1]); }
    }

    void cmd_csv(const std::vector<std::string>& tk) {
        if (!need_solution() || !need_decision()) return;
        if (tk.size() < 2) { err("usage: csv <file>"); return; }
        std::string e;
        if (!write_csv(tk[1], *S.solver(), S.deal(), here(),
                       board_str(S.deal().board), S.where(), e)) { err(e); return; }
        ok("wrote " + tk[1]);
    }
};
