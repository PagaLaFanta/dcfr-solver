#pragma once
// =============================================================================
//  Regression checks -- `solver --check`.
//
//  Fixed spots with numbers that were correct when they were written down. The
//  point is not that these particular values are sacred; it is that a change
//  which moves them has to be noticed and explained, instead of being spotted
//  three commits later because a printed figure looked odd.
//
//  Everything here is deterministic: the orbit results are accumulated in a
//  fixed order regardless of how many threads carried them, so the numbers do
//  not depend on the machine. That is checked too.
//
//  Runs in a few seconds. Run it after every change.
// =============================================================================

#include "session.hpp"
#include "rooms.hpp"
#include "webui_page.hpp"
#include "webui.hpp"
#include "console.hpp"

#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#ifdef _WIN32
  #include <io.h>
#else
  #include <unistd.h>
#endif
#include <thread>
#include <string>
#include <vector>

class Checks {
public:
    // `record` prints every measured value at full precision instead of
    // comparing, which is how the expected numbers below were obtained.
    explicit Checks(bool record) : record_(record) {}

    int run() {
        // Sin buffer, y no por linea.
        //
        // `setvbuf(stdout, nullptr, _IOLBF, 0)` es un PARAMETRO INVALIDO para
        // el CRT de Visual Studio: con _IOLBF el tamano tiene que estar entre
        // 2 e INT_MAX. Y el CRT no devuelve un error, mata el proceso con
        // __fastfail: 0xC0000409, sin mensaje y sin haber impreso una linea.
        //
        // MEDIDO en la primera build de la CI: la bateria se moria en tres
        // segundos con el log vacio, y la primera linea que imprime esta
        // dos lineas mas abajo. Con MinGW la misma llamada se traga sin
        // rechistar, asi que aqui nunca se vio.
        //
        // _IONBF hace lo que hacia falta -- que cada linea salga cuando se
        // escribe, para poder ver por donde va una bateria que tarda diez
        // minutos -- y ahi el tamano se ignora de verdad.
        std::setvbuf(stdout, nullptr, _IONBF, 0);
        // Con que idioma ARRANCA el programa, antes de que nadie pida nada. Se
        // mira aqui porque es lo unico que se puede mirar: en cuanto llega la
        // primera peticion el servidor lo pone, y la consola tambien.
        arranca_en_ingles_ = msg::EN;
        // Los mensajes del motor, en ingles: esto es una superficie de
        // desarrollo y todo lo que imprime esta en ingles. Que los dos idiomas
        // funcionan lo comprueba the_engine_answers_in_the_language_it_was_asked,
        // que los pide los dos a proposito.
        msg::EN = true;
        std::printf("\n  solver --check\n");
        std::printf("  ---------------------------------------------------------\n");
        // Lo que se LEE en pantalla, primero: no necesita sesion ni solve, y
        // asi una mutacion en el texto se caza en dos segundos y no al final
        // de un fichero que tarda media hora.
        every_number_is_written_the_same_way();
        nothing_on_screen_is_left_in_english();
        the_build_says_which_one_it_is();
        the_readme_names_buttons_that_exist();
        the_help_says_what_it_is_written_to_say();
        a_narrow_window_does_not_squeeze_the_solution();
        the_bars_say_out_loud_that_they_are_bars();
        every_spanish_line_on_screen_has_an_english_one();
        the_gear_changes_the_language();
        the_english_table_does_not_rot();
        nothing_asks_to_be_translated_and_is_not();
        no_key_has_a_space_stuck_to_it();
        the_workflows_are_not_broken_yaml();
        it_refuses_to_open_next_to_a_poker_room();
        the_gear_holds_every_advanced_option();
        taking_the_line_panel_out_left_nothing_behind();
        the_long_notes_are_one_line_until_you_ask();
        a_family_bar_looks_like_a_control();
        the_family_table_is_read_at_a_glance();
        the_grid_writes_the_number_it_is_painting();
        the_grid_says_what_a_hand_weighs();
        the_page_says_which_language_it_wants();

        // Three sessions, reused. Building a deal means strength tables for
        // every runout, which on a flop is seconds; a session per check turned
        // a five-second suite into a thirty-second one.
        Session flop, turn, river;
        // Esta escribe un fichero y lo pesa, asi que es barata y va pronto:
        // una mutacion suya no tiene que esperar a media hora de solves.
        a_saved_tree_is_exactly_as_big_as_it_says(river);
        // Baratas, y de lo que se rompe solo: van pronto.
        a_fresh_start_does_not_die_on_any_command(river);
        an_unrecognised_yes_no_is_an_error(river);
        a_refused_setting_does_not_throw_the_solve_away(river);
        the_street_the_solve_starts_on_says_why_it_has_no_donk(flop);
        putting_back_the_same_numbers_costs_nothing(river);
        a_saved_range_is_the_pair(river);
        the_ranges_that_come_in_the_box();
        playing_the_tree_is_playing_the_solution(river);
        the_hand_can_start_anywhere_in_the_tree(river);
        the_advice_does_not_travel_when_it_is_off(river);
        the_engine_answers_in_the_language_it_was_asked(river);
        two_flops_that_are_the_same_board_are_one();
        the_many_board_script_is_a_script_that_runs(river);
        a_time_limit_actually_stops_the_solve(river);
        tree_shape(flop);
        memory_accounting(flop);
        raise_controls(flop);
        the_raise_ladder_is_the_standard_one(flop);
        // Pronto: monta su propio spot y no necesita a nadie, y asi una
        // mutacion suya se caza en minutos en vez de al final del fichero.
        a_whole_family_is_one_lock(flop);
        a_lock_does_not_throw_the_solve_away(flop);
        each_family_has_its_own_undo(flop);
        donk_controls(flop);
        each_player_has_their_own_sizes(flop);
        solve_values(river, turn, flop);
        isomorphism_agrees(turn, "Ah9h4hKd", 300);
        isomorphism_agrees(flop, "Ah9h4h", 40);
        threads_agree(turn);
        save_load_round_trip(turn);
        solve_continues(turn);
        locks_survive_a_save(turn);
        runout_table(turn);
        csv_export(turn);
        report_grids_add_up(turn);
        class_aggregate_is_an_average(flop);
        the_board_group_is_the_group(flop);
        iso_bows_to_an_asymmetric_range(flop);
        a_symmetry_the_flop_does_not_have(flop);
        iso_bows_to_an_asymmetric_lock(flop);
        runout_addressing(flop);
        permuted_reads(flop);
        block_average_weighting(flop);
        range_parsing(river);
        range_refusals(river);
        sizing_and_board_parsing(river);
        what_it_prints_it_takes(river);
        old_configs_still_open(river);
        the_allin_threshold_is_the_stack(flop);
        bets_are_whole_chips(flop);
        a_raise_chain_that_never_ends(river);
        discount_paths_agree(turn);
        rake_model(river);
        exploitability_shuts(river);
        polarised_toy(river, 0.5, 0.00, 0.0,  20000);
        polarised_toy(river, 1.0, 0.00, 0.0,  20000);
        polarised_toy(river, 0.5, 0.05, 0.0,  40000);   // uncapped rake
        polarised_toy(river, 0.5, 0.05, 0.5,  40000);   // a cap that binds
        blocker_toy(river, 0.5, 30000);
        a_lock_belongs_to_one_runout(turn);
        equity_follows_the_node(flop);
        the_range_of_a_node_is_the_parent_times_the_frequency(flop);
        the_runout_average_is_the_average_of_the_runouts(flop);
        the_equity_is_of_this_runout(flop);
        an_allin_raise_needs_a_raise_list(flop);
        the_best_response_is_a_best_response(river);
        the_category_names_are_the_ones_everyone_shows(flop);
        the_rows_come_from_the_board_not_from_the_range(flop);
        a_made_hand_is_a_fact(flop);
        the_categories_are_the_ones_everyone_uses(flop);
        a_draw_belongs_to_the_hand(flop);
        a_line_frequency_is_a_probability(flop);
        a_raise_is_a_multiple_or_the_minimum(flop);
        nodelock_bites(turn);
        nothing_to_decide_with_nothing_behind(flop);
        the_accuracy_target_stops_the_solve(flop);
        what_it_shows_is_never_worse_than_what_you_asked(flop);
        two_sizes_written_as_people_write_them(flop);
        every_grid_label_can_be_read(flop);
        the_convergence_bands_are_where_we_put_them();
        the_built_in_spot_loads();
        two_solvers_cannot_share_a_port(flop);
        the_server_serves_while_it_solves(flop);
        the_page_is_not_broken();
        a_class_name_means_one_thing();
        folding_the_setup_gives_the_room_away();
        the_lock_strip_cannot_become_a_wall();
        freezing_keeps_what_you_painted();
        nothing_throws_a_solve_away_in_silence();
        the_page_reads_what_the_server_sends(flop);
        a_chance_node_averages_over_the_right_cards(turn);
        nodelock_promises(turn);
        nodelock_from_the_strategy(flop);
        std::printf("  ---------------------------------------------------------\n");
        if (record_) {
            std::printf("  RECORD mode: values printed, not compared.\n"
                        "  The theory checks were still compared: %d failed.\n\n",
                        failed_);
            return failed_ ? 1 : 0;
        }
        std::printf("  %d passed, %d failed\n\n", passed_, failed_);
        return failed_ ? 1 : 0;
    }

private:
    bool record_;
    bool arranca_en_ingles_ = true;
    int  passed_ = 0;
    int  failed_ = 0;

    void pass(const std::string& name) {
        ++passed_;
        std::printf("  ok    %s\n", name.c_str());
    }
    void fail(const std::string& name, const std::string& detail) {
        ++failed_;
        std::printf("  FAIL  %s\n          %s\n", name.c_str(), detail.c_str());
    }

    bool close_to(const std::string& name, double got, double want, double tol) {
        if (record_) {
            std::printf("  rec   %-46s %.10f\n", name.c_str(), got);
            return true;
        }
        if (std::fabs(got - want) <= tol) { pass(name); return true; }
        char b[192];
        std::snprintf(b, sizeof b, "got %.10f, expected %.10f (tolerance %g)", got, want, tol);
        fail(name, b);
        return false;
    }
    // Like close_to, but it compares even in record mode. A number that came
    // out of a piece of algebra is not a number to record: if `--record` could
    // rewrite it, a bug in the engine would launder itself into the expected
    // values the moment anyone re-recorded.
    static std::string fmt_sci(double v) {
        char b[48]; std::snprintf(b, sizeof b, "%.3e", v); return b;
    }
    bool must_be(const std::string& name, double got, double want, double tol) {
        if (std::fabs(got - want) <= tol) { if (!record_) pass(name); return true; }
        char b[192];
        std::snprintf(b, sizeof b, "got %.10f, theory says %.10f (tolerance %g)",
                      got, want, tol);
        fail(name, b);
        return false;
    }
    bool same(const std::string& name, long long got, long long want) {
        if (record_) {
            std::printf("  rec   %-46s %lld\n", name.c_str(), got);
            return true;
        }
        if (got == want) { pass(name); return true; }
        char b[160];
        std::snprintf(b, sizeof b, "got %lld, expected %lld", got, want);
        fail(name, b);
        return false;
    }
    bool truth(const std::string& name, bool got, const std::string& detail = "") {
        if (got) { pass(name); return true; }
        fail(name, detail.empty() ? "expected true" : detail);
        return false;
    }

    // The spot every numeric check below is built on.
    static const char* OOP_RANGE() { return "22+,A2s+,K9s+,Q9s+,J9s+,T9s,98s,ATo+,KJo+"; }
    static const char* IP_RANGE()  { return "22+,A2s+,K9s+,QTs+,JTs,T9s,AJo+,KQo"; }

    // A session with the standard ranges and the default tree, on `board`.
    // Every knob is set explicitly so a change of default shows up as a failure
    // here rather than silently moving every number in the file.
    bool spot(Session& S, const char* board, std::string& e) {
        cfg::POT0  = 20.0;
        cfg::STACK = 100.0;
        cfg::ISO   = true;
        cfg::DCFR_ALPHA = 1.5;
        cfg::DCFR_BETA  = 0.0;
        cfg::DCFR_GAMMA = 2.0;
        cfg::RAKE_PCT   = 0.0;
        cfg::RAKE_CAP   = 0.0;
        cfg::ALLIN_THRESH = 0.67;
        cfg::MERGE_PCT    = 0.10;
        // Un spot de prueba corre las vueltas que se le piden y ni una menos.
        // Con el objetivo de precision activo -- que es como sale de fabrica,
        // igual que en la referencia -- un solve para en cuanto lo alcanza, y una
        // comprobacion que dice "100 mas 50 son 150" se encuentra con 101. La
        // comprobacion tiene razon: lo que mide es la contabilidad de
        // iteraciones, no la convergencia.
        S.set_acc_stop(false);
        // Locks are session state like any other knob: a lock left behind by
        // an earlier check would ride along into this one.
        S.clear_all_locks();
        if (!S.set_board(board, e)) return false;
        if (!S.set_range(0, OOP_RANGE(), e)) return false;
        if (!S.set_range(1, IP_RANGE(), e)) return false;
        for (int s = 0; s < 3; ++s) {
            if (!S.set_allin(s, false, e)) return false;
            if (!S.set_donks(s, std::vector<Sizing>(), e)) return false;
        }
        std::vector<Sizing> b;
        const std::vector<Sizing> r0;
        const char* bs[3] = { "60", "66", "75" };
        for (int s = 0; s < 3; ++s) {
            if (!parse_sizings(bs[s], b, e) || !S.set_sizings(true, s, b, e)) return false;
            // No raises in the baseline. That is what the old bet+raise cap of
            // 1 amounted to, with a 3x sizing sitting in the config unused.
            if (!S.set_sizings(false, s, r0, e)) return false;
        }
        return true;
    }

    // Every action label at a node, e.g. "F|C|R24|R36".
    static std::string actions_at(const GameTree& T, int ci, int nid) {
        const BetTree& bt = T.ctx[static_cast<size_t>(ci)].tree;
        const Node& n = bt.nodes[static_cast<size_t>(nid)];
        std::string s;
        for (int a = 0; a < n.num_actions; ++a) {
            if (a) s += "|";
            s += bt.act(n, a).code;
        }
        return s;
    }
    static std::string actions_on_path(const GameTree& T, const std::string& ctx_label,
                                       const std::string& path) {
        const int c = T.find_ctx(ctx_label);
        if (c < 0) return "<no ctx>";
        const int n = T.find_node(c, path);
        if (n < 0) return "<no node>";
        return actions_at(T, c, n);
    }
    void expect_actions(const std::string& name, const GameTree& T,
                        const std::string& ctx_label, const std::string& path,
                        const std::string& want) {
        const std::string got = actions_on_path(T, ctx_label, path);
        if (record_) {
            std::printf("  rec   %-46s %s\n", name.c_str(), got.c_str());
            return;
        }
        if (got == want) pass(name);
        else fail(name, "got '" + got + "', expected '" + want + "'");
    }

    // ---- the checks ------------------------------------------------------

    void tree_shape(Session& S) {
        std::string e;
        if (!truth("flop spot builds", spot(S, "Ah9h4h", e), e)) return;
        const GameTree& T = S.tree();
        same("flop: contexts",        static_cast<long long>(T.ctx.size()), 12);
        same("flop: template nodes",  T.num_decision_nodes(), 44);
        same("flop: instanced nodes", T.num_instanced_nodes(), 19832);
        // Memory is laid out over live combos, so this also pins the ranges.
        same("flop: buffer entries",  T.mem_size, 7140144);
        same("flop: live combos OOP", T.nlive[0], 204);
        same("flop: live combos IP",  T.nlive[1], 174);
    }

    // The three controls that stop the betting tree exploding. Structural, so
    // they run in milliseconds and pin down exactly what each knob means.
    // La escalera de subidas entera, que es la de la referencia.
    //
    // MEDIDA en un arbol que construyo LA REFERENCIA, no deducida: `show_all_lines` sobre
    // Sale de un arbol de fuera con el que este solver cuadro linea por
    // linea, 611 contra 611 -- da 33 -> 66 -> 99 -> 132 y de ahi al all-in. Sube
    // de 33 en 33 con `2x`, o sea "lo que llevas puesto MAS dos veces lo que hay
    // que pagar".
    //
    // Lo que NO es: "dos veces la apuesta que afrontas", que daria 33 -> 66 ->
    // 132 -> 264. A la PRIMERA subida las dos cuentas dan lo mismo -- 66 -- y
    // por eso esto se colaba: la comprobacion de al lado solo miraba el primer
    // escalon, y ahi no hay nada que distinguir.
    //
    // De hecho se colo: el generador de lineas del banco de pruebas, el que
    // la referencia el arbol para compararlo con el nuestro, subia a 200 donde nosotros
    // subimos a 150, y la comparacion de frecuencias de linea salia con 37
    // lineas nuestras y 12 suyas que no estaban en el otro. El solver tenia
    // razon y el generador no.
    void the_raise_ladder_is_the_standard_one(Session& S) {
        std::string e;
        if (!truth("ladder spot builds", spot(S, "Ah9h4h", e), e)) return;
        // El spot, tal cual: bote 55, stack 220, apuesta del 60% -- 33 --
        // y subidas 2x. Son los numeros del fichero suyo que se acaba de leer.
        cfg::POT0  = 55.0;
        cfg::STACK = 220.0;
        std::vector<Sizing> b, r;
        if (!truth("60% parses", parse_sizings("60%", b, e), e)) return;
        if (!truth("and applies", S.set_sizings(true, 0, b, e), e)) return;
        if (!truth("2x parses", parse_sizings("2x", r, e), e)) return;
        if (!truth("and applies too", S.set_sizings(false, 0, r, e), e)) return;
        if (!truth("the tree rebuilds on those numbers", S.rebuild(e), e)) return;

        expect_actions("the bet is 33, like his",  S.tree(), "R", "R", "X|B33");
        expect_actions("the raise, 66",            S.tree(), "R", "R/B33", "F|C|R66");
        expect_actions("the next one, 99",         S.tree(), "R", "R/B33/R66", "F|C|R99");
        expect_actions("and the next, 132",        S.tree(), "R", "R/B33/R66/R99",
                       "F|C|R132");
        // Y el escalon que separa las dos cuentas, dicho al reves: si alguien
        // vuelve a la otra, aqui apareceria un R132 en vez del R99.
        truth("and not the other count, which would jump to 132 here",
              actions_on_path(S.tree(), "R", "R/B33/R66").find("R132") ==
                  std::string::npos,
              actions_on_path(S.tree(), "R", "R/B33/R66"));

        cfg::POT0  = 20.0;
        cfg::STACK = 100.0;
    }

    void raise_controls(Session& S) {
        std::string e;
        if (!truth("raise spot builds", spot(S, "Ah9h4h", e), e)) return;

        // No raise sizings, no raises. That is the whole rule, and now it is
        // the only one: there used to be a separate cap that could overrule the
        // sizings without saying anything.
        expect_actions("no raise sizes, no raise", S.tree(), "R", "R/B12", "F|C");

        std::vector<Sizing> r;
        if (!truth("3x parses", parse_sizings("3x", r, e), e)) return;
        if (!truth("3x applies", S.set_sizings(false, 0, r, e), e)) return;
        expect_actions("a raise size is all it takes", S.tree(), "R", "R/B12", "F|C|R36");

        // ...and taking the sizings away takes the raise away again.
        if (!truth("none parses", parse_sizings("none", r, e) && r.empty(), e)) return;
        if (!truth("none applies", S.set_sizings(false, 0, r, e), e)) return;
        expect_actions("none removes the raise", S.tree(), "R", "R/B12", "F|C");

        // 3x is three times the bet faced; 2x is exactly the min-raise.
        if (!truth("2x,3x parses", parse_sizings("2x,3x", r, e), e)) return;
        if (!truth("2x,3x applies", S.set_sizings(false, 0, r, e), e)) return;
        expect_actions("x notation sizes raises", S.tree(), "R", "R/B12", "F|C|R24|R36");

        // Medio bote encima cae en 34, a tres fichas del 3x que cae en 36.
        //
        // El tamano se arma a mano: el CAMPO de subidas ya no acepta
        // porcentajes -- solo multiplos y `min` -- pero el motor si, porque los
        // .cfg guardados los llevan. Lo que se prueba aqui es la fusion de
        // tamanos parecidos, no el lector.
        r.clear();
        { Sizing z; z.v = 0.5; r.push_back(z); }
        { Sizing z; z.v = 3.0; z.xbet = true; r.push_back(z); }
        if (!truth("half-pot and 3x apply", S.set_sizings(false, 0, r, e), e)) return;
        cfg::MERGE_PCT = 0.0;
        if (!truth("merge off", S.rebuild(e), e)) return;
        expect_actions("merge 0 keeps near-duplicates", S.tree(), "R", "R/B12", "F|C|R34|R36");
        cfg::MERGE_PCT = 0.10;
        if (!truth("merge 0.10", S.rebuild(e), e)) return;
        expect_actions("merge 0.10 collapses them", S.tree(), "R", "R/B12", "F|C|R34");

        // ...but leaves genuinely different sizings alone.
        if (!truth("2x,3x again", parse_sizings("2x,3x", r, e), e)) return;
        if (!truth("2x,3x applies again", S.set_sizings(false, 0, r, e), e)) return;
        expect_actions("merge spares distinct sizings", S.tree(), "R", "R/B12", "F|C|R24|R36");

        // A sizing below the legal minimum is raised to it. Every sizing used
        // elsewhere in these checks already clears the min-raise, so nothing
        // was exercising the clamp: 5% of the pot on top of a bet of 12 wants
        // to raise to 14.2, and the smallest legal raise is to 24.
        r.clear();
        { Sizing z; z.v = 0.05; r.push_back(z); }
        if (!truth("tiny raise applies", S.set_sizings(false, 0, r, e), e)) return;
        expect_actions("a sizing under the minimum is clamped up",
                       S.tree(), "R", "R/B12", "F|C|R24");
        if (!truth("back to 3x", parse_sizings("3x", r, e), e)) return;
        if (!truth("3x applies", S.set_sizings(false, 0, r, e), e)) return;

        // A raise that would leave a stub behind becomes a clean all-in, which
        // needs a re-raise deep enough for it to matter. Nothing to arrange for
        // that any more: the chain runs until the threshold stops it.
        r.clear();
        { Sizing z; z.v = 0.5; r.push_back(z); }
        if (!truth("half-pot raises apply", S.set_sizings(false, 0, r, e), e)) return;
        if (!truth("threshold off", S.set_allin_thresh(1.0, e), e)) return;
        const std::string loose = actions_on_path(S.tree(), "R", "R/B12/R34");
        const long long loose_n = S.tree().num_decision_nodes();
        if (!truth("threshold 0.4", S.set_allin_thresh(0.4, e), e)) return;
        const std::string tight = actions_on_path(S.tree(), "R", "R/B12/R34");
        const long long tight_n = S.tree().num_decision_nodes();
        if (record_)
            std::printf("  rec   %-46s '%s' (%lld nodes) -> '%s' (%lld nodes)\n",
                        "allin threshold", loose.c_str(), loose_n, tight.c_str(), tight_n);
        else {
            truth("allin threshold changes the re-raise", loose != tight,
                  "threshold made no difference: both '" + loose + "'");
            truth("allin threshold shrinks the tree", tight_n < loose_n,
                  std::to_string(tight_n) + " nodes vs " + std::to_string(loose_n));
        }
        truth("threshold back", S.set_allin_thresh(0.67, e), e);
    }

    // The two players do not bet the same sizes, and until now they had no
    // choice. One shared list per street was a simplification nobody asked for
    // -- the reference solver gives each of them their own box, and so does this.
    void each_player_has_their_own_sizes(Session& S) {
        std::string e;
        if (!truth("per-player spot builds", spot(S, "Ah9h4h", e), e)) return;
        cfg::MERGE_PCT = 0.0;
        if (!truth("no merging", S.rebuild(e), e)) return;

        std::vector<Sizing> half, whole, three;
        if (!truth("sizes parse", parse_sizings("50", half, e) &&
                                  parse_sizings("pot", whole, e) &&
                                  parse_sizings("3x", three, e), e)) return;
        if (!truth("OOP bets half the pot",
                   S.set_sizings_for(true, 0, ST_FLOP, half, e), e)) return;
        if (!truth("IP bets the pot",
                   S.set_sizings_for(true, 1, ST_FLOP, whole, e), e)) return;

        // The whole point, in one pair of lines: OOP opens for 10 on a 20 pot,
        // IP opens for 20. One list could not have said that.
        expect_actions("OOP opens for its own size", S.tree(), "R", "R", "X|B10");
        expect_actions("IP opens for a different one", S.tree(), "R", "R/X", "X|B20");

        // "Don't 3-bet" is IP's, and it bites exactly where its name says: IP
        // bets, OOP check-raises, and the third aggressive action of the street
        // is the one that goes.
        if (!truth("both raise 3x", S.set_sizings(false, ST_FLOP, three, e), e)) return;
        expect_actions("IP can 3-bet by default",
                       S.tree(), "R", "R/X/B20/R60", "F|C|R100");
        if (!truth("no3bet on", S.set_no3bet(ST_FLOP, true, e), e)) return;
        expect_actions("and does not with the box ticked",
                       S.tree(), "R", "R/X/B20/R60", "F|C");
        // It is IP's alone, so OOP's own raise over IP's bet is untouched.
        expect_actions("while OOP still raises", S.tree(), "R", "R/X/B20", "F|C|R60");
        truth("no3bet off", S.set_no3bet(ST_FLOP, false, e), e);

        // And the fact that keeps getting mistaken for a bug: with no raise
        // sizes anywhere there are no raises anywhere, which is correct and is
        // worth being able to state rather than leaving somebody to deduce it
        // from an empty field.
        std::vector<Sizing> none;
        for (int st = 0; st < 3; ++st)
            if (!truth("clear every raise list", S.set_sizings(false, st, none, e), e)) return;
        truth("a tree with no raise sizes says it has no raises", S.no_raises_anywhere());
        if (!truth("one street gets them back",
                   S.set_sizings_for(false, 1, ST_TURN, three, e), e)) return;
        truth("and then it does not say that any more", !S.no_raises_anywhere());
    }

    // A donk is OOP betting into the player who was last aggressive. After a
    // checked-through street nobody was, so leading there is not a donk.
    void donk_controls(Session& S) {
        std::string e;
        if (!truth("donk spot builds", spot(S, "Ah9h4h", e), e)) return;

        expect_actions("no donk after IP's bet",    S.tree(), "R|R/X/B12/C", "R", "X");
        expect_actions("cbet after OOP's own bet",  S.tree(), "R|R/B12/C",   "R", "X|B29");
        expect_actions("lead after check-check",    S.tree(), "R|R/X/X",     "R", "X|B13");

        // "Donk on" is now "give the donk a size". 0.66 of the turn pot is
        // what OOP's own turn bet was, so this is the same tree the old on/off
        // switch built, expressed as the list it always really was.
        std::vector<Sizing> dz;
        if (!truth("a donk size parses", parse_sizings("66", dz, e), e)) return;
        if (!truth("donk sizes apply", S.set_donks(ST_TURN, dz, e), e)) return;
        expect_actions("a donk size restores the lead", S.tree(), "R|R/X/B12/C", "R", "X|B29");

        // ...and taking the sizes away takes the lead away again, which is the
        // same rule as bets and raises: no sizings, no action.
        if (!truth("donk sizes cleared", S.set_donks(ST_TURN, std::vector<Sizing>(), e), e)) return;
        expect_actions("and no donk size takes it away", S.tree(), "R|R/X/B12/C", "R", "X");
        if (!truth("donk sizes back", S.set_donks(ST_TURN, dz, e), e)) return;

        // And it is its own list, not a second name for OOP's bet sizes: a
        // quarter-pot lead where the continuation bet is two thirds. That is
        // the whole reason it is a list rather than the on/off switch it was.
        std::vector<Sizing> qz;
        if (!truth("a quarter pot parses", parse_sizings("25", qz, e), e)) return;
        if (!truth("as a donk size", S.set_donks(ST_TURN, qz, e), e)) return;
        expect_actions("the donk is priced from its own list",
                       S.tree(), "R|R/X/B12/C", "R", "X|B11");
        expect_actions("while the ordinary turn bet is not",
                       S.tree(), "R|R/X/X", "R", "X|B13");
        if (!truth("donk sizes restored", S.set_donks(ST_TURN, dz, e), e)) return;

        // And on the street the solve STARTS on there is no donk to switch: no
        // previous street, so nobody was aggressive, so OOP's opening bet is
        // always there. The builder never reads the flag. Accepting it and
        // doing nothing is the bug this whole family is about, so it refuses.
        std::string why;
        truth("donk sizes on the starting street are refused",
              !S.set_donks(ST_FLOP, dz, why), "they were accepted");
        truth("and says why", why.find("starts on") != std::string::npos,
              "said '" + why + "'");
        truth("clearing them there is fine, because that is the state anyway",
              S.set_donks(ST_FLOP, std::vector<Sizing>(), e), e);

        // The bug that made this worth writing: the rule is "the starting
        // street", not "the flop". On a turn solve it is the TURN that has no
        // donk, and the flop is not in the tree at all.
        if (!truth("a turn solve builds", S.set_board("Ah9h4hKd", e), e)) return;
        truth("on a turn solve it is the turn that has no donk",
              !S.set_donks(ST_TURN, dz, why), "the turn accepted a donk");
        truth("while the river still does",
              S.set_donks(ST_RIVER, dz, e), e);
        if (!truth("back to the flop", S.set_board("Ah9h4h", e), e)) return;
        truth("and there the flop is the one without", !S.set_donks(ST_FLOP, dz, why),
              "the flop accepted a donk");
        truth("with the turn free again", S.set_donks(ST_TURN, dz, e), e);

        // The third way in, and the one a refusal cannot catch: the board moves
        // UNDER a donk list that was fine where it was. Turn donks on a flop
        // board are legal; deal the turn and they belong to the starting street,
        // where nobody reads them. Dropped where the board is decided, and said.
        if (!truth("the turn has donk sizes", !S.tc().donks[ST_TURN].empty(),
                   "they did not stick")) return;
        if (!truth("now deal the turn", S.set_board("Ah9h4hKd", e), e)) return;
        truth("the stranded donk sizes are dropped",
              S.tc().donks[ST_TURN].empty(), "they survived onto the starting street");
        truth("and the drop is reported, not silent",
              S.load_note().find("donk sizes") != std::string::npos,
              S.load_note().empty() ? "said nothing" : S.load_note());
        truth("back to the flop", S.set_board("Ah9h4h", e), e);
    }

    struct Expect { const char* board; int iters; double ev_oop; double expl; };

    void solve_values(Session& river, Session& turn, Session& flop) {
        // Same ranges, same tree, three starting streets.
        const Expect cases[3] = {
            { "Ah9h4hKd2s", 300, 8.8406317336, 0.0038278213 },
            // Turn and flop moved when the chance-node divisor was corrected;
            // the river above did not, because a river has no chance node.
            { "Ah9h4hKd",   300, 8.6681012450, 0.0158492528 },
            { "Ah9h4h",     300, 8.5251115892, 0.0312695738 },
        };
        Session* sess[3] = { &river, &turn, &flop };
        for (int ci2 = 0; ci2 < 3; ++ci2) {
            const Expect& c = cases[ci2];
            Session& S = *sess[ci2];
            std::string e;
            const std::string tag = std::string(c.board);
            if (!truth(tag + ": builds", spot(S, c.board, e), e)) continue;
            S.solve(c.iters, 0);
            if (!truth(tag + ": solved", S.solved())) continue;

            const double ev0 = S.solver()->root_ev(0);
            close_to(tag + ": game value OOP", ev0, c.ev_oop, 1e-6);
            const double x = S.solver()->exploitability();
            close_to(tag + ": exploitability", x, c.expl, 1e-6);
            // It is the sum of what each player would gain by best-responding
            // to the other, in a zero-sum game: at equilibrium the two cancel,
            // and away from it they cannot cancel to less than nothing. A
            // negative reading would mean one of the two "best" responses is
            // not one -- and it is the number people read to decide whether a
            // solve is finished, so it is worth saying out loud.
            must_be(tag + ": and it is not negative", x < 0.0 ? x : 0.0, 0.0, 0.0);

            // The two players' EVs must add up to the pot at EVERY decision
            // node, not just the root. This is the check that catches a
            // traversal that quietly stops covering part of the tree.
            double worst = 0.0;
            std::string worst_at;
            const GameTree& T = S.tree();
            for (size_t ci = 0; ci < T.ctx.size(); ++ci) {
                const RoundCtx& rc = T.ctx[ci];
                for (size_t ni = 0; ni < rc.tree.nodes.size(); ++ni) {
                    if (rc.tree.nodes[ni].type != NT_DECISION) continue;
                    const int c0 = static_cast<int>(ci), n0 = static_cast<int>(ni);
                    const double a = S.solver()->node_ev_player(c0, n0, 0, 0);
                    const double b = S.solver()->node_ev_player(c0, n0, 0, 1);
                    const double d = std::fabs(a + b - cfg::POT0);
                    if (d > worst) { worst = d; worst_at = rc.label + " " + rc.tree.nodes[ni].path; }
                }
            }
            if (record_)
                std::printf("  rec   %-46s %.3e at %s\n",
                            (tag + ": zero-sum drift").c_str(), worst, worst_at.c_str());
            else
                truth(tag + ": OOP+IP = pot at every node", worst < 1e-9,
                      "worst drift " + std::to_string(worst) + " at " + worst_at);
        }
    }

    // Collapsing suits must be exact, not an approximation. Checked on both a
    // turn and a flop: a turn start has one chance level and a stabiliser of
    // order two, so on its own it says nothing about the two-level case, which
    // is where the collapsing actually earns its keep.
    void isomorphism_agrees(Session& S, const char* board, int iters) {
        const std::string tag(board);
        double with = 0.0, without = 0.0, xw = 0.0, xo = 0.0;
        for (int pass = 0; pass < 2; ++pass) {
            std::string e;
            if (!truth(tag + ": iso spot builds", spot(S, board, e), e)) return;
            cfg::ISO = (pass == 0);
            if (!truth(tag + ": iso rebuild", S.rebuild(e), e)) { cfg::ISO = true; return; }
            S.solve(iters, 0);
            (pass == 0 ? with : without) = S.solver()->root_ev(0);
            (pass == 0 ? xw : xo)        = S.solver()->exploitability();
        }
        cfg::ISO = true;
        if (record_) {
            std::printf("  rec   %-46s on %.10f  off %.10f\n",
                        (tag + ": iso game value").c_str(), with, without);
            std::printf("  rec   %-46s on %.10f  off %.10f\n",
                        (tag + ": iso exploitability").c_str(), xw, xo);
            return;
        }
        // "Changes nothing" means up to floating point: with collapsing on the
        // sums are associated differently, and 1e-9 on numbers of this size is
        // finer than doubles promise after thousands of terms.
        close_to(tag + ": collapsing suits changes no game value", with, without, 1e-6);
        close_to(tag + ": collapsing suits changes no exploitability", xw, xo, 1e-6);
    }

    // The result must not depend on how many cores happened to be free.
    void threads_agree(Session& S) {
        double one = 0.0, many = 0.0;
        const int saved = cfg::THREADS;
        for (int pass = 0; pass < 2; ++pass) {
            std::string e;
            cfg::THREADS = (pass == 0) ? 1 : 0;
            if (!truth("thread spot builds", spot(S, "Ah9h4hKd", e), e)) { cfg::THREADS = saved; return; }
            S.solve(300, 0);
            (pass == 0 ? one : many) = S.solver()->root_ev(0);
        }
        cfg::THREADS = saved;
        if (record_)
            std::printf("  rec   %-46s 1 thread %.10f  all %.10f\n", "threads", one, many);
        else
            close_to("one thread and many agree", one, many, 1e-9);
    }

    // Saving is only useful if the solve can carry on from where it stopped.
    void save_load_round_trip(Session& S) {
        std::string e;
        const std::string name = "__check_roundtrip";

        if (!truth("round-trip spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(200, 0);
        const double straight = S.solver()->root_ev(0);

        if (!truth("round-trip spot builds again", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(100, 0);
        if (!truth("saves a tree", S.save_tree(name, e), e)) return;

        double resumed = 0.0;
        {
            // A fresh session with no ranges at all: loading has to restore the
            // whole setup from inside the file, not just the numbers.
            Session fresh;
            if (!truth("loads into an empty session", fresh.load_tree(name, e), e)) return;
            truth("load restores the ranges",
                  fresh.tree().nlive[0] == 188 && fresh.tree().nlive[1] == 161,
                  "got OOP " + std::to_string(fresh.tree().nlive[0]) +
                  " IP " + std::to_string(fresh.tree().nlive[1]) + ", expected 188 / 161");
            truth("load restores the iteration count",
                  fresh.solver()->iterations_done() == 100,
                  "got " + std::to_string(fresh.solver()->iterations_done()));
            if (!truth("resumes", fresh.iterate(100, 0), "iterate() refused")) return;
            resumed = fresh.solver()->root_ev(0);
        }
        // A corrupt file must be refused, not half-loaded. Flip one byte deep
        // inside the regrets, where nothing structural would notice.
        {
            const std::string path = Session::save_path(true, name);
            std::FILE* f = std::fopen(path.c_str(), "r+b");
            if (!truth("reopens the saved tree", f != nullptr, path)) return;
            std::fseek(f, 0, SEEK_END);
            const long size = std::ftell(f);
            std::fseek(f, size / 2, SEEK_SET);
            int byte = std::fgetc(f);
            std::fseek(f, size / 2, SEEK_SET);
            std::fputc(byte ^ 0x40, f);
            std::fclose(f);

            Session corrupt;
            std::string ce;
            truth("a corrupt tree is refused", !corrupt.load_tree(name, ce),
                  "it loaded anyway");
            truth("and says why", ce.find("checksum") != std::string::npos,
                  "message was '" + ce + "'");
        }

        std::string ignored;
        S.delete_save(true, name, ignored);

        if (record_)
            std::printf("  rec   %-46s straight %.10f  resumed %.10f\n",
                        "round trip", straight, resumed);
        else
            close_to("100 + save + load + 100 == 200 straight", resumed, straight, 1e-9);
    }

    // ---- theory ----------------------------------------------------------
    //
    // Everything else in this file compares the solver against itself: the same
    // number twice, from two directions. This one compares it against a result
    // worked out on paper.
    //
    // The polarised river game. OOP holds either the nuts or nothing; IP holds
    // only bluffcatchers, which beat the nothing and lose to the nuts. One bet
    // size, no raises. The equilibrium is a page of algebra with no free
    // parameters:
    //
    //   IP calls                P / (P + B)
    //   OOP bluffs, as a share
    //   of the hands it bets    B / (P + 2B)
    //
    // and from those two, OOP's whole share of the pot.
    //
    // Board Ks Qd 7h 2c 3d: no flush is possible and no straight is possible,
    // so the hand ranks are as plain as they look. OOP holds KK (trip kings) or
    // 54o (nothing at all); IP holds QJo, a pair of queens, which beats 54 and
    // loses to KK. None of the three blocks either of the others, so the toy
    // game is the toy game -- card removal cannot bend it.
    void polarised_toy(Session& S, double betfrac, double rpct, double rcap,
                       int iters) {
        std::string e;
        std::string tag = "toy " + fmt_num(betfrac) + "p";
        if (rpct > 0.0) {
            tag += " rake " + fmt_num(100.0 * rpct) + "%";
            if (rcap > 0.0) tag += " cap " + fmt_num(rcap);
        }
        tag += ": ";
        cfg::POT0  = 20.0;
        cfg::STACK = 100.0;
        cfg::ISO   = true;
        cfg::DCFR_ALPHA = 1.5;
        cfg::DCFR_BETA  = 0.0;
        cfg::DCFR_GAMMA = 2.0;
        cfg::RAKE_PCT   = rpct;
        cfg::RAKE_CAP   = rcap;
        S.clear_all_locks();
        if (!truth(tag + "board", S.set_board("KsQd7h2c3d", e), e)) return;
        if (!truth(tag + "OOP is nuts or nothing", S.set_range(0, "KK,54o", e), e)) return;
        if (!truth(tag + "IP is bluffcatchers", S.set_range(1, "QJo", e), e)) return;
        cfg::MERGE_PCT = 0.0;                 // these toys want exact sizings
        for (int st = 0; st < 3; ++st) {
            if (!S.set_sizings(false, st, std::vector<Sizing>(), e) ||
                !S.set_allin(st, false, e) ||
                !S.set_donks(st, std::vector<Sizing>(), e)) {
                truth(tag + "tree controls", false, e);
                return;
            }
        }
        std::vector<Sizing> bz;
        if (!truth(tag + "one bet size", parse_sizings(fmt_num(100.0 * betfrac) + "%", bz, e) &&
                                         S.set_sizings(true, ST_RIVER, bz, e), e)) return;
        same(tag + "OOP combos", S.live_combos(0), 15);   // 3 x KK + 12 x 54o
        same(tag + "IP combos",  S.live_combos(1), 9);    // 9 x QJo
        S.solve(iters, 0);

        const BetTree& bt = S.tree().ctx[0].tree;
        const int root = bt.root;
        const Node& rn = bt.nodes[static_cast<size_t>(root)];
        const int bx = bt.action_index(rn, AK_BET);
        if (!truth(tag + "OOP can bet", bx >= 0)) return;
        const int ipn = bt.child(rn, bx);
        if (!truth(tag + "and IP then acts", ipn >= 0)) return;
        const Node& in = bt.nodes[static_cast<size_t>(ipn)];
        const int cx = bt.action_index(in, AK_CALL);
        if (!truth(tag + "with a call", cx >= 0)) return;

        DCFRSolver& sol = *S.solver();
        const int nh = sol.num_hands();
        std::vector<double> so(static_cast<size_t>(rn.num_actions) * nh);
        std::vector<double> si(static_cast<size_t>(in.num_actions) * nh);
        sol.avg_strategy_block(0, root, so.data());
        sol.avg_strategy_block(0, ipn, si.data());

        // OOP's hands split into the two the game is made of. This toy is built
        // so the split is exact and known: KK is the nuts here and everything
        // else is air, so the test says which hands they are rather than asking
        // a bucketing rule that no longer exists.
        double vw = 0.0, aw = 0.0, vbet = 0.0, abet = 0.0;
        for (int h = 0; h < nh; ++h) {
            const double w = S.range(0)[static_cast<size_t>(h)];
            if (w <= 0.0) continue;
            const double b = so[static_cast<size_t>(bx) * nh + h];
            if (sol.equity(0, h) > 90.0) { vw += w; vbet += w * b; }
            else                         { aw += w; abet += w * b; }
        }
        double ipw = 0.0, ipcall = 0.0;
        for (int h = 0; h < nh; ++h) {
            const double w = S.range(1)[static_cast<size_t>(h)];
            if (w <= 0.0) continue;
            ipw += w;
            ipcall += w * si[static_cast<size_t>(cx) * nh + h];
        }
        same(tag + "the nuts are the three kings", static_cast<long long>(vw + 0.5), 3);
        same(tag + "and the rest is air", static_cast<long long>(aw + 0.5), 12);
        if (ipw <= 0.0 || vw <= 0.0 || aw <= 0.0) return;

        // Theory. Nothing below is a recorded number -- it is the algebra above,
        // evaluated at this pot and this bet.
        const double P = cfg::POT0, B = betfrac * cfg::POT0;
        // The house takes its cut of the MATCHED pot, so a called river bet
        // is raked on P + 2B and a fold only on P -- the uncalled bet goes
        // back before anyone counts. Written out here rather than called out
        // of cfg::, so this stays a statement about what the rake should be.
        const double cut = (rpct <= 0.0) ? 0.0 : rpct;
        const double rk_fold = (cut <= 0.0) ? 0.0
                             : (rcap > 0.0 ? std::min(cut * P, rcap) : cut * P);
        const double rk_call = (cut <= 0.0) ? 0.0
                             : (rcap > 0.0 ? std::min(cut * (P + 2.0 * B), rcap)
                                           : cut * (P + 2.0 * B));
        // Bluffing wins P less the rake on it; being called costs B. Calling
        // wins the raked pot less the call; folding wins nothing.
        const double call_t  = (P - rk_fold) / (P - rk_fold + B);
        const double bluff_t = B / (P + 2.0 * B - rk_call);
        const double x_t     = bluff_t * vw / (1.0 - bluff_t);   // bluff combos
        const double ev_t    = (vw / (vw + aw)) *
                               (call_t * (P + 2.0 * B - rk_call - B) +
                                (1.0 - call_t) * (P - rk_fold));

        must_be(tag + "IP calls what the pot odds say", ipcall / ipw, call_t, 3e-3);
        must_be(tag + "the nuts always bet", vbet / vw, 1.0, 3e-3);
        must_be(tag + "OOP bluffs just enough to make that right",
                abet / (vbet + abet), bluff_t, 3e-3);
        must_be(tag + "which is that many air combos", abet, x_t, 3e-2);
        must_be(tag + "air is indifferent, so OOP's share is the nuts' alone",
                S.solver()->root_ev(0), ev_t, 5e-3);
    }

    // Exploitability, checked against what it is supposed to mean rather than
    // against what it was last time. In the polarised game the equilibrium is
    // exact, so there is a right answer to converge to and the gap has to shut.
    void exploitability_shuts(Session& S) {
        std::string e;
        cfg::POT0 = 20.0; cfg::STACK = 100.0; cfg::ISO = true;
        cfg::DCFR_ALPHA = 1.5; cfg::DCFR_BETA = 0.0; cfg::DCFR_GAMMA = 2.0;
        cfg::RAKE_PCT = 0.0; cfg::RAKE_CAP = 0.0;
        S.clear_all_locks();
        if (!truth("gap spot builds", S.set_board("KsQd7h2c3d", e), e)) return;
        if (!truth("gap: OOP", S.set_range(0, "KK,54o", e), e)) return;
        if (!truth("gap: IP",  S.set_range(1, "QJo", e), e)) return;
        cfg::MERGE_PCT = 0.0;                 // this toy wants exact sizings
        for (int st = 0; st < 3; ++st)
            if (!S.set_sizings(false, st, std::vector<Sizing>(), e) ||
                !S.set_allin(st, false, e) ||
                !S.set_donks(st, std::vector<Sizing>(), e)) {
                truth("gap: tree controls", false, e); return;
            }
        std::vector<Sizing> bz;
        if (!truth("gap: one bet size", parse_sizings("50", bz, e) &&
                                        S.set_sizings(true, ST_RIVER, bz, e), e)) return;

        S.solve(50, 0);
        const double early = S.solver()->exploitability();
        S.iterate(19950, 0);
        const double late = S.solver()->exploitability();

        must_be("a gap is never negative, early", early < 0.0 ? early : 0.0, 0.0, 0.0);
        must_be("nor late",                       late  < 0.0 ? late  : 0.0, 0.0, 0.0);
        truth("and it shuts by a factor of ten", late * 10.0 < early,
              "from " + std::to_string(early) + " to " + std::to_string(late));

        // The toy has an exact equilibrium, and the average strategy sits on
        // it to six decimals, so the gap has to shut to nothing worth reading.
        // It did not until the best response was pointed at the average
        // strategy instead of the current iterate: 0.173 after two hundred
        // thousand iterations, against 0.00003 now.
        must_be("down to nothing worth reading, on a pot of 20", late, 0.0, 0.002);
    }

    // The same game, but with a bluff that blocks the calling range.
    //
    // OOP's air is now half 54o, which blocks nothing, and half J8o, which
    // removes one of IP's jacks. That changes the arithmetic in a way only a
    // solver that does card removal properly can find: conditional on IP's
    // exact combo, three of OOP's twelve J8 hands are impossible, so OOP is
    // bluffing out of 21 air combos rather than 24, and the frequency that
    // makes IP indifferent is 1/21, not 1/24. A solver that ignored blockers
    // would settle on 4.17% and be wrong by a seventh.
    //
    // Nothing below is recorded. The conditional counts are enumerated here by
    // hand -- combo against combo, sharing a card or not -- and the frequencies
    // come out of the same two formulas as the game above.
    void blocker_toy(Session& S, double betfrac, int iters) {
        std::string e;
        const std::string tag = "blocker toy " + fmt_num(betfrac) + "p: ";
        cfg::POT0  = 20.0;
        cfg::STACK = 100.0;
        cfg::ISO   = true;
        cfg::DCFR_ALPHA = 1.5;
        cfg::DCFR_BETA  = 0.0;
        cfg::DCFR_GAMMA = 2.0;
        cfg::RAKE_PCT   = 0.0;
        cfg::RAKE_CAP   = 0.0;
        S.clear_all_locks();
        if (!truth(tag + "board", S.set_board("KsQd7h2c3d", e), e)) return;
        if (!truth(tag + "OOP: nuts, blank air, blocking air",
                   S.set_range(0, "KK,54o,J8o", e), e)) return;
        if (!truth(tag + "IP: bluffcatchers", S.set_range(1, "QJo", e), e)) return;
        cfg::MERGE_PCT = 0.0;                 // this toy wants exact sizings
        for (int st = 0; st < 3; ++st)
            if (!S.set_sizings(false, st, std::vector<Sizing>(), e) ||
                !S.set_allin(st, false, e) ||
                !S.set_donks(st, std::vector<Sizing>(), e)) {
                truth(tag + "tree controls", false, e);
                return;
            }
        std::vector<Sizing> bz;
        if (!truth(tag + "one bet size", parse_sizings(fmt_num(100.0 * betfrac) + "%", bz, e) &&
                                         S.set_sizings(true, ST_RIVER, bz, e), e)) return;
        same(tag + "OOP combos", S.live_combos(0), 27);   // 3 + 12 + 12
        same(tag + "IP combos",  S.live_combos(1), 9);
        S.solve(iters, 0);

        const Deal& D = S.deal();
        const int nh = D.num();

        // Value or air, decided by the river score rather than by the range
        // text: on this board KK is trips and everything else misses.
        int ipref = -1;
        for (int h = 0; h < nh; ++h)
            if (S.range(1)[static_cast<size_t>(h)] > 0.0) { ipref = h; break; }
        if (!truth(tag + "IP has a combo", ipref >= 0)) return;
        const int* sc = &D.scores[0];
        std::vector<int> oop, ip, value;
        for (int h = 0; h < nh; ++h) {
            if (S.range(0)[static_cast<size_t>(h)] > 0.0) {
                oop.push_back(h);
                value.push_back(sc[h] > sc[ipref] ? 1 : 0);
            }
            if (S.range(1)[static_cast<size_t>(h)] > 0.0) ip.push_back(h);
        }
        long long nv = 0;
        for (size_t i = 0; i < oop.size(); ++i) nv += value[i];
        same(tag + "three of them beat a bluffcatcher", nv, 3);

        const BetTree& bt = S.tree().ctx[0].tree;
        const int root = bt.root;
        const Node& rn = bt.nodes[static_cast<size_t>(root)];
        const int bx = bt.action_index(rn, AK_BET);
        if (!truth(tag + "OOP can bet", bx >= 0)) return;
        const int ipn = bt.child(rn, bx);
        if (!truth(tag + "IP then acts", ipn >= 0)) return;
        const Node& in = bt.nodes[static_cast<size_t>(ipn)];
        const int cx = bt.action_index(in, AK_CALL);
        if (!truth(tag + "IP can call", cx >= 0)) return;

        DCFRSolver& sol = *S.solver();
        std::vector<double> so(static_cast<size_t>(rn.num_actions) * nh);
        std::vector<double> si(static_cast<size_t>(in.num_actions) * nh);
        sol.avg_strategy_block(0, root, so.data());
        sol.avg_strategy_block(0, ipn, si.data());

        const double P = cfg::POT0, B = betfrac * cfg::POT0;
        const double call_t  = P / (P + B);
        const double bluff_t = B / (P + 2.0 * B);

        // Conditional on any one IP combo, how many air combos can OOP still
        // hold? Counted here, not assumed: two combos clash when they share a
        // card.
        int cond_air = 0, cond_val = 0;
        for (size_t i = 0; i < oop.size(); ++i) {
            const Combo& a = D.combos[static_cast<size_t>(oop[i])];
            const Combo& b = D.combos[static_cast<size_t>(ip[0])];
            if (a.c1 == b.c1 || a.c1 == b.c2 || a.c2 == b.c1 || a.c2 == b.c2) continue;
            if (value[i]) ++cond_val; else ++cond_air;
        }
        same(tag + "air combos left when IP holds one", cond_air, 21);
        same(tag + "value combos left", cond_val, 3);

        // The bluff mass that makes IP indifferent, spread over those combos.
        const double bluff_mass = cond_val * bluff_t / (1.0 - bluff_t);
        const double b_each     = bluff_mass / cond_air;
        must_be(tag + "so each air combo bluffs 1/21, not 1/24",
                b_each, 1.0 / 21.0, 1e-12);

        double worst_air = 0.0, worst_val = 0.0;
        for (size_t i = 0; i < oop.size(); ++i) {
            const double f = so[static_cast<size_t>(bx) * nh + oop[i]];
            if (value[i]) worst_val = std::max(worst_val, std::fabs(f - 1.0));
            else          worst_air = std::max(worst_air, std::fabs(f - b_each));
        }
        must_be(tag + "the nuts always bet", worst_val, 0.0, 3e-4);
        // 30k iterations land within 1.4e-5 of the algebra; an engine that ignored
        // card removal settles 4.2e-3 away, so the gap is two orders wide.
        must_be(tag + "every air combo bluffs at that rate", worst_air, 0.0, 3e-4);

        double worst_call = 0.0;
        for (size_t i = 0; i < ip.size(); ++i)
            worst_call = std::max(worst_call,
                std::fabs(si[static_cast<size_t>(cx) * nh + ip[i]] - call_t));
        must_be(tag + "IP still calls P/(P+B)", worst_call, 0.0, 3e-4);

        // The pot share. OOP's air is worth nothing, so the whole of it is the
        // nuts' -- but weighted by how often IP can hold a hand at all, which
        // is where the blockers show up in the answer.
        double wv = 0.0, wtot = 0.0;
        for (size_t i = 0; i < oop.size(); ++i) {
            const Combo& a = D.combos[static_cast<size_t>(oop[i])];
            double w = 0.0;
            for (size_t j = 0; j < ip.size(); ++j) {
                const Combo& b = D.combos[static_cast<size_t>(ip[j])];
                if (a.c1 == b.c1 || a.c1 == b.c2 || a.c2 == b.c1 || a.c2 == b.c2) continue;
                w += 1.0;
            }
            wtot += w;
            if (value[i]) wv += w;
        }
        same(tag + "pairs of hands that can be dealt at once",
             static_cast<long long>(wtot + 0.5), 216);
        must_be(tag + "the nuts are an eighth of the weight, not a ninth",
                wv / wtot, 0.125, 1e-12);
        const double ev_t = (wv / wtot) *
                            (call_t * (P + 2.0 * B) + (1.0 - call_t) * (P + B) - B);
        must_be(tag + "and that is OOP's whole share",
                S.solver()->root_ev(0), ev_t, 5e-3);
    }

    // Adding iterations to a solve has to be the same thing as having asked
    // for them all at once. It runs on the background worker, in chunks whose
    // length adapts to the clock, with read-only exploitability passes in
    // between -- three chances for something to leak into the numbers.
    void solve_continues(Session& S) {
        std::string e;
        if (!truth("continue spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(100, 0);
        same("100 iterations done", S.solver()->iterations_done(), 100);

        if (!truth("adds fifty more", S.solve_more_async(50), "it refused")) return;
        int guard = 0;
        while (S.busy() && guard++ < 20000)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        S.reap();
        truth("and finished", !S.busy(), "still running after 20s");
        same("which makes 150", S.solver()->iterations_done(), 150);
        const double added = S.solver()->root_ev(0);

        Session straight;
        if (!truth("straight spot builds", spot(straight, "Ah9h4hKd", e), e)) return;
        straight.solve(150, 0);
        same("straight 150", straight.solver()->iterations_done(), 150);
        // Not close: the same. The chunking changes when the lock is released,
        // not what the arithmetic is.
        close_to("100 + 50 is exactly 150", added, straight.solver()->root_ev(0), 1e-12);

        // And it has to refuse when there is nothing to add to, rather than
        // quietly starting from zero and calling it a continuation.
        Session fresh;
        truth("refuses when nothing is solved", !fresh.solve_more_async(50),
              "it started anyway");
    }

    // What a solve costs is not just its two big buffers. The limit used to be
    // checked against those alone, so on a small flop the number in front of
    // the user was a fifth low -- and the thing the limit protects is the
    // process, which does not care which vector the bytes are in.
    void memory_accounting(Session& S) {
        std::string e;
        if (!truth("memory spot builds", spot(S, "Ah9h4h", e), e)) return;
        const int    saveth = cfg::THREADS;
        const double savemm = cfg::MAX_MEM_GB;
        cfg::THREADS = 4;                    // the frames are per thread
        const Session::MemUse m = S.memory_use();

        close_to("flop: regret + strategy",  m.buffers, 0.0531982183, 1e-9);
        close_to("flop: discount stamps",    m.stamps,  0.0000738800, 1e-9);
        close_to("flop: the deal's tables",  m.tables,  0.0104410946, 1e-9);
        close_to("flop: the showdown order", m.sweep,   0.0017874241, 1e-9);
        close_to("flop: frames per thread",  m.scratch, 0.0141504407, 1e-9);
        must_be("and the total is the sum of them", m.total,
                m.buffers + m.stamps + m.tables + m.sweep + m.scratch, 1e-12);
        // The whole point: the total is strictly more than the buffers.
        truth("which is more than the buffers alone", m.total > m.buffers * 1.05,
              "total " + std::to_string(m.total) + " vs buffers " +
              std::to_string(m.buffers));

        // A limit set between the buffers and the truth has to refuse. Checking
        // it against the buffers alone would let this through and then take
        // half as much again as the user allowed.
        cfg::MAX_MEM_GB = 0.5 * (m.buffers + m.total);
        S.invalidate();
        S.solve(1, 0);
        truth("a spot that fits only if you ignore half of it is refused",
              !S.solved(), "it built anyway");

        cfg::MAX_MEM_GB = savemm;
        cfg::THREADS    = saveth;
        S.invalidate();
    }

    // The CSV export, read back and checked against the solver that wrote it.
    // It is the file people take to a spreadsheet, which is exactly where a
    // wrong column stops being noticeable.
    void csv_export(Session& S) {
        std::string e;
        if (!truth("csv spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(150, 0);

        const int ci = 0, nid = S.tree().ctx[0].tree.root;
        const NodeStats N = gather(*S.solver(), ci, nid, 0, S.deal().identity());
        if (!truth("the node gathers", N.ok)) return;

        const std::string path = "__check_export.csv";
        if (!truth("writes the file",
                   write_csv(path, *S.solver(), S.deal(), N,
                             board_str(S.deal().board), "R", e), e)) return;

        std::vector<std::string> lines;
        {
            std::ifstream in(path.c_str());
            std::string ln;
            while (std::getline(in, ln)) lines.push_back(ln);
        }
        std::remove(path.c_str());
        if (!truth("and it has a header and rows", lines.size() > 8,
                   std::to_string(lines.size()) + " lines")) return;

        // The header has to name this node's actions, in this node's order.
        int hdr = -1;
        for (size_t i = 0; i < lines.size(); ++i)
            if (lines[i].rfind("combo,class,equity", 0) == 0) { hdr = static_cast<int>(i); break; }
        if (!truth("the header is where it should be", hdr >= 0)) return;
        const std::vector<std::string> cols = split(lines[static_cast<size_t>(hdr)], ',');
        same("one column per field plus two per action",
             static_cast<long long>(cols.size()), 6 + 2 * N.A + 1);
        bool named = true;
        for (int a = 0; a < N.A; ++a) {
            if (cols[static_cast<size_t>(6 + 2 * a)]     != "freq_" + N.codes[static_cast<size_t>(a)] ||
                cols[static_cast<size_t>(6 + 2 * a + 1)] != "ev_"   + N.codes[static_cast<size_t>(a)])
                named = false;
        }
        truth("and every action is named in it", named);

        // Every row: the frequencies are a strategy, and the node EV is the
        // strategy's own average of the action EVs. Both are identities the
        // file has to carry on its own, without the solver next to it.
        int rows = 0;
        double worst_sum = 0.0, worst_ev = 0.0, wsum = 0.0, rsum = 0.0;
        double peor_razon = 1e18;
        bool mal_orden = false;
        for (size_t i = static_cast<size_t>(hdr) + 1; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            const std::vector<std::string> c = split(lines[i], ',');
            if (c.size() != cols.size()) { truth("every row is the right width", false,
                                                 "line " + std::to_string(i)); return; }
            ++rows;
            double w = 0.0, rc = 0.0;
            parse_double(c[4], w);
            parse_double(c[5], rc);
            wsum += w;
            rsum += rc;
            // El alcance nunca puede pasar de la masa de parejas: es esa
            // masa dividida por los combos del rival que no te bloquean.
            if (rc > w + 1e-9) mal_orden = true;
            if (rc > 1e-9) peor_razon = std::min(peor_razon, w / rc);
            double fs = 0.0, mix = 0.0, evn = 0.0;
            for (int a = 0; a < N.A; ++a) {
                double fq = 0.0, ev = 0.0;
                parse_double(c[static_cast<size_t>(6 + 2 * a)], fq);
                parse_double(c[static_cast<size_t>(6 + 2 * a + 1)], ev);
                fs  += fq;
                mix += fq * ev;
            }
            parse_double(c[cols.size() - 1], evn);
            worst_sum = std::max(worst_sum, std::fabs(fs - 1.0));
            worst_ev  = std::max(worst_ev,  std::fabs(mix - evn));
        }
        same("one row per combo the player can hold", rows, 188);
        must_be("the frequencies on every row are a strategy", worst_sum, 0.0, 2e-6);
        must_be("and the node EV is that strategy's own average", worst_ev, 0.0, 2e-5);
        close_to("the weights add up to the range's reach", wsum, 26571.0000000000, 1e-3);

        // Y la columna que SI es el rango. `weight` es masa de parejas y no
        // sirve para leer un rango: quien abria la hoja y leia "weight" leia
        // el alcance multiplicado por los quinientos y pico combos del rival
        // que no le bloquean. `reach` es el alcance a secas, y es lo que hay
        // que poner al lado del show_range de otro solver.
        close_to("and the reach column is the range itself",
                 rsum, N.combos(), 1e-6);
        truth("no hand reaches more than its pair mass", !mal_orden);
        truth("and the two columns are not the same number",
              peor_razon > 100.0,
              "la razon mas floja weight/reach fue " + std::to_string(peor_razon));
    }

    // The text report, read back. It is written for eyes rather than for a
    // spreadsheet, so the exact wording is nobody's business -- but the grids
    // are a strategy printed one action at a time, and a strategy has to add
    // up whichever way it is sliced. One 13x13 per action, and the same square
    // across all of them is either a hand nobody holds in every grid, or it is
    // 100%.
    void report_grids_add_up(Session& S) {
        std::string e;
        if (!truth("report spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(150, 0);

        const std::string path = "__check_report.txt";
        {
            std::FILE* f = std::fopen(path.c_str(), "w");
            if (!truth("opens the file", f != nullptr, path)) return;
            write_report(f, *S.solver(), S.tree(), S.deal());
            std::fclose(f);
        }
        std::vector<std::string> lines;
        {
            std::ifstream in(path.c_str());
            std::string ln;
            while (std::getline(in, ln)) lines.push_back(ln);
        }
        std::remove(path.c_str());
        if (!truth("the report has content", lines.size() > 40,
                   std::to_string(lines.size()) + " lines")) return;

        const int root = S.tree().ctx[0].tree.root;
        const int A = S.tree().ctx[0].tree.nodes[static_cast<size_t>(root)].num_actions;

        // Every grid is the 13 rows that follow a "frequency at" heading.
        std::vector<std::vector<double>> grids;   // [grid][169], -1 for '.'
        for (size_t i = 0; i + 15 < lines.size(); ++i) {
            if (lines[i].find("frequency at") == std::string::npos) continue;
            std::vector<double> g;
            for (size_t r = i + 3; r < i + 16 && r < lines.size(); ++r) {
                const std::vector<std::string> tok = tokenize(lines[r]);
                if (tok.size() != 14) { g.clear(); break; }
                for (size_t c = 1; c < tok.size(); ++c) {
                    if (tok[c] == ".") g.push_back(-1.0);
                    else {
                        double v = 0.0;
                        if (!parse_double(tok[c], v)) { g.clear(); c = tok.size(); r = i + 99; break; }
                        g.push_back(v);
                    }
                }
            }
            if (g.size() == 169) grids.push_back(g);
        }
        same("one grid per action at the root",
             static_cast<long long>(grids.size()), A);
        if (static_cast<int>(grids.size()) != A) return;

        int live = 0, dead = 0;
        double worst = 0.0;
        bool ragged = false;
        for (int k = 0; k < 169; ++k) {
            int shown = 0;
            double sum = 0.0;
            for (int a = 0; a < A; ++a) {
                const double v = grids[static_cast<size_t>(a)][static_cast<size_t>(k)];
                if (v < 0.0) continue;
                ++shown;
                sum += v;
            }
            if (shown == 0) { ++dead; continue; }
            // A class either appears in every grid or in none: the grids are
            // the same range sliced by action, not different ranges.
            if (shown != A) ragged = true;
            ++live;
            worst = std::max(worst, std::fabs(sum - 100.0));
        }
        truth("a class is in every grid or in none", !ragged);
        same("classes the player holds", live, 42);
        same("and classes nobody holds", dead, 127);
        // Printed as whole percents, so the rounding is worth half a point per
        // action and no more.
        must_be("every square adds to 100 across the actions", worst, 0.0,
                0.5 * A + 1e-9);

        // The game value line has to be the game value.
        double printed = -1.0;
        for (const std::string& ln : lines) {
            const size_t p = ln.find("Game value: OOP ");
            if (p == std::string::npos) continue;
            parse_double(tokenize(ln.substr(p + 16))[0], printed);
        }
        must_be("and the game value printed is the game value",
                printed, S.solver()->root_ev(0), 5e-5);
    }

    // The 13x13 grid, the CSV's class column, the bucket rows and the report's
    // grids all come out of aggregate_by_class. It is a weighted average, and a
    // weighted average has properties that a wrong one does not: it conserves
    // the weight it was given, it produces a distribution, and every number it
    // produces lies between the smallest and the largest it averaged. None of
    // those is the formula written twice -- a wrong divisor, a mis-indexed
    // class or a missed combo breaks at least one of them.
    void class_aggregate_is_an_average(Session& S) {
        std::string e;
        if (!truth("aggregate spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(150, 0);

        const int root = S.tree().ctx[0].tree.root;
        // A lock, so the per-class lock flag is exercised rather than assumed.
        std::vector<std::pair<std::string, double>> mix;
        mix.push_back(std::make_pair(std::string("B"), 1.0));
        int matched = 0;
        if (!truth("locks QQ+", S.add_lock(0, root, "QQ+", mix, matched, e), e)) return;
        S.solve(80, 0);

        const NodeStats N = gather(*S.solver(), 0, root, 0, S.deal().identity());
        if (!truth("the root gathers", N.ok)) return;
        DCFRSolver& s2 = *S.solver();
        const std::vector<ClassAgg> agg = aggregate_by_class(s2, N);
        same("classes with weight on this board",
             static_cast<long long>(agg.size()), 42);

        // 1. Weight in equals weight out.
        double wall = 0.0;
        for (int h = 0; h < N.nh; ++h) {
            const double w = N.weight[static_cast<size_t>(h)];
            if (w > 1e-12) wall += w;
        }
        double wagg = 0.0;
        for (const ClassAgg& g : agg) wagg += g.w;
        must_be("the classes hold all the weight and no more", wagg, wall, 1e-9);

        // 2. Each class is a strategy.
        // 3. Each averaged number is bracketed by what it averaged.
        double worst_sum = 0.0;
        int outside = 0, wrongcls = 0, badlock = 0;
        for (const ClassAgg& g : agg) {
            double fs = 0.0;
            for (int a = 0; a < N.A; ++a) fs += g.freq[static_cast<size_t>(a)];
            worst_sum = std::max(worst_sum, std::fabs(fs - 1.0));

            double eqlo = 1e18, eqhi = -1e18, evlo = 1e18, evhi = -1e18;
            bool anylock = false;
            int members = 0;
            for (int h = 0; h < N.nh; ++h) {
                if (N.weight[static_cast<size_t>(h)] <= 1e-12) continue;
                if (S.deal().combos[static_cast<size_t>(h)].cls != g.cls) continue;
                ++members;
                const double q = s2.equity(N.player, h), v = N.ev_node(h);
                eqlo = std::min(eqlo, q); eqhi = std::max(eqhi, q);
                evlo = std::min(evlo, v); evhi = std::max(evhi, v);
                if (s2.is_hand_locked(N.ci, N.nid, N.inst, N.stored(h))) anylock = true;
            }
            if (members == 0) { ++wrongcls; continue; }
            const double sl = 1e-9;
            if (g.eq      < eqlo - sl || g.eq      > eqhi + sl) ++outside;
            if (g.node_ev < evlo - sl || g.node_ev > evhi + sl) ++outside;
            if (g.locked != anylock) ++badlock;
        }
        must_be("every class is a strategy", worst_sum, 0.0, 1e-9);
        same("every class has members", wrongcls, 0);
        same("no average falls outside what it averaged", outside, 0);
        same("a class is flagged locked exactly when one of its combos is",
             badlock, 0);

        // 4. Sorted strongest first, which is what every caller assumes.
        bool sorted = true;
        for (size_t i = 1; i < agg.size(); ++i)
            if (agg[i - 1].eq < agg[i].eq - 1e-12) sorted = false;
        truth("and they come back sorted by equity", sorted);

        // Everything above reads the node through the identity permutation,
        // where a combo's index and the index it is stored under are the same
        // number -- so a lock lookup that used the wrong one of the two would
        // pass unnoticed. Read the same node through a runout that needs a
        // permutation and the two come apart.
        {
            const GameTree& T = S.tree();
            const int cn = T.find_node(0, "R/X/X");
            int deep = -1, moved = 0;
            long long inst = 0;
            int perm = S.deal().identity();
            if (cn >= 0) {
                const int child = T.ctx[0].cont_ctx[
                    static_cast<size_t>(T.ctx[0].tree.nodes[static_cast<size_t>(cn)].cont_id)];
                if (child >= 0) deep = child;
            }
            if (truth("there is a street below the root", deep >= 0)) {
                // Find a card whose runout is reached by a real permutation.
                for (int slot = 0; slot < static_cast<int>(S.deal().deck.size()); ++slot) {
                    std::vector<int> one(1, slot);
                    long long i2 = 0;
                    int p2 = S.deal().identity();
                    if (!S.deal().locate(one, i2, p2)) continue;
                    if (p2 == S.deal().identity()) continue;
                    inst = i2; perm = p2; moved = 1;
                    break;
                }
            }
            if (truth("and a runout that needs a permutation to reach", moved == 1)) {
                const int droot = S.tree().ctx[static_cast<size_t>(deep)].tree.root;
                const NodeStats P = gather(s2, deep, droot, inst, perm);
                if (truth("the permuted node gathers", P.ok)) {
                    int shifted = 0;
                    for (int h = 0; h < P.nh; ++h) if (P.stored(h) != h) ++shifted;
                    truth("and the permutation really moves combos", shifted > 0,
                          "nothing moved");
                    // Lock something there and check the class flag follows the
                    // combos through the permutation, not past it.
                    int m2 = 0;
                    std::string e2;
                    if (truth("locks QQ+ at the permuted node",
                              S.add_lock(deep, droot, "QQ+", mix, m2, e2), e2)) {
                        S.solve(60, 0);
                        DCFRSolver& s3 = *S.solver();
                        const NodeStats Q = gather(s3, deep, droot, inst, perm);
                        const std::vector<ClassAgg> ag2 = aggregate_by_class(s3, Q);
                        int bad2 = 0;
                        for (const ClassAgg& g : ag2) {
                            bool any = false;
                            for (int h = 0; h < Q.nh; ++h) {
                                if (Q.weight[static_cast<size_t>(h)] <= 1e-12) continue;
                                if (S.deal().combos[static_cast<size_t>(h)].cls != g.cls) continue;
                                if (s3.is_hand_locked(Q.ci, Q.nid, Q.inst, Q.stored(h))) any = true;
                            }
                            if (g.locked != any) ++bad2;
                        }
                        same("the lock flag follows the permutation", bad2, 0);
                    }
                }
            }
        }

        S.clear_all_locks();
    }

    // Collapsing suits is only legal while nothing in the spot tells the suits
    // apart. Ranges were tested for that from the start. Locks were not -- and
    // `lock AsKs` pins one spade combo and leaves its heart twin free, which is
    // exactly the thing suit collapsing assumes cannot happen.
    void iso_bows_to_an_asymmetric_lock(Session& S) {
        std::string e;
        if (!truth("iso-lock spot builds", spot(S, "Ah9h4h", e), e)) return;
        truth("suits collapse to begin with", S.deal().iso_on);
        truth("and nothing has turned that down", !S.iso_off_by_lock());

        const int root = S.tree().ctx[0].tree.root;
        std::vector<std::pair<std::string, double>> bet;
        bet.push_back(std::make_pair(std::string("B"), 1.0));
        int m = 0;

        // A lock that names ranks only: every suit is treated alike, so it is
        // no threat to the collapsing.
        if (!truth("locks QQ+", S.add_lock(0, root, "QQ+", bet, m, e), e)) return;
        truth("a rank-only lock leaves suits collapsed", S.deal().iso_on);
        same("by the whole group", static_cast<long long>(S.deal().use_group.size()), 6);
        truth("and says so", !S.iso_off_by_lock());
        S.clear_all_locks();

        // A rank-only selector spanning many classes is no threat either: it
        // names no card, so every suit permutation maps it onto itself.
        S.solve(60, 0);
        if (!truth("locks a wide rank-only range", S.add_lock(0, root, "22+,A2s+", bet, m, e), e)) return;
        truth("a wide rank-only lock leaves suits collapsed", S.deal().iso_on);
        S.clear_all_locks();

        // One that names cards is. The rebuild waits for the solve, though:
        // doing it the moment the lock is set would throw away the strategy
        // being looked at, and looking at it is the entire point of locking.
        if (!truth("locks AsKs", S.add_lock(0, root, "AsKs", bet, m, e), e)) return;
        same("which is one combo", m, 1);
        truth("the tree is not rebuilt yet", S.deal().iso_on);
        truth("but it is queued", S.iso_resync_pending());
        S.solve(300, 0);
        // Not off: cut down. `AsKs` pins a spade, so any permutation moving
        // spades is out, and the swap between the two suits the lock says
        // nothing about is still a legal thing to collapse by.
        truth("a lock that names cards still collapses", S.deal().iso_on);
        same("by what is left of the group",
             static_cast<long long>(S.deal().use_group.size()), 2);
        truth("and says the lock cut it down", S.iso_off_by_lock());
        truth("with nothing left queued", !S.iso_resync_pending());
        const double collapsed_off = S.solver()->root_ev(0);

        // The reason it is deferred, stated as a property: the strategy on
        // screen survives setting a lock, so a second group can be read off it.
        // Doing this eagerly made the second edit impossible.
        truth("and the strategy survived the first lock long enough to read",
              S.solver()->iterations_done() > 0);

        // And the answer has to be the answer you get with collapsing switched
        // off by hand -- that is the whole claim.
        cfg::ISO = false;
        if (!truth("rebuilds without collapsing", S.rebuild(e), e)) return;
        S.solve(300, 0);
        // 1e-6, not 1e-9: the two sum the same numbers in a different order.
        truth("and it is the same answer as never collapsing at all",
              std::fabs(S.solver()->root_ev(0) - collapsed_off) < 1e-6,
              "gap " + fmt_sci(std::fabs(S.solver()->root_ev(0) - collapsed_off)));
        cfg::ISO = true;

        // Turning the global back on is a rebuild in the console and in the
        // browser both, so it is one here too -- resync_iso() watches the
        // lock verdict, not the global behind its back.
        S.clear_all_locks();
        if (!truth("rebuilds after clearing", S.rebuild(e), e)) return;
        truth("clearing the lock brings collapsing back", S.deal().iso_on);
        same("by the whole group again",
             static_cast<long long>(S.deal().use_group.size()), 6);
        truth("and nothing is turning it down any more", !S.iso_off_by_lock());

        // Two locks whose UNION is symmetric and which are not.
        //
        // `AsKs` and `AdKd` together cover a set the swap (s d) maps onto
        // itself -- so a single merged mask, which is what this used to build,
        // calls the pair symmetric. It is not: the swap carries a combo locked
        // to betting onto one locked to checking. Each lock has to be asked
        // separately, and then the swap is out and so is every other one.
        S.solve(60, 0);
        std::vector<std::pair<std::string, double>> chk;
        chk.push_back(std::make_pair(std::string("X"), 1.0));
        if (!truth("locks AsKs to betting", S.add_lock(0, root, "AsKs", bet, m, e), e)) return;
        if (!truth("and AdKd to checking", S.add_lock(0, root, "AdKd", chk, m, e), e)) return;
        S.solve(60, 0);
        // The trap, stated as a number: the merged mask leaves the swap (s d)
        // in the group, because the union does survive it.
        same("a merged mask would have kept the swap between them",
             static_cast<long long>(
                 S.deal().keep_part(S.deal().base_group, union_mask(S)).size()), 2);
        same("but asked one at a time there is no group left",
             static_cast<long long>(S.deal().use_group.size()), 1);
        truth("so nothing is collapsed", !S.deal().iso_on);
        S.clear_all_locks();
    }

    // The merged mask the old code would have built for those two locks, kept
    // only so the check above can say what it is testing.
    std::vector<double> union_mask(Session& S) {
        std::vector<double> mask(static_cast<size_t>(S.deal().num()), 0.0);
        std::vector<std::vector<double>> per;
        S.lock_masks(per);
        for (const std::vector<double>& w : per)
            for (size_t h = 0; h < mask.size(); ++h)
                if (w[h] > 0.0) mask[h] = 1.0;
        return mask;
    }

    // The same claim for ranges, which is where the symmetry test started and
    // the only place it mattered until locks arrived. Nothing tested it: with
    // range_symmetric() hardwired to say yes, the whole suite passed except the
    // lock checks written the night before.
    //
    // What is collapsed by is no longer all-or-nothing. A range that survives
    // half the board's group gets that half, and the claim underneath is
    // unchanged and is the only one that matters: whatever is collapsed, the
    // answer has to be the answer with collapsing switched off.
    void iso_bows_to_an_asymmetric_range(Session& S) {
        std::string e;
        if (!truth("iso-range spot builds", spot(S, "Ah9h4h", e), e)) return;
        same("the board's suit group", static_cast<long long>(S.deal().base_group.size()), 6);
        truth("suits collapse with the standard ranges", S.deal().iso_on);
        same("and by the whole group",
             static_cast<long long>(S.deal().use_group.size()), 6);

        // Every suit named: still symmetric.
        if (!truth("a range that names all four suits", S.set_range(0, "22+,ATo+,AKs", e), e)) return;
        truth("leaves suits collapsed", S.deal().iso_on);
        same("by the whole group still",
             static_cast<long long>(S.deal().use_group.size()), 6);

        // Invariant under one swap but not under the group. Ah9h4h fixes hearts
        // and permutes spades, diamonds and clubs, so a range holding two of
        // those three survives the swap between them and nothing else -- and
        // that swap is a legal thing to collapse by, so it is kept.
        if (!truth("a range symmetric under one swap only",
                   S.set_range(0, "22+,ATo+,AsKs,AdKd", e), e)) return;
        truth("still collapses", S.deal().iso_on);
        same("but only by the swap it survives",
             static_cast<long long>(S.deal().use_group.size()), 2);

        // And the plain asymmetric case: naming one suit leaves the swap
        // between the other two.
        if (!truth("a range that names one suit", S.set_range(0, "22+,ATo+,AsKs", e), e)) return;
        truth("keeps collapsing", S.deal().iso_on);
        same("by the two suits it says nothing about",
             static_cast<long long>(S.deal().use_group.size()), 2);
        truth("and it is not a lock doing any of this", !S.iso_off_by_lock());

        S.solve(700, 0);
        const double part = S.solver()->root_ev(0);

        cfg::ISO = false;
        if (!truth("rebuilds with collapsing switched off by hand", S.rebuild(e), e)) return;
        same("and then there is no group left",
             static_cast<long long>(S.deal().use_group.size()), 1);
        S.solve(700, 0);
        const double none = S.solver()->root_ev(0);
        cfg::ISO = true;

        // Not to 1e-9. The two runs add the same numbers in a different order
        // -- one sums over orbits, the other over every runout -- so they walk
        // slightly different paths through 2000 iterations of a solve that has
        // not fully converged. What matters is that the gap is the size of that
        // and not the size of a wrong collapse, which on the board below was
        // 0.09 chips.
        truth("the part-collapsed answer is the uncollapsed one",
              std::fabs(part - none) < 1e-3,
              "gap " + fmt_sci(std::fabs(part - none)));

        if (!truth("a symmetric range again", S.set_range(0, OOP_RANGE(), e), e)) return;
        if (!truth("rebuilds", S.rebuild(e), e)) return;
        truth("brings collapsing back", S.deal().iso_on);
    }

    // A permutation the flop's group does not have.
    //
    // Runouts are collapsed in two levels, and the second one used the group of
    // the four-card board -- which is not a subgroup of the flop's. `Ks Kh 2s`
    // with a `2h` turn admits the swap (s h): it maps the four-card board onto
    // itself, and since only the five-card set matters at the river it really
    // is a symmetry of the board there. It is not a symmetry of the spot,
    // because nothing had asked the ranges about it.
    //
    // With a range holding no hearts, every spade river came back as the mirror
    // of the range -- the node reported 22 heart combos for a player who could
    // not hold one -- the game value moved by 0.09 chips, and the exploitability
    // printed NEGATIVE, which no equilibrium can be. Found by comparing against
    // the reference solver, which stores every runout separately and so cannot have it.
    void a_symmetry_the_flop_does_not_have(Session& S) {
        std::string e;
        if (!truth("two-level iso spot builds", spot(S, "KsKh2s", e), e)) return;
        same("the flop's group is the two suits it does not show",
             static_cast<long long>(S.deal().base_group.size()), 2);

        // Hearts are on the board, so a range without them is a legal range and
        // is still symmetric under the swap the flop does allow.
        if (!truth("a range with no hearts at all",
                   S.set_range(0, "22+,AJo+,AsKs,AcKc,AdKd", e), e)) return;
        truth("still collapses", S.deal().iso_on);
        same("by the flop's whole group",
             static_cast<long long>(S.deal().use_group.size()), 2);

        S.solve(1200, 0);
        const double on = S.solver()->root_ev(0);
        const double expl_on = S.solver()->exploitability();

        cfg::ISO = false;
        if (!truth("rebuilds without collapsing", S.rebuild(e), e)) return;
        S.solve(1200, 0);
        const double off = S.solver()->root_ev(0);
        cfg::ISO = true;

        truth("collapsing does not move the answer", std::fabs(on - off) < 1e-3,
              "gap " + fmt_sci(std::fabs(on - off)));
        // The symptom, stated as its own property: a wrong collapse compares two
        // best responses computed in frames that do not match, and the
        // difference comes out below zero.
        truth("and the exploitability is a number a game can have", expl_on >= 0.0,
              "exploitability " + fmt_sci(expl_on));

        if (!truth("a symmetric range again", S.set_range(0, OOP_RANGE(), e), e)) return;
        if (!truth("rebuilds", S.rebuild(e), e)) return;
    }

    // The board's suit group is what every collapsed runout rests on, and the
    // pinned node counts catch a wrong one -- but only because they were
    // recorded with the right one. This works it out from the definition
    // instead: of the 24 ways to relabel the four suits, keep the ones that map
    // the board's card set onto itself. Cards are rank*4+suit, so relabelling a
    // suit is arithmetic the check can do without asking the solver anything.
    void the_board_group_is_the_group(Session& S) {
        struct Case { const char* board; int size; const char* why; };
        const Case cases[4] = {
            { "Ah9h4h",  6, "monotone: hearts are pinned, the other three are free to swap" },
            { "Ah9h4d",  2, "two suits used: only the two unused ones can trade places" },
            { "Ah9d4c",  1, "rainbow: three suits pinned, and one alone can only stay" },
            { "AhAd9c",  2, "a pair: its two suits can swap with each other" },
        };
        for (int k = 0; k < 4; ++k) {
            const std::string tag = std::string(cases[k].board) + ": ";
            std::string e;
            if (!truth(tag + "board builds", S.set_board(cases[k].board, e), e)) continue;

            std::vector<int> board;
            if (!truth(tag + "board parses", parse_board(cases[k].board, board, e), e)) continue;

            // All 24 relabellings of the four suits, longhand.
            std::vector<int> mine;
            int order[4] = { 0, 1, 2, 3 };
            for (int a = 0; a < 4; ++a)
            for (int b = 0; b < 4; ++b) {
                if (b == a) continue;
                for (int c = 0; c < 4; ++c) {
                    if (c == a || c == b) continue;
                    const int d = 6 - a - b - c;
                    order[0] = a; order[1] = b; order[2] = c; order[3] = d;
                    bool fixes = true;
                    for (size_t i = 0; i < board.size() && fixes; ++i) {
                        const int moved = (board[i] >> 2) * 4 + order[board[i] & 3];
                        bool found = false;
                        for (size_t j = 0; j < board.size(); ++j)
                            if (board[j] == moved) { found = true; break; }
                        if (!found) fixes = false;
                    }
                    if (!fixes) continue;
                    // Which of the solver's 24 permutations is this one?
                    // Matched over all 52 cards, not just four, so this also
                    // says that perm_card really is a relabelling of suits
                    // that leaves every rank alone.
                    for (int p = 0; p < 24; ++p) {
                        bool same_perm = true;
                        for (int cd = 0; cd < 52 && same_perm; ++cd)
                            if (S.deal().perm_card[p][cd] != (cd >> 2) * 4 + order[cd & 3])
                                same_perm = false;
                        if (same_perm) { mine.push_back(p); break; }
                    }
                }
            }
            std::sort(mine.begin(), mine.end());

            std::vector<int> theirs = S.deal().base_group;
            std::sort(theirs.begin(), theirs.end());
            same(tag + cases[k].why, static_cast<long long>(mine.size()), cases[k].size);
            truth(tag + "the solver's group is that group, exactly",
                  mine == theirs,
                  "worked out " + std::to_string(mine.size()) + ", solver has " +
                  std::to_string(theirs.size()));
        }
    }

    // A saved tree carries its locks. Without that, loading a solve back gives
    // you the frozen strategy in the buffers but nothing holding it there: the
    // next `iterate` lets it drift, and the file you saved stops describing the
    // spot you saved. The locks ride in the config header, so this also covers
    // a plain config save.
    void locks_survive_a_save(Session& S) {
        std::string e;
        const std::string name = "__check_locks";
        if (!truth("lock save spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(60, 0);

        const int root = S.tree().ctx[0].tree.root;
        const Node& rn = S.tree().ctx[0].tree.nodes[static_cast<size_t>(root)];
        const int bx = S.tree().ctx[0].tree.action_index(rn, AK_BET);
        if (!truth("root bets", bx >= 0)) return;

        std::vector<std::pair<std::string, double>> mix;
        mix.push_back(std::make_pair(std::string("B"), 1.0));
        mix.push_back(std::make_pair(std::string("X"), 3.0));
        int m = 0;
        // One selector spelled as a range and one spelled as a bucket: the
        // bucket form cannot be resolved until hand types exist, so it is the
        // one that would break if locks were restored before the solver was.
        if (!truth("locks a range", S.add_lock(0, root, "QQ+", mix, m, e), e)) return;
        std::vector<std::pair<std::string, double>> allbet;
        allbet.push_back(std::make_pair(std::string("B"), 1.0));
        if (!truth("and another, deeper in", lock_first_child(S, root, allbet, e), e)) return;
        S.solve(120, 0);
        if (!truth("saves it", S.save_tree(name, e), e)) return;

        {
            Session fresh;
            if (!truth("loads it into an empty session", fresh.load_tree(name, e), e)) return;
            same("both locks came back",
                 static_cast<long long>(fresh.locks().size()), 2);
            same("and none were dropped on the way", fresh.dropped_locks(), 0);

            // The selector text has to survive too, not just the node address:
            // it is what re-resolves to combos on the far side.
            bool spec_ok = false;
            for (const LockSpec& L : fresh.locks())
                if (L.hand_spec == "QQ+") spec_ok = true;
            truth("the selector text came back", spec_ok);

            const int nh = fresh.solver()->num_hands();
            std::vector<double> st(static_cast<size_t>(rn.num_actions) * nh);
            fresh.solver()->avg_strategy_block(0, root, st.data());
            double worst = 0.0;
            int locked = 0;
            for (int h = 0; h < nh; ++h) {
                if (!fresh.solver()->is_hand_locked(0, root, 0, h)) continue;
                ++locked;
                worst = std::max(worst, std::fabs(st[static_cast<size_t>(bx) * nh + h] - 0.25));
            }
            same("the same combos are locked after loading", locked, 12);
            truth("and still to the same mix", worst < 1e-9,
                  "worst " + std::to_string(worst));

            // The point of carrying the lock rather than just the numbers: more
            // iterations must not let the frozen hands drift.
            fresh.iterate(120, 0);
            fresh.solver()->avg_strategy_block(0, root, st.data());
            double drift = 0.0;
            for (int h = 0; h < nh; ++h)
                if (fresh.solver()->is_hand_locked(0, root, 0, h))
                    drift = std::max(drift, std::fabs(st[static_cast<size_t>(bx) * nh + h] - 0.25));
            truth("and solving on does not let them drift", drift < 1e-9,
                  "drifted " + std::to_string(drift));
        }

        std::string ignored;
        S.delete_save(true, name, ignored);
        S.clear_all_locks();
    }

    // Locks a bucket selector at the first decision node below `root`, to get a
    // second lock that is addressed differently from the first.
    bool lock_first_child(Session& S, int root,
                          const std::vector<std::pair<std::string, double>>& mix,
                          std::string& e) {
        const BetTree& bt = S.tree().ctx[0].tree;
        const Node& rn = bt.nodes[static_cast<size_t>(root)];
        for (int a = 0; a < rn.num_actions; ++a) {
            const int c = bt.child(rn, a);
            if (c < 0) continue;
            const Node& cn = bt.nodes[static_cast<size_t>(c)];
            if (cn.type != NT_DECISION) continue;
            if (bt.action_index(cn, AK_BET) < 0) continue;
            int m = 0;
            return S.add_lock(0, c, "22+", mix, m, e);
        }
        e = "no decision node below the root";
        return false;
    }

    // The aggregate over every card that can come. Its strongest structural
    // property is that cards equivalent under the board's suit symmetry must
    // come out identical -- if they do not, either the aggregation or the
    // isomorphism is wrong, and both are worth catching.
    void runout_table(Session& S) {
        std::string e;
        if (!truth("runout spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(200, 0);

        const GameTree& T = S.tree();
        const int nid = T.find_node(0, "R/X/X");
        if (!truth("finds the river chance node", nid >= 0)) return;

        const std::vector<int> none;
        const RunoutTable t = aggregate_runouts(*S.solver(), 0, nid, none);
        if (!truth("aggregates", t.ok, t.note)) return;

        int rows = 0;
        for (const RunoutRow& r : t.rows) if (r.ok) ++rows;
        same("one row per remaining card", rows, 48);

        // Zero-sum, exactly, on every single card.
        double worst = 0.0;
        std::string worst_at;
        for (const RunoutRow& r : t.rows) {
            if (!r.ok) continue;
            const double d = std::fabs(r.ev_oop + r.ev_ip - cfg::POT0);
            if (d > worst) { worst = d; worst_at = r.name; }
        }
        truth("every runout row has OOP+IP = pot", worst < 1e-9,
              "worst " + std::to_string(worst) + " on " + worst_at);

        double fsum = 0.0;
        for (double f : t.avg_freq) fsum += f;
        close_to("the weighted average is a distribution", fsum, 1.0, 1e-9);

        // Ah9h4hKd leaves spades and clubs entirely unseen, so for any rank the
        // spade and the club are the same card as far as the solve is concerned.
        int compared = 0;
        double suit_gap = 0.0;
        std::string gap_at;
        for (const RunoutRow& a : t.rows) {
            if (!a.ok || a.name.size() != 2 || a.name[1] != 's') continue;
            for (const RunoutRow& b : t.rows) {
                if (!b.ok || b.name.size() != 2 || b.name[1] != 'c') continue;
                if (b.name[0] != a.name[0]) continue;
                ++compared;
                for (size_t k = 0; k < a.freq.size() && k < b.freq.size(); ++k) {
                    const double d = std::fabs(a.freq[k] - b.freq[k]);
                    if (d > suit_gap) { suit_gap = d; gap_at = a.name + " vs " + b.name; }
                }
                const double d = std::fabs(a.ev_oop - b.ev_oop);
                if (d > suit_gap) { suit_gap = d; gap_at = a.name + " vs " + b.name; }
            }
        }
        // Neither suit appears on Ah9h4hKd, so all thirteen ranks pair up.
        // Neither suit appears on Ah9h4hKd, so all thirteen ranks pair up.
        truth("compared every spade against its club", compared == 13,
              "compared " + std::to_string(compared) + " pairs, expected 13");
        truth("unseen suits give identical rows", suit_gap < 1e-9,
              "worst gap " + std::to_string(suit_gap) + " at " + gap_at);

        // Concrete numbers, not just internal consistency. Asserting that the
        // two EVs add up to the pot proves nothing here, because one is derived
        // from the other; a change to the reading layer's arithmetic would move
        // both together and the sum would still hold. These do have teeth.
        const RunoutRow* ks = nullptr;
        for (const RunoutRow& row : t.rows) if (row.ok && row.name == "Ks") ks = &row;
        if (truth("finds the Ks row", ks != nullptr)) {
            close_to("Ks: check frequency", 100.0 * ks->freq[0], 60.5070366882, 1e-6);
            close_to("Ks: reach",           ks->reach,           36.1885995918, 1e-6);
            close_to("Ks: EV for OOP",      ks->ev_oop,          10.3734880009, 1e-6);
        }
        close_to("weighted average check frequency", 100.0 * t.avg_freq[0],
                 62.5354310274, 1e-6);
        close_to("weighted average EV for OOP", t.avg_ev_oop, 10.7065021768, 1e-6);
    }

    // Reading a node on a runout that is not the stored one means rotating every
    // per-combo vector through a permutation. On a turn board the stabiliser has
    // order two and every element is its own inverse, so inverting or not makes
    // no difference and a bug there is invisible. A monotone flop has a group of
    // six, with three-cycles in it, and there the distinction is real.
    void permuted_reads(Session& S) {
        std::string e;
        if (!truth("permuted-read spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(120, 0);

        const GameTree& T = S.tree();
        const int nid = T.find_node(0, "R/X/X");
        if (!truth("finds the turn chance node", nid >= 0)) return;
        const std::vector<int> none;
        const RunoutTable t = aggregate_runouts(*S.solver(), 0, nid, none);
        if (!truth("aggregates over turns", t.ok, t.note)) return;

        // Ah9h4h leaves spades, diamonds and clubs all unseen, so for any rank
        // the three of them are the same card. Comparing all three pins the
        // three-cycles, which comparing a pair never could.
        int trios = 0;
        double gap = 0.0;
        std::string at;
        for (const RunoutRow& a : t.rows) {
            if (!a.ok || a.name.size() != 2 || a.name[1] != 's') continue;
            const RunoutRow *d = nullptr, *c = nullptr;
            for (const RunoutRow& o : t.rows) {
                if (!o.ok || o.name.size() != 2 || o.name[0] != a.name[0]) continue;
                if (o.name[1] == 'd') d = &o;
                if (o.name[1] == 'c') c = &o;
            }
            if (!d || !c) continue;
            ++trios;
            const RunoutRow* all[3] = { &a, d, c };
            for (int i = 1; i < 3; ++i) {
                for (size_t k = 0; k < a.freq.size(); ++k) {
                    const double g = std::fabs(all[0]->freq[k] - all[i]->freq[k]);
                    if (g > gap) { gap = g; at = all[0]->name + " vs " + all[i]->name; }
                }
                const double g = std::fabs(all[0]->ev_oop - all[i]->ev_oop);
                if (g > gap) { gap = g; at = all[0]->name + " vs " + all[i]->name; }
            }
        }
        truth("compared all three unseen suits", trios == 13,
              "compared " + std::to_string(trios) + " ranks, expected 13");
        truth("a group of six still reads back exactly", gap < 1e-9,
              "worst gap " + std::to_string(gap) + " at " + at);

        // Rotating a two-card runout by a three-cycle must land on the same
        // solved spot: (Ks,Qd), (Kd,Qc) and (Kc,Qs) are one position. This is a
        // real consistency property, but note what it cannot see. The three
        // permutations involved form a cyclic subgroup, which is closed under
        // inversion, so reading every member back through the inverse instead
        // of the permutation itself just relabels which is which -- and they
        // all have to agree anyway. Catching a wrong direction needs a ground
        // truth, which is the check after this one.
        const int rctx = T.find_ctx("R|R/X/X|R/X/X");
        if (!truth("finds the river context", rctx >= 0)) return;
        const int rroot = T.ctx[static_cast<size_t>(rctx)].tree.root;
        const Deal& D = S.deal();

        const char* ranks[4] = { "K", "Q", "T", "7" };
        const char* second[4] = { "Q", "J", "8", "5" };
        const char* cyc[3][2] = { { "s", "d" }, { "d", "c" }, { "c", "s" } };
        int rotations = 0;
        double rgap = 0.0;
        std::string rat;
        for (int k = 0; k < 4; ++k) {
            double ref = 0.0, ref_f = 0.0;
            bool have = false;
            for (int i = 0; i < 3; ++i) {
                const int c1 = parse_card(std::string(ranks[k]) + cyc[i][0]);
                const int c2 = parse_card(std::string(second[k]) + cyc[i][1]);
                if (c1 < 0 || c2 < 0) continue;
                if (D.slot_of[c1] < 0 || D.slot_of[c2] < 0) continue;
                std::vector<int> slots;
                slots.push_back(D.slot_of[c1]);
                slots.push_back(D.slot_of[c2]);
                long long inst = 0;
                int perm = D.identity();
                if (!D.locate(slots, inst, perm)) continue;
                const NodeStats N = gather(*S.solver(), rctx, rroot, inst, perm);
                if (!N.ok || N.A < 1) continue;
                const double ev = N.node_ev(), fr = N.node_freq(0);
                if (!have) { ref = ev; ref_f = fr; have = true; ++rotations; continue; }
                ++rotations;
                const double g = std::max(std::fabs(ev - ref), std::fabs(fr - ref_f));
                if (g > rgap) { rgap = g; rat = std::string(ranks[k]) + cyc[i][0]; }
            }
        }
        truth("rotated two-card runouts were readable", rotations == 12,
              "read " + std::to_string(rotations) + " of 12");
        truth("a three-cycle reads back exactly", rgap < 1e-9,
              "worst gap " + std::to_string(rgap) + " at " + rat);
        permuted_read_ground_truth(S);
    }

    // The ground truth. With suits collapsed off, every runout is stored in its
    // own right and read with the identity, so there is no permutation to get
    // wrong. Solve the same spot both ways at a low iteration count and read the
    // same river node: the numbers have to match. This is the only check here
    // that can see a read-back applied in the wrong direction, because it is the
    // only one comparing against something that never permuted at all.
    void permuted_read_ground_truth(Session& S) {
        std::string e;
        const char* pairs[3][2] = { { "Ks", "Qd" }, { "Td", "8c" }, { "7c", "5s" } };
        double ev_on[3] = { 0, 0, 0 }, fr_on[3] = { 0, 0, 0 };
        double ev_off[3] = { 0, 0, 0 }, fr_off[3] = { 0, 0, 0 };
        int got = 0;

        for (int pass = 0; pass < 2; ++pass) {
            if (!truth("ground-truth spot builds", spot(S, "Ah9h4h", e), e)) return;
            cfg::ISO = (pass == 0);
            if (!truth("ground-truth rebuild", S.rebuild(e), e)) { cfg::ISO = true; return; }
            S.solve(40, 0);
            const GameTree& T = S.tree();
            const int rctx = T.find_ctx("R|R/X/X|R/X/X");
            if (rctx < 0) { cfg::ISO = true; return; }
            const int rroot = T.ctx[static_cast<size_t>(rctx)].tree.root;
            const Deal& D = S.deal();
            for (int k = 0; k < 3; ++k) {
                const int c1 = parse_card(pairs[k][0]), c2 = parse_card(pairs[k][1]);
                if (c1 < 0 || c2 < 0 || D.slot_of[c1] < 0 || D.slot_of[c2] < 0) continue;
                std::vector<int> slots;
                slots.push_back(D.slot_of[c1]);
                slots.push_back(D.slot_of[c2]);
                long long inst = 0;
                int perm = D.identity();
                if (!D.locate(slots, inst, perm)) continue;
                const NodeStats N = gather(*S.solver(), rctx, rroot, inst, perm);
                if (!N.ok || N.A < 1) continue;
                if (pass == 0) { ev_on[k] = N.node_ev(); fr_on[k] = N.node_freq(0); ++got; }
                else           { ev_off[k] = N.node_ev(); fr_off[k] = N.node_freq(0); ++got; }
            }
        }
        cfg::ISO = true;

        truth("read the same runouts both ways", got == 6,
              "read " + std::to_string(got) + " of 6");
        double worst = 0.0, worst_ev = 0.0;
        std::string at;
        for (int k = 0; k < 3; ++k) {
            const double g = std::fabs(fr_on[k] - fr_off[k]);
            if (g > worst) { worst = g; at = std::string(pairs[k][0]) + pairs[k][1]; }
            worst_ev = std::max(worst_ev, std::fabs(ev_on[k] - ev_off[k]));
        }
        truth("a permuted read has the same EV", worst_ev < 1e-6,
              "worst EV gap " + fmt_sci(worst_ev));
        truth("a permuted read matches the unpermuted one", worst < 1e-6,
              "worst gap " + fmt_sci(worst) + " at " + at);
    }

    // Every runout must resolve to a stored one plus a permutation that really
    // carries it there. This is pure combinatorics -- no solving -- so it can
    // cover all 1176 flop runouts in milliseconds, and it reaches the part of
    // the isomorphism the solve-based check never touches: a turn start has one
    // chance level, so second-level orbits and the two-card branch of locate()
    // were going entirely unexercised.
    void runout_addressing(Session& S) {
        std::string e;
        if (!truth("addressing spot builds", spot(S, "Ah9h4h", e), e)) return;
        const Deal& D = S.deal();

        int checked = 0, bad_locate = 0, bad_perm = 0, off_group = 0;
        std::vector<char> seen(static_cast<size_t>(D.inst[2]), 0);
        for (int t = 0; t < D.deckN(); ++t) {
            for (int r = 0; r < D.deckN(); ++r) {
                if (r == t) continue;
                std::vector<int> slots;
                slots.push_back(t);
                slots.push_back(r);
                long long inst = 0;
                int perm = D.identity();
                if (!D.locate(slots, inst, perm) || inst < 0 || inst >= D.inst[2]) {
                    ++bad_locate;
                    continue;
                }
                ++checked;
                seen[static_cast<size_t>(inst)] = 1;

                // The permutation has to fix the base board, or it is not a
                // symmetry of this spot at all.
                bool in_group = false;
                for (int g : D.base_group) if (g == perm) { in_group = true; break; }
                if (!in_group) ++off_group;

                // And it has to carry the stored runout's two cards onto the
                // two that were actually asked for.
                const int ro = D.runout_by_inst[static_cast<size_t>(inst)];
                const int st = D.runout_slot[static_cast<size_t>(ro) * 2 + 0];
                const int sr = D.runout_slot[static_cast<size_t>(ro) * 2 + 1];
                const int got_t = D.perm_card[perm][D.deck[static_cast<size_t>(st)]];
                const int got_r = D.perm_card[perm][D.deck[static_cast<size_t>(sr)]];
                const int want_t = D.deck[static_cast<size_t>(t)];
                const int want_r = D.deck[static_cast<size_t>(r)];
                const bool ok = (got_t == want_t && got_r == want_r) ||
                                (got_t == want_r && got_r == want_t);
                if (!ok) ++bad_perm;
            }
        }
        long long unused = 0;
        for (char c : seen) if (!c) ++unused;

        same("every ordered flop runout addressed", checked, 49 * 48);
        truth("locate() never fails", bad_locate == 0,
              std::to_string(bad_locate) + " runouts could not be located");
        truth("the permutation fixes the board", off_group == 0,
              std::to_string(off_group) + " permutations were not in the stabiliser");
        truth("the permutation lands on the right cards", bad_perm == 0,
              std::to_string(bad_perm) + " runouts mapped somewhere else");
        truth("no stored runout is unreachable", unused == 0,
              std::to_string(unused) + " stored instances were never addressed");
    }

    // "What the player does at this decision point across the board" is an
    // average over runouts, and with suits collapsed the stored instances stand
    // for different numbers of them. Two properties pin it: the weights have to
    // account for every runout exactly once, and the average must not care
    // whether suits were collapsed at all.
    void block_average_weighting(Session& S) {
        std::string e;
        if (!truth("block-average spot builds", spot(S, "Ah9h4h", e), e)) return;
        const Deal& D = S.deal();

        long long total = 0;
        for (long long b = 0; b < D.inst[2]; ++b) total += D.orbit_weight(b, 2);
        same("orbit weights cover every runout", total, 49 * 48);

        long long turns = 0;
        for (long long b = 0; b < D.inst[1]; ++b) turns += D.orbit_weight(b, 1);
        same("and every turn card", turns, 49);

        // The turn context after a checked-through flop: several instances, and
        // orbits of genuinely different sizes on a monotone board.
        const int ci = S.tree().find_ctx("R|R/X/X");
        if (!truth("finds the turn context", ci >= 0)) return;
        const int nid = S.tree().ctx[static_cast<size_t>(ci)].tree.root;
        const int A = S.tree().ctx[static_cast<size_t>(ci)].tree.nodes[
                          static_cast<size_t>(nid)].num_actions;

        std::vector<double> on, off;
        for (int pass = 0; pass < 2; ++pass) {
            if (!truth("block-average rebuild", spot(S, "Ah9h4h", e), e)) return;
            cfg::ISO = (pass == 0);
            if (!truth("block-average iso rebuild", S.rebuild(e), e)) { cfg::ISO = true; return; }
            S.solve(40, 0);
            std::vector<double> st(static_cast<size_t>(A) * S.solver()->num_hands(), 0.0);
            S.solver()->avg_strategy_block(ci, nid, st.data());
            (pass == 0 ? on : off) = st;
        }
        cfg::ISO = true;

        double worst = 0.0;
        int at = -1;
        for (size_t i = 0; i < on.size() && i < off.size(); ++i) {
            const double g = std::fabs(on[i] - off[i]);
            if (g > worst) { worst = g; at = static_cast<int>(i); }
        }
        truth("the across-the-board average ignores collapsing", worst < 1e-6,
              "worst gap " + fmt_sci(worst) + " at entry " + std::to_string(at));
    }

    // The range parser is the front door: everything downstream is only as
    // right as what came through it, and a spec that quietly means the wrong
    // thing produces a perfectly convergent solve of the wrong spot. Nothing
    // was pinning any of it. Counts and total weight on a fixed board, so a
    // change in what a notation means has to be deliberate.
    // Whatever the solver prints, it has to accept back. That sounds like a
    // truism and it was not: the sizing refusal added for '33' also refused '5',
    // which is exactly what the formatter wrote for a 500% overbet -- so a
    // config with one saved cleanly and then would not load. The property is
    // cheap to state and catches that whole class.
    void what_it_prints_it_takes(Session& S) {
        std::string e;

        // Sizings, across the range where the formatter has a choice to make.
        const double vals[] = { 0.25, 0.33, 0.5, 0.6, 1.0, 1.25, 2.0, 3.0,
                                4.99, 5.0, 5.5, 10.0, 25.0 };
        int bad = 0;
        for (double v : vals) {
            std::vector<double> one(1, v);
            const std::string printed = fmt_sizings(one);
            std::vector<Sizing> back;
            std::string why;
            if (!parse_sizings(printed, back, why) || back.size() != 1 ||
                std::fabs(back[0].v - v) > 1e-9 || back[0].xbet) {
                ++bad;
                truth("a bet of " + fmt_num(v) + " prints as something it takes back",
                      false, "printed '" + printed + "', " +
                             (why.empty() ? "read back wrong" : why));
            }
        }
        same("every bet size survives being printed and read", bad, 0);

        // Raises, which carry the multiplier flag through as well.
        // Las subidas, que ademas llevan el multiplicador y el minimo. `min` se
        // escribe con nombre, asi que tiene que volver a leerse como el minimo
        // y no como el 1x que es por dentro.
        std::vector<Sizing> rs;
        if (truth("raises parse", parse_raises("2x,3x,min", rs, e), e)) {
            const std::string printed = fmt_sizings(rs);
            std::vector<Sizing> back;
            if (truth("and print as something that parses", parse_raises(printed, back, e), e)) {
                same("with the same count", static_cast<long long>(back.size()),
                     static_cast<long long>(rs.size()));
                int off = 0;
                for (size_t i = 0; i < back.size() && i < rs.size(); ++i)
                    if (std::fabs(back[i].v - rs[i].v) > 1e-12 ||
                        back[i].xbet != rs[i].xbet ||
                        back[i].minraise != rs[i].minraise) ++off;
                same("and the same sizes, multiplier and minimum included", off, 0);
            }
        }

        // The board, printed with spaces and read back.
        std::vector<int> b;
        if (truth("a board parses", parse_board("Ah9h4hKd2s", b, e), e)) {
            std::vector<int> again;
            if (truth("its printed form parses", parse_board(board_str(b), again, e), e))
                truth("into the same five cards", again == b, board_str(b));
        }

        // The rake, which the console prints as a percentage.
        double f = 0.0;
        truth("the rake prints as a percentage and reads back",
              parse_fraction("5%", f) && std::fabs(f - 0.05) < 1e-12,
              "got " + fmt_num(f));
        truth("and the plain fraction still works",
              parse_fraction("0.05", f) && std::fabs(f - 0.05) < 1e-12,
              "got " + fmt_num(f));

        // And the whole config, which is the thing this actually protects: a
        // spot with an overbet in it, written out and read back.
        if (!truth("overbet spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        std::vector<Sizing> big;
        if (!truth("500% parses", parse_sizings("500%", big, e), e)) return;
        if (!truth("and can be set", S.set_sizings(true, ST_FLOP, big, e), e)) return;
        const std::string cfg = S.config_text();
        Session fresh;
        truth("a config holding an overbet loads again",
              fresh.load_config_text(cfg, e), e);

        // An empty list is a printed form like any other, and it is the one
        // that carries meaning now: no sizings, no action. It cannot print as
        // nothing, because an empty field would disappear inside a config line
        // and swallow the field after it.
        const std::vector<Sizing> nothing;
        const std::vector<double> nothing_b;
        truth("no raises print as a word, not a blank", fmt_sizings(nothing) == "none",
              "printed '" + fmt_sizings(nothing) + "'");
        truth("and so do no bets", fmt_sizings(nothing_b) == "none",
              "printed '" + fmt_sizings(nothing_b) + "'");
        std::vector<Sizing> back;
        truth("which reads back as no sizings",
              parse_sizings(fmt_sizings(nothing), back, e) && back.empty(), e);
    }

    // The cap is gone, but the configs that were saved with one are not, and a
    // saved config that no longer loads is a file the user cannot get back.
    // `cap 1` meant a bet and no raise; that is now an empty raise list, and it
    // has to keep meaning that rather than turning into an unlimited raise war.
    void old_configs_still_open(Session& S) {
        std::string e;
        if (!truth("a spot to write over", spot(S, "Ah9h4h", e), e)) return;

        // Written the way the solver used to write it: a raise sizing sitting
        // in the file next to a cap that made it a no-op.
        const std::string old =
            "board Ah9h4h\n"
            "oop 22+,A2s+\n"
            "ip 22+,A2s+\n"
            "pot 20\nstack 100\n"
            "street flop bets 0.6 raises 3x cap 1 allin 0 donk 0 thresh 0.15 merge 0.1\n"
            "street turn bets 0.66 raises 3x cap 2 allin 0 donk 0 thresh 0.15 merge 0.1\n"
            "street river bets 0.75 raises 3x cap 1 allin 0 donk 0 thresh 0.15 merge 0.1\n";
        Session old_s;
        if (!truth("a config written before the cap was removed still loads",
                   old_s.load_config_text(old, e), e)) return;

        // Flop said cap 1: a bet and nothing after it.
        expect_actions("its cap of 1 is still no raise", old_s.tree(), "R", "R/B12", "F|C");
        // Turn said cap 2, so the raises it listed were real and stay real --
        // and a line with no player token means both of them, which is what it
        // meant when there was only one list.
        truth("its cap of 2 keeps the raises it listed",
              !old_s.tc().raises[ST_TURN][0].empty() &&
              !old_s.tc().raises[ST_TURN][1].empty(), "the turn lost its raises");
        truth("and a line without a player reaches both of them",
              old_s.tc().bets[ST_TURN][0] == old_s.tc().bets[ST_TURN][1],
              "the two players ended up with different bets");

        // `donk 1` meant OOP could lead with its ordinary bet sizes. That is a
        // list now, so it translates to exactly those sizes rather than being
        // dropped as an unknown field.
        const std::string donked =
            "board Ah9h4h\noop AA\nip KK\npot 20\nstack 100\n"
            "street flop bets 0.6 raises none allin 0 donk 0 merge 0.1\n"
            "street turn bets 0.66 raises none allin 0 donk 1 merge 0.1\n"
            "street river bets 0.75 raises none allin 0 donk 0 merge 0.1\n";
        Session donk_s;
        if (!truth("a config with the old donk switch loads",
                   donk_s.load_config_text(donked, e), e)) return;
        truth("donk 1 becomes OOP's own bet sizes",
              donk_s.tc().donks[ST_TURN] == donk_s.tc().bets[ST_TURN][0],
              "the turn donk list is not OOP's bet list");
        truth("and donk 0 stays empty",
              donk_s.tc().donks[ST_RIVER].empty(), "the river gained a donk list");

        // The merge went the way the all-in threshold went: pinned, and out of
        // every interface. A file asking for another value does not get it, and
        // is told -- while a file asking for the pinned one says nothing. Both
        // halves, because a warning that never fires and a warning that always
        // fires are wrong in exactly the same way.
        const std::string merged =
            "board Ah9h4h\noop AA\nip KK\npot 20\nstack 100\n"
            "street flop bets 0.6 raises none allin 0 merge 0.4\n";
        Session merged_s;
        if (!truth("a config with a hand-tuned merge loads",
                   merged_s.load_config_text(merged, e), e)) return;
        truth("and says the merge is pinned",
              merged_s.load_note().find("merge") != std::string::npos,
              merged_s.load_note().empty() ? "said nothing" : merged_s.load_note());
        close_to("while the tree is built with the pinned one",
                 cfg::MERGE_PCT, 0.10, 1e-12);

        const std::string same_merge =
            "board Ah9h4h\noop AA\nip KK\npot 20\nstack 100\n"
            "street flop bets 0.6 raises none allin 0 merge 0.1\n";
        Session same_s;
        if (!truth("a config at the pinned merge loads",
                   same_s.load_config_text(same_merge, e), e)) return;
        truth("with nothing to report about it", same_s.load_note().empty(),
              "said '" + same_s.load_note() + "'");

        // And what it writes now has neither field in it.
        const std::string fresh_text = old_s.config_text();
        truth("what it writes back has no cap in it",
              fresh_text.find(" cap ") == std::string::npos,
              "still writes a cap");
        truth("nor a threshold",
              fresh_text.find(" thresh ") == std::string::npos,
              "still writes a threshold");
        Session again;
        truth("and that loads too", again.load_config_text(fresh_text, e), e);

        // The threshold is a harder case than the cap, because it cannot be
        // translated: it is one pinned number now, so a file asking for another
        // one does not get it. Loading has to say so out loud -- a value read
        // and quietly dropped is the whole family of bug this project keeps
        // finding.
        const std::string tuned =
            "board Ah9h4h\noop AA\nip KK\npot 20\nstack 100\n"
            "street flop bets 0.6 raises none allin 0 donk 0 thresh 0.4 merge 0.1\n";
        Session tuned_s;
        if (!truth("a config with a hand-tuned threshold still loads",
                   tuned_s.load_config_text(tuned, e), e)) return;
        truth("and says the old threshold does not convert",
              tuned_s.load_note().find("all-in") != std::string::npos,
              tuned_s.load_note().empty() ? "said nothing" : tuned_s.load_note());

        // ...while a config written since says nothing, because nothing was
        // reinterpreted. Both halves matter: a warning that never fires and a
        // warning that always fires are wrong in exactly the same way.
        const std::string plain =
            "board Ah9h4h\noop AA\nip KK\npot 20\nstack 100\nallinpct 0.5\n"
            "street flop bets 0.6 raises none allin 0 donk 0 merge 0.1\n";
        Session plain_s;
        if (!truth("a config in the new units loads", plain_s.load_config_text(plain, e), e)) return;
        truth("with nothing to report", plain_s.load_note().empty(),
              "said '" + plain_s.load_note() + "'");
        close_to("and the threshold it asked for", cfg::ALLIN_THRESH, 0.5, 1e-12);
        cfg::ALLIN_THRESH = 0.67;
    }

    // The rule itself, stated the way the reference states it, checked at the edge:
    // a bet committing MORE than the threshold of the STARTING effective stack
    // is an all-in instead. The old rule measured the pot, which is a different
    // number on every street; this one is fixed for the hand.
    void the_allin_threshold_is_the_stack(Session& S) {
        std::string e;
        if (!truth("threshold spot builds", spot(S, "Ah9h4h", e), e)) return;
        cfg::MERGE_PCT = 0.0;
        if (!truth("no merging", S.rebuild(e), e)) return;

        // Pot 20, stack 100. A bet of 3 pots is 60 -- under two thirds of the
        // stack, so it stands on its own.
        std::vector<Sizing> z;
        if (!truth("300% parses", parse_sizings("300%", z, e), e)) return;
        if (!truth("300% applies", S.set_sizings(true, ST_FLOP, z, e), e)) return;
        if (!truth("threshold 0.67", S.set_allin_thresh(0.67, e), e)) return;
        expect_actions("a bet of 60 is under two thirds of 100", S.tree(), "R", "R", "X|B60");

        // Move the line under it and the same bet becomes the all-in.
        if (!truth("threshold 0.5", S.set_allin_thresh(0.5, e), e)) return;
        expect_actions("under half, the same bet is the all-in", S.tree(), "R", "R", "X|B100");

        // Exactly on the line is not over it: 50 of a 100 stack at 0.5 stands.
        if (!truth("50% parses", parse_sizings("250%", z, e), e)) return;
        if (!truth("50% applies", S.set_sizings(true, ST_FLOP, z, e), e)) return;
        expect_actions("exactly on the line still stands", S.tree(), "R", "R", "X|B50");

        // And it is the STARTING stack, not the pot: doubling the pot doubles
        // the bet in chips but must not change where the line falls, because
        // the line does not know about the pot.
        if (!truth("threshold 0.67 again", S.set_allin_thresh(0.67, e), e)) return;
        if (!truth("a bet of 60 chips", parse_sizings("300%", z, e) &&
                   S.set_sizings(true, ST_FLOP, z, e), e)) return;
        expect_actions("60 of a 100 stack stands", S.tree(), "R", "R", "X|B60");
        if (!truth("pot 40", S.set_pot(40.0, e), e)) return;
        if (!truth("and a bet of 1.5 pots is the same 60", parse_sizings("150%", z, e) &&
                   S.set_sizings(true, ST_FLOP, z, e), e)) return;
        expect_actions("the same 60 chips, a different pot, the same answer",
                       S.tree(), "R", "R", "X|B60");

        // 1 turns it off: a bet leaving a single chip behind is allowed to be
        // its own branch.
        if (!truth("pot back", S.set_pot(20.0, e), e)) return;
        if (!truth("threshold off", S.set_allin_thresh(1.0, e), e)) return;
        if (!truth("a bet of 99", parse_sizings("495%", z, e) &&
                   S.set_sizings(true, ST_FLOP, z, e), e)) return;
        expect_actions("with the rule off, 99 is not 100", S.tree(), "R", "R", "X|B99");

        // The gap a mutation found, and the reason it is worth writing these:
        // every case above is on the flop, where nothing has gone in yet, so a
        // rule that forgot the earlier streets passed all of them. It is the
        // commitment for the HAND, not for the round.
        //
        // Flop bet of 30 called leaves 70 behind in an 80 pot. A half-pot turn
        // bet is 40, and 30 + 40 = 70 is past two thirds of the starting 100 --
        // so it is the all-in, not a bet of 40. Forget the 30 and it is not.
        if (!truth("threshold 0.67 for the turn", S.set_allin_thresh(0.67, e), e)) return;
        std::vector<Sizing> fb, tb;
        if (!truth("a flop bet of 30", parse_sizings("150%", fb, e) &&
                   S.set_sizings(true, ST_FLOP, fb, e), e)) return;
        if (!truth("a half-pot turn bet", parse_sizings("50", tb, e) &&
                   S.set_sizings(true, ST_TURN, tb, e), e)) return;
        expect_actions("what went in on the flop counts on the turn",
                       S.tree(), "R|R/B30/C", "R", "X|B70");

        // ...and it is not simply snapping everything: 30 + 4 is nowhere near
        // the line, and that bet stands as itself.
        if (!truth("a tiny turn bet", parse_sizings("5", tb, e) &&
                   S.set_sizings(true, ST_TURN, tb, e), e)) return;
        expect_actions("while a small one still stands on its own",
                       S.tree(), "R|R/B30/C", "R", "X|B4");

        // The near side of the line, which the two cases above do not pin: a
        // pot-sized flop bet of 20 called, then half of the 60 pot is 30, and
        // 20 + 30 = 50 is under the 67. It stands -- and a rule that counted
        // the flop money twice would make it the all-in of 80 instead.
        if (!truth("a pot-sized flop bet", parse_sizings("pot", fb, e) &&
                   S.set_sizings(true, ST_FLOP, fb, e), e)) return;
        if (!truth("half the turn pot", parse_sizings("50", tb, e) &&
                   S.set_sizings(true, ST_TURN, tb, e), e)) return;
        expect_actions("counted once, not twice", S.tree(), "R|R/B20/C", "R", "X|B30");

        truth("threshold back", S.set_allin_thresh(0.67, e), e);
    }

    // Without a cap the depth belongs to the sizings, and almost every sizing
    // converges: each raise is at least as big as the one it answers, so the
    // stack runs out. The exception is a bet small enough that the min-raise
    // creeps, which used to be the cap's real job. It has to be refused with a
    // sentence, not by recursing until the machine gives out.
    // Fichas enteras, en TODO el arbol y no solo donde uno mira.
    //
    // En una mesa no se apuesta media ficha. la referencia trunca -- escribes 62.5 y
    // construye 62 -- y mientras nosotros no lo haciamos los dos programas
    // resolvian juegos distintos sin decirlo: al 25% del bote nueve de las 37
    // lineas llevaban un 62.5 y la comparacion entera no valia. Y para quien lo
    // usa era peor todavia, porque la estrategia era de una partida que no se
    // puede jugar.
    //
    // Se comprueban las dos mitades, porque una sola engana:
    //   - que ninguna cantidad del arbol tenga decimales, con un tamano que SI
    //     trunca (0.33 de 100 son 33, pero 0.33 del bote de despues no cae
    //     redondo),
    //   - y que con tamanos que no truncan el resultado sea el MISMO de antes,
    //     hasta el ultimo digito. Sin esta segunda, cualquier cambio que moviera
    //     los numeros pasaria por "es el truncado".
    void bets_are_whole_chips(Session& S) {
        std::string e;
        if (!truth("whole-chip spot builds", spot(S, "Ah9h4h", e), e)) return;
        std::vector<Sizing> third;
        if (!truth("a third of the pot parses", parse_sizings("33", third, e), e)) return;
        for (int st = 0; st < 3; ++st)
            if (!truth("a third is set", S.set_sizings(true, st, third, e), e)) return;

        int decimales = 0;
        double primera = 0.0;
        const GameTree& T = S.tree();
        for (size_t c = 0; c < T.ctx.size(); ++c) {
            const BetTree& bt = T.ctx[c].tree;
            for (size_t a = 0; a < bt.actions.size(); ++a) {
                const double x = bt.actions[a].to_amount;
                if (std::fabs(x - std::floor(x)) > 1e-9) {
                    if (!decimales) primera = x;
                    ++decimales;
                }
            }
        }
        truth("no amount in the tree has a fraction of a chip", decimales == 0,
              std::to_string(decimales) + " la tienen, la primera " + std::to_string(primera));

        // Y se REDONDEA al mas cercano, no se trunca.
        //
        // Con los tamanos de la suite -- 0,6 y 0,75 sobre un bote de 20 -- las
        // dos cosas dan lo mismo, asi que todo lo de arriba pasaba igual
        // truncando. Hizo falta un bote y un tamano que las separen, que es
        // como salio: el 66% de un bote de 116 son 76,56, la referencia construye
        // 77 y nosotros construiamos 76. Una ficha, y los dos programas
        // resolviendo juegos distintos con el mismo aspecto.
        //
        // Ojo con la evidencia que hay por ahi de lo contrario: si a la referencia le
        // pasas la cantidad ya hecha con `add_line`, esa SI la trunca (87.78
        // construye 87). Son dos caminos distintos y me costo una noche.
        {
            Session R;
            std::string e2;
            if (truth("rounding spot builds", spot(R, "Ah9h4h", e2), e2)) {
                // Bote 116: el 66% son 76,56, que separa truncar de redondear.
                // Y stack de 220 con el umbral apagado: con el stack de 100 que
                // trae el spot, una apuesta de 77 pasa del 67% y se convierte
                // en all-in, con lo que la prueba medía otra cosa (daba 100).
                cfg::POT0 = 116.0;
                cfg::STACK = 220.0;
                cfg::ALLIN_THRESH = 1.0;
                std::vector<Sizing> sz;
                if (truth("66% parses", parse_sizings("66", sz, e2), e2) &&
                    truth("no raises", R.set_sizings(false, ST_FLOP,
                                                     std::vector<Sizing>(), e2), e2) &&
                    truth("the size is set", R.set_sizings(true, ST_FLOP, sz, e2), e2)) {
                    const BetTree& bt = R.tree().ctx[0].tree;
                    const Node& rn = bt.nodes[static_cast<size_t>(bt.root)];
                    double apuesta = -1.0;
                    for (int a = 0; a < rn.num_actions; ++a)
                        if (bt.act(rn, a).kind == AK_BET) apuesta = bt.act(rn, a).to_amount;
                    close_to("66% of 116 rounds to 77, it does not truncate to 76",
                             apuesta, 77.0, 1e-9);
                }
                cfg::POT0 = 20.0;
                cfg::STACK = 100.0;
                cfg::ALLIN_THRESH = 0.67;
            }
        }
    }

    void a_raise_chain_that_never_ends(Session& S) {
        std::string e;
        if (!truth("chain spot builds", spot(S, "Ah9h4h", e), e)) return;
        if (!truth("no all-in snap", S.set_allin_thresh(1.0, e), e)) return;
        cfg::MERGE_PCT = 0.0;
        if (!truth("no merging", S.rebuild(e), e)) return;

        // Por debajo de una ficha ya no hay apuesta que valga. Con bote de 20,
        // una milesima del bote son 0,02 fichas: eso no se puede poner en una
        // mesa, y truncado a fichas enteras no se puede poner aqui tampoco. Lo
        // que importa es que lo diga en vez de dejarla caer sin mas.
        // El tamano se arma a mano y no se teclea: una milesima del bote es un
        // 0,1% y el lector ya no deja escribir eso -- con razon, porque quien lo
        // escribe se ha equivocado de escala. Lo que se prueba aqui no es el
        // lector sino el constructor del arbol, y ese tiene que seguir sabiendo
        // decir que no aunque el tamano le llegue por otro sitio.
        std::vector<Sizing> crumb;
        { Sizing z; z.v = 0.001; z.xbet = false; crumb.push_back(z); }
        std::string toosmall;
        truth("but a bet under one chip is refused",
              !S.set_sizings(true, ST_FLOP, crumb, toosmall), "it was accepted");
        truth("and says how small it was",
              toosmall.find("less than one chip") != std::string::npos,
              "said '" + toosmall + "'");
        truth("and says the smallest this pot allows, in per cent",
              toosmall.find("5%") != std::string::npos,
              "said '" + toosmall + "'");

        // Y con la mas pequena que SI cabe -- una ficha justa de un bote de 20
        // -- la apuesta es legal, pero como subida la cadena sube de ficha en
        // ficha y no llega al stack en 24 niveles.
        std::vector<Sizing> tiny;
        if (!truth("one chip of the pot parses", parse_sizings("5", tiny, e), e)) return;
        if (!truth("as a bet it is fine", S.set_sizings(true, ST_FLOP, tiny, e), e)) return;

        std::string why;
        const bool refused = !S.set_sizings(false, ST_FLOP, tiny, why);
        if (!truth("a raise chain that does not converge is refused", refused,
                   "it built the tree instead")) return;
        truth("and says what is wrong with it",
              why.find("do not stop") != std::string::npos, "said '" + why + "'");
        truth("and the spot it refused is still standing",
              S.tc().raises[ST_FLOP][0].empty(), "the refusal left the raises behind");
    }

    // Bet sizes, and the board, are the other two things typed by hand. Same
    // rule as the ranges: what people write should work, and what is almost
    // certainly a mistake should be refused rather than silently obeyed.
    void sizing_and_board_parsing(Session& S) {
        std::string e;

        struct Ok { const char* spec; int count; double first; bool xbet; };
        const Ok good[] = {
            { "60",             1, 0.60, false },   // como se escribe ahora
            { "60%",            1, 0.60, false },   // y con la unidad puesta
            { "50",             1, 0.50, false },
            { "300",            1, 3.00, false },   // tres botes, sin ambiguedad
            { "pot",            1, 1.00, false },   // como se dice
            { "P",              1, 1.00, false },
            { "33,66,125",      3, 0.33, false },
            { "33%,66%,125%",   3, 0.33, false },
            { "500%",           1, 5.00, false },
            { "12.5",           1, 0.125, false },  // decimal grande: legitimo
            { "2x",             1, 2.00, true  },
            { "2x,3x",          2, 2.00, true  },
            { " 60 , 100 ",     2, 0.60, false },
            // La notacion de la referencia. Son DOS tamanos, no dos y medio: un 2%
            // -- que la apuesta minima sube -- y un 5x. Confundirlos costo dias.
            { "2,5x",           2, 0.02, false },
        };
        for (const Ok& c : good) {
            std::vector<Sizing> z;
            const std::string tag = std::string("sizing '") + c.spec + "'";
            if (!truth(tag, parse_sizings(c.spec, z, e), e)) continue;
            same(tag + ": how many", static_cast<long long>(z.size()), c.count);
            if (!z.empty()) {
                close_to(tag + ": the first one", z[0].v, c.first, 1e-12);
                truth(tag + ": multiplier or fraction", z[0].xbet == c.xbet);
            }
        }

        struct No { const char* spec; const char* saying; };
        const No bad[] = {
            // El que importa ahora es el contrario que antes. Un decimal
            // pequeno solo se escribe creyendo que se sigue en la escala vieja,
            // donde "0.66" eran dos tercios; leerlo como un 0,66% y callar seria
            // construir un arbol que nadie pidio.
            { "0.66",      "por ciento" },
            { "0.75",      "por ciento" },
            { "2.5",       "por ciento" },
            { "0.6,0.3",   "por ciento" },
            { "50000",     "botes" },
            { "0",         "positive" },
            { "-50",       "positive" },
            { "half",      "bad sizing" },
            { "1/2",       "bad sizing" },
        };
        for (const No& c : bad) {
            std::vector<Sizing> z;
            std::string why;
            const std::string tag = std::string("refuses sizing '") + c.spec + "'";
            // Zero and negatives are caught when the tree is built, not when the
            // text is read, so both gates count as a refusal.
            const bool parsed = parse_sizings(c.spec, z, why);
            bool refused = !parsed;
            if (parsed) refused = !S.set_sizings(true, ST_FLOP, z, why);
            if (!truth(tag, refused, "it was accepted")) continue;
            truth(tag + ", and says why", why.find(c.saying) != std::string::npos,
                  "said '" + why + "'");
        }

        // And a ten on the board, written the way people write it.
        std::vector<int> b;
        if (truth("board '10h9h4h' parses", parse_board("10h9h4h", b, e), e)) {
            same("which is three cards", static_cast<long long>(b.size()), 3);
            truth("and the first is the ten of hearts", !b.empty() && card_str(b[0]) == "Th",
                  b.empty() ? "nothing" : card_str(b[0]));
        }
        truth("board 'Ah9h' is refused",  !parse_board("Ah9h", b, e) || b.size() != 4);
        truth("board 'AhAh4h' is refused", !parse_board("AhAh4h", b, e));
    }

    // What the parser must refuse, and say why. A range it accepts wrongly is
    // worse than one it rejects: nothing downstream can tell.
    void range_refusals(Session& S) {
        std::string e;
        if (!truth("refusal spot builds", spot(S, "Ah9h4hKd2s", e), e)) return;
        struct Bad { const char* spec; const char* saying; };
        const Bad bad[] = {
            // Someone means half and types fifty. Taken at face value that hand
            // becomes fifty times likelier than the rest of the range and the
            // solve is quietly wrong, so it is refused rather than guessed at.
            { "AA:50",       "above 1" },
            { "AA:2",        "above 1" },
            { "QQ+:100",     "above 1" },
            { "AA:-0.5",     "negative" },
            { "AA:150%",     "bad weight" },
            { "[100]AA",     "unrecognised" },
            { "AXs",         "unrecognised" },
            { "AKs-AQo",     "suitedness" },
            // AA-AKs trips the suitedness test first, so this is the one that
            // reaches the pairs-against-non-pairs branch.
            { "AA-AK",       "pairs" },
            { "AA-AKs",      "suitedness" },
        };
        for (const Bad& b : bad) {
            std::vector<double> w;
            std::string why;
            const std::string tag = std::string("refuses '") + b.spec + "'";
            if (!truth(tag, !parse_range(b.spec, S.deal(), w, why), "it was accepted")) continue;
            truth(tag + ", and says why",
                  why.find(b.saying) != std::string::npos,
                  "said '" + why + "'");
        }
    }

    void range_parsing(Session& S) {
        std::string e;
        if (!truth("parser spot builds", spot(S, "Ah9h4hKd2s", e), e)) return;
        const Deal& D = S.deal();

        struct Case { const char* spec; int combos; double weight; };
        const Case cases[] = {
            // Pairs. The board holds Ah, Kd and 9h, so those are down to three.
            { "AA",        3,  3.0 },
            { "22",        3,  3.0 },
            { "QQ+",      12, 12.0 },
            // Suited and offsuit, with the board eating into both. AKs is down
            // to spades and clubs; AKo loses the two remaining suited ones.
            { "AKs",       2,  2.0 },
            { "AKo",       7,  7.0 },
            { "T9s",       3,  3.0 },
            // The "+" forms. KJo+ is KJo and KQo -- the kicker climbs, the top
            // card does not, so AKo is not in it.
            { "A2s+",     34, 34.0 },
            { "KJo+",     18, 18.0 },
            // One exact combo, and one the board has taken.
            { "AsKs",      1,  1.0 },
            { "AhKh",      0,  0.0 },
            // Weights.
            { "A5s:0.5",   3,  1.5 },
            { "QQ:0.25,JJ:0.75", 12, 6.0 },
            // Everything.
            { "random", 1081, 1081.0 },
            // A ten written the way people and exports write it. Applied
            // after the weight is split off, so ":10" is still a weight.
            { "109s",      3,  3.0 },
            { "A10s",      3,  3.0 },
            { "A10s:0.5",  3,  1.5 },
            // Semicolons separate as well as commas.
            { "AA;KK",     6,  6.0 },
            { "AA;KK,QQ", 12, 12.0 },
        };

        for (const Case& c : cases) {
            std::vector<double> w;
            std::string pe;
            if (!truth(std::string("parses '") + c.spec + "'",
                       parse_range(c.spec, D, w, pe), pe)) continue;
            int live = 0;
            double tot = 0.0;
            for (double x : w) { if (x > 0.0) ++live; tot += x; }
            if (record_) {
                std::printf("  rec   %-46s %d combos, weight %.4f\n", c.spec, live, tot);
                continue;
            }
            same(std::string("'") + c.spec + "' combo count", live, c.combos);
            close_to(std::string("'") + c.spec + "' total weight", tot, c.weight, 1e-9);
        }

        // El formato de rangos de la referencia, que es el que circula por foros,
        // videos y librerias de rangos: manos puras sin peso, parciales con
        // ':0.5', separadas por comas. Un rango copiado de ahi tiene que
        // entrar tal cual, sin que nadie lo reescriba a mano.
        {
            const std::string pegado =
                "AA,KK,QQ,AKs,A8o:0.5,KQ,KJ,K8o:0.5,K7o:0.5,QJ,Q7o:0.5,"
                "Q5s:0.5,Q4s:0.5,Q3s:0.5,Q2s:0.5,J9o:0.5,J8o:0.5,J7o:0.5,"
                "J4s:0.5,J3s:0.5,J2s:0.5";
            std::vector<double> w;
            std::string pe;
            if (truth("a range copied out of another solver", parse_range(pegado, D, w, pe), pe)) {
                int    kq = 0, a8o_n = 0;
                double a8o = 0.0;
                for (int h = 0; h < D.num(); ++h) {
                    const std::string n = class_name(D.combos[static_cast<size_t>(h)].cls);
                    if ((n == "KQs" || n == "KQo") && w[static_cast<size_t>(h)] > 0.0) ++kq;
                    if (n == "A8o") { a8o += w[static_cast<size_t>(h)]; ++a8o_n; }
                }
                // KQ sin sufijo son las dos, suited y offsuit. El tablero tiene
                // Kd, asi que de las 16 quedan 12.
                same("KQ without a suffix is both", kq, 12);
                // Y el 0.5 va a cada combo de A8o, no a uno de ellos.
                close_to("A8o:0.5 is half of every A8o", a8o, 0.5 * a8o_n, 1e-9);
                same("A8o has combos to weigh at all", a8o_n, 9);
            }
        }

        // Things that must be refused rather than silently misread.
        const char* bad[] = { "", "ZZ", "AKx", "QQ:", "QQ:abc", "QQ:-1", "AsAs" };
        for (const char* b : bad) {
            std::vector<double> w;
            std::string pe;
            truth(std::string("rejects '") + b + "'", !parse_range(b, D, w, pe),
                  "it was accepted");
        }
    }

    // The deferred discount has two paths. Every block is read every iteration,
    // so the one-step path is the only one that ever runs -- which means the
    // multi-step path, the one that would matter if that assumption ever broke,
    // is never exercised by solving. Tie them together directly: closing a gap
    // in one jump has to equal taking the single steps one at a time.
    void discount_paths_agree(Session& S) {
        std::string e;
        if (!truth("discount spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(200, 0);
        const DCFRSolver& sol = *S.solver();

        double worst = 0.0;
        int gap_at = 0;
        for (int from = 1; from < 190; from += 17) {
            for (int span = 1; span <= 9; ++span) {
                const int to = from + span;
                for (int sign = 0; sign < 2; ++sign) {
                    double step = 1.0;
                    for (int k = from + 1; k <= to; ++k) {
                        double dp, dn;
                        DCFRSolver::discount_factors(k, dp, dn);
                        step *= (sign == 0) ? dp : dn;
                    }
                    const double jump = sol.catchup_factor(from, to, sign == 0);
                    const double rel = (step > 1e-300) ? std::fabs(jump - step) / step : 0.0;
                    if (rel > worst) { worst = rel; gap_at = span; }
                }
            }
        }
        truth("one jump equals the single steps", worst < 1e-9,
              "worst relative gap " + std::to_string(worst) +
              " over a span of " + std::to_string(gap_at));
    }

    // Rake, which is the one thing here that deliberately breaks zero sum.
    void rake_model(Session& S) {
        std::string e;
        if (!truth("rake spot builds", spot(S, "Ah9h4hKd2s", e), e)) return;

        // The structural half first: an uncalled bet goes back to the bettor,
        // so the house takes its cut of the MATCHED pot, not of the pile on
        // the table. Getting this wrong overcharges every fold in the tree.
        if (!truth("rake 5% applies", S.set_rake(0.05, 0.0, e), e)) return;
        const GameTree& T = S.tree();
        const int fold_after_bet = T.find_node(0, "R/B15/F");
        if (truth("finds a fold facing a bet", fold_after_bet >= 0)) {
            const Node& fn = T.ctx[0].tree.nodes[static_cast<size_t>(fold_after_bet)];
            close_to("the uncalled bet is not raked", fn.rake, 0.05 * 20.0, 1e-12);
            truth("and the pot on the table is bigger than that", fn.pot > 20.0 + 1e-9,
                  "pot was " + std::to_string(fn.pot));
        }

        S.solve(300, 0);
        const double oop_rake = S.solver()->root_ev(0);
        const double ip_rake  = S.solver()->root_ev(1);
        const double taken    = cfg::POT0 - oop_rake - ip_rake;
        truth("rake takes chips out of the game", taken > 1e-6,
              "the house took " + std::to_string(taken));
        // Every matched pot is at least the starting pot, so an uncapped 5%
        // cannot take less than 5% of it.
        truth("uncapped 5% takes at least 5% of the starting pot", taken > 1.0 - 1e-9,
              "took " + std::to_string(taken));

        // With a cap below 5% of the smallest possible pot, the cap binds on
        // every single terminal, so the house takes exactly the cap. That makes
        // this an exact assertion rather than a bound.
        if (!truth("rake capped at 0.5", S.set_rake(0.05, 0.5, e), e)) return;
        S.solve(300, 0);
        const double capped = cfg::POT0 - S.solver()->root_ev(0) - S.solver()->root_ev(1);
        close_to("a cap that always binds takes exactly the cap", capped, 0.5, 1e-9);

        // And it costs both players, not just one.
        if (!truth("rake back off", S.set_rake(0.0, 0.0, e), e)) return;
        S.solve(300, 0);
        truth("rake costs OOP", oop_rake < S.solver()->root_ev(0) - 1e-6);
        truth("rake costs IP",  ip_rake  < S.solver()->root_ev(1) - 1e-6);
    }

    // Nodelocking, past "does it bite". A lock is a promise about four things:
    // the mix is exactly what was asked for, it holds on every runout and not
    // just the one being looked at, it survives a tree rebuild, and it is
    // dropped honestly when its node stops existing.
    // Nodelocking is not "type a strategy in", it is "look at the one that came
    // out and move it". These three are what that needs.
    void nodelock_from_the_strategy(Session& S) {
        std::string e;
        if (!truth("lock-edit spot builds", spot(S, "Ah9h4h", e), e)) return;
        if (!truth("OOP has a few classes", S.set_range(0, "AA,KK,QQ,AJo,T9s", e), e)) return;
        if (!truth("IP has a range", S.set_range(1, "JJ,TT,AQo", e), e)) return;
        S.solve(100, 0);

        const BetTree& bt = S.tree().ctx[0].tree;
        const int root = bt.root;
        const Node& rn = bt.nodes[static_cast<size_t>(root)];
        const int ib = bt.action_index(rn, AK_BET);
        const int ix = bt.action_index(rn, AK_CHECK);
        if (!truth("the root offers check and bet", ib >= 0 && ix >= 0)) return;

        // 1. Two groups, two different locks, one node. This is the whole job --
        //    the calls come off one group while another bets more -- and the
        //    second lock used to delete the first without saying anything.
        std::vector<std::pair<std::string, double>> allbet, allchk;
        allbet.push_back(std::make_pair(std::string("B"), 1.0));
        allchk.push_back(std::make_pair(std::string("X"), 1.0));
        int m1 = 0, m2 = 0;
        if (!truth("AA is locked to betting", S.add_lock(0, root, "AA", allbet, m1, e), e)) return;
        if (!truth("KK is locked to checking", S.add_lock(0, root, "KK", allchk, m2, e), e)) return;
        same("and both locks are still there", static_cast<long long>(S.locks().size()), 2);
        // ...while re-locking the SAME hands replaces rather than piles up.
        if (!truth("AA is locked again", S.add_lock(0, root, "AA", allchk, m1, e), e)) return;
        same("which replaces, not adds", static_cast<long long>(S.locks().size()), 2);
        S.clear_all_locks();

        // 2. Freezing what came out, per combo. AJo does not play as one hand:
        //    the combo holding the heart blocks IP differently from the rest,
        //    and one averaged number for the class would erase that.
        DCFRSolver& sol = *S.solver();
        const int nh = sol.num_hands();
        std::vector<double> before(static_cast<size_t>(rn.num_actions) * nh);
        sol.avg_strategy_block(0, root, before.data());

        int got = 0;
        if (!truth("freeze AJo at what it was doing",
                   S.add_lock_from_current(0, root, "AJo", got, e), e)) return;
        same("one lock per combo", static_cast<long long>(S.locks().size()), got);
        truth("and AJo is nine combos on this board", got == 9,
              "froze " + std::to_string(got));

        // Every lock is that combo's own number, not the class average.
        double worst = 0.0;
        bool split = false;
        double seen = -1.0;
        for (const LockSpec& L : S.locks()) {
            std::vector<int> hs;
            if (!S.resolve_hands_public(L.hand_spec, rn.player, hs, e) || hs.size() != 1) {
                truth("each lock names one combo", false, L.hand_spec); return;
            }
            double want = before[static_cast<size_t>(ib) * nh + hs[0]], have = 0.0;
            for (const auto& pr : L.mix) if (!pr.first.empty() && pr.first[0] == 'B') have = pr.second;
            worst = std::max(worst, std::fabs(have - want));
            if (seen < 0.0) seen = want;
            else if (std::fabs(want - seen) > 1e-6) split = true;
        }
        truth("each combo keeps its own frequency", worst < 1e-9,
              "off by " + std::to_string(worst));
        truth("and they were not all the same to begin with", split,
              "every AJo combo played identically, so this proves nothing");

        // 3. Moving it. Thirty points onto the bet, taken from the rest in
        //    proportion -- which on a two-action node is all of it.
        //
        //    Clearing the locks turns suit collapsing back on, which rebuilds
        //    the solver, so this starts from a fresh solve and reads the numbers
        //    again rather than trusting the ones from before.
        S.clear_all_locks();
        S.solve(100, 0);
        DCFRSolver& sol2 = *S.solver();
        std::vector<double> base(static_cast<size_t>(rn.num_actions) * nh);
        sol2.avg_strategy_block(0, root, base.data());
        if (!truth("bet thirty points more",
                   S.add_lock_nudged(0, root, "AJo", AK_BET, 0.30, got, e), e)) return;
        double bad = 0.0, offsum = 0.0;
        for (const LockSpec& L : S.locks()) {
            std::vector<int> hs;
            if (!S.resolve_hands_public(L.hand_spec, rn.player, hs, e) || hs.size() != 1) continue;
            const double was = base[static_cast<size_t>(ib) * nh + hs[0]];
            double now = 0.0, tot = 0.0;
            for (const auto& pr : L.mix) { tot += pr.second; if (!pr.first.empty() && pr.first[0] == 'B') now = pr.second; }
            bad = std::max(bad, std::fabs(now - std::min(1.0, was + 0.30)));
            offsum = std::max(offsum, std::fabs(tot - 1.0));
        }
        truth("every combo moved by exactly thirty points", bad < 1e-9,
              "worst off by " + std::to_string(bad));
        truth("and the mix still sums to one", offsum < 1e-9,
              "worst sum off by " + std::to_string(offsum));

        // And it holds through a re-solve, which is the point of doing it.
        S.solve(150, 0);
        DCFRSolver& sol3 = *S.solver();
        std::vector<double> after(static_cast<size_t>(rn.num_actions) * nh);
        sol3.avg_strategy_block(0, root, after.data());
        double held = 0.0;
        int locked = 0;
        for (int h = 0; h < nh; ++h) {
            if (!sol3.is_hand_locked(0, root, 0, h)) continue;
            ++locked;
            held = std::max(held, std::fabs(after[static_cast<size_t>(ib) * nh + h] -
                                            std::min(1.0, base[static_cast<size_t>(ib) * nh + h] + 0.30)));
        }
        same("nine combos are locked after the re-solve", locked, 9);
        truth("and they still play what they were moved to", held < 1e-9,
              "drifted by " + std::to_string(held));

        // 4. The edits compose off ONE solve, which is how the job is actually
        //    done: take the calls off this group, make that one bet more, then
        //    solve once. Two things have to hold for that. Moving a group that
        //    is already locked works from the LOCK, not from a solve that no
        //    longer matches it -- twenty more than what you froze. And a group
        //    that is not locked yet can still be read, which means setting the
        //    first lock must not throw the strategy away.
        S.clear_all_locks();
        S.solve(100, 0);
        DCFRSolver& sol4 = *S.solver();
        std::vector<double> b4(static_cast<size_t>(rn.num_actions) * nh);
        sol4.avg_strategy_block(0, root, b4.data());

        if (!truth("freeze AJo", S.add_lock_from_current(0, root, "AJo", got, e), e)) return;
        if (!truth("then move the frozen AJo",
                   S.add_lock_nudged(0, root, "AJo", AK_BET, 0.20, got, e), e)) return;
        if (!truth("and a group nobody has touched yet, off the same solve",
                   S.add_lock_nudged(0, root, "T9s", AK_BET, -1.0, got, e), e)) return;

        double moved_off = 0.0, air_bets = 0.0;
        for (const LockSpec& L : S.locks()) {
            std::vector<int> hs;
            if (!S.resolve_hands_public(L.hand_spec, rn.player, hs, e) || hs.size() != 1) continue;
            double bet = 0.0;
            for (const auto& pr : L.mix) if (!pr.first.empty() && pr.first[0] == 'B') bet = pr.second;
            const double was = b4[static_cast<size_t>(ib) * nh + hs[0]];
            // T9s was told to stop betting; AJo was frozen and then moved 20.
            if (std::fabs(bet) < 1e-12 && was > 1e-9) { air_bets = std::max(air_bets, bet); continue; }
            moved_off = std::max(moved_off, std::fabs(bet - std::min(1.0, was + 0.20)));
        }
        truth("moving a frozen group counts from the freeze", moved_off < 1e-9,
              "worst off by " + std::to_string(moved_off));
        truth("and the group told to stop betting stopped", air_bets < 1e-12);
        S.clear_all_locks();
    }

    // The page is one big string compiled into the binary, so nothing checks it
    // and nothing ever will at build time. And a single bad line does not
    // degrade it -- it kills the whole script, so every button stops working at
    // once and the page just sits there. That is exactly what it looks like
    // when there is no bug at all, which is why it shipped twice: a duplicate
    // `const SUITS`, and a stray `async` left in front of a comment.
    //
    // These are the two cheapest properties that would have caught both.
    // Every `state.X` the script reads has to be a field the server sends.
    //
    // The other two page checks catch a script that does not run at all. This
    // one catches a script that runs fine and shows nothing: a renamed or
    // mistyped field is not an error in JavaScript, it is `undefined`, and
    // `undefined` lands in the page looking like an answer. Checked against the
    // state the server actually builds, not against a list written by hand,
    // because a list written by hand is the thing that goes stale.
    // Un nombre de clase, una cosa.
    //
    // `.pc` es la carta de la baraja: lleva aspect-ratio y fondo claro, o sea
    // que cualquier elemento que la reciba sale como una carta blanca, mida lo
    // que mida. Y durante un tiempo nombraba TAMBIEN un porcentaje del dialogo
    // de nodelock, asi que el nombre estaba libre en la cabeza de uno y ocupado
    // en la hoja de estilos. Al poner un span de porcentaje con ese nombre
    // dentro de un boton de accion, el boton salio con un cuadro blanco del
    // tamano de una carta dentro. No fallo ninguna logica: fallo un nombre.
    //
    // La regla que lo evita es que la clase de carta la ponga UNA sola funcion,
    // cardFace(), y que nadie la escriba a mano en el HTML. Eso si se puede
    // comprobar leyendo la pagina.
    // Plegar el montaje tiene que DAR el sitio, no solo esconderlo.
    //
    // La primera version escondia la columna de la izquierda y ya esta. La
    // rejilla tiene un tope de ancho -- el que cabia con el montaje al lado --
    // y ese tope seguia puesto, asi que al plegar se quedaba igual de pequena y
    // el hueco vacio. Escondio algo y no gano nada, que es el unico resultado
    // que no vale la pena.
    //
    // Lo que se comprueba es la relacion entre los dos topes, no su valor: que
    // el de plegado sea mayor. Cambiar los numeros no rompe nada; quitar el
    // segundo, si.
    // La tira de locks no puede crecer con el numero de locks.
    //
    // Listaba todos. Un nodelock de verdad son sesenta manos, asi que la tira se
    // volvia un muro de "R R JcJd -> Check=1" de media pantalla que nadie lee:
    // sesenta lineas iguales no informan de nada que no diga el numero 60.
    //
    // Dos cosas la sujetan y las dos se pueden leer en la pagina: el detalle
    // sale cortado por arriba, y su caja tiene alto tope con scroll propio. Sin
    // cualquiera de las dos, vuelve el muro en cuanto alguien bloquee un rango
    // entero -- que es el caso normal, no el raro.
    // Congelar no puede borrar lo que ya has pintado.
    //
    // El boton se llama "Congelar lo que no hayas tocado", asi que promete
    // respetarlo. Lo cumple porque lee lkFreq -- lo pintado si lo hay, y la
    // frecuencia del solve si no -- y no lkOrig, que devuelve siempre la del
    // solve. Cambiar una por otra no da error ni se ve al probar: solo hace que
    // pintar cuatro manos y congelar el resto pierda las cuatro, en silencio,
    // justo al fijar.
    // Nada tira una solucion sin preguntar.
    //
    // Reconstruir el arbol borra los arrepentimientos acumulados, y eso no se
    // deshace ni se guarda en ningun sitio. Paso de verdad: un arbol de 2,7 GB
    // resuelto, un clic en Construir para mirar otra cosa, y de vuelta a 7
    // iteraciones con una explotabilidad del 211% del bote -- ensenando en todos
    // los paneles unas tablas con toda la pinta de ser una solucion.
    //
    // Cuatro sitios de la pagina reconstruyen: el boton de construir, el board,
    // el texto del rango y la rejilla. Los cuatro tienen que pasar por el mismo
    // aviso, y el aviso tiene que mirar si hay algo que perder -- sin
    // iteraciones hechas no debe molestar a nadie.
    void nothing_throws_a_solve_away_in_silence() {
        const std::string P = WEBUI_PAGE;
        if (!truth("there is one place that asks",
                   P.find("function tirariaElSolve(") != std::string::npos)) return;

        // Mira lo que hay que perder, y no otra cosa.
        const size_t i = P.find("function tirariaElSolve(");
        const size_t fin = P.find("\n}", i);
        const std::string cuerpo = P.substr(i, fin - i);
        truth("and it asks only when there is something to lose",
              cuerpo.find("state.done>0") != std::string::npos, cuerpo);
        truth("and the answer is what it returns",
              cuerpo.find("return confirm(") != std::string::npos,
              "el confirm esta pero su respuesta no decide nada");

        // Hasta donde llega esto, dicho para que nadie se confie.
        //
        // La pagina es una cadena de C++ y aqui no hay ningun motor de
        // JavaScript, asi que esto LEE el texto, no ejecuta nada. Con eso caza
        // lo que se rompe por descuido: quitarle el aviso a un camino, que el
        // aviso deje de mirar si hay algo que perder, o neutralizarlo con un
        // `true ||` delante.
        //
        // Lo que NO puede cazar es un `return true;` puesto ANTES del confirm:
        // el texto sigue estando y la funcion ya no bloquea. Probado, y pasa
        // entera. Anadir un cuarto patron para ese caso concreto seria fingir
        // que se comprueba el comportamiento cuando solo se comprueba la letra.
        // Quien mueva esta funcion, que la pruebe en el navegador.

        // Y los cuatro caminos pasan por ahi.
        const char* caminos[] = { "applyTree", "pushBoard", "pushRange", "applyRangeText" };
        for (const char* c : caminos) {
            const size_t f = P.find(std::string("function ") + c + "(");
            if (!truth(std::string(c) + " exists", f != std::string::npos)) continue;
            const size_t f2 = P.find("\n}", f);
            truth(std::string(c) + " asks before rebuilding",
                  P.substr(f, f2 - f).find("tirariaElSolve(") != std::string::npos,
                  std::string(c) + " reconstruye sin preguntar");
        }
    }

    void freezing_keeps_what_you_painted() {
        const std::string P = WEBUI_PAGE;
        const size_t i = P.find("function lockFreezeAll(){");
        if (!truth("there is a freeze button behind something", i != std::string::npos)) return;
        const size_t fin = P.find("\n}", i);
        const std::string cuerpo = P.substr(i, fin - i);
        truth("freezing reads what is on screen", cuerpo.find("lkFreq(") != std::string::npos,
              cuerpo);
        truth("and not what the solve produced", cuerpo.find("lkOrig(") == std::string::npos,
              "usa lkOrig: congelar borraria lo pintado");
        truth("and the button says as much",
              P.find("Congelar lo que no hayas tocado") != std::string::npos);
    }

    void the_lock_strip_cannot_become_a_wall() {
        const std::string P = WEBUI_PAGE;
        // Que el tope EXISTA no basta: una mutacion dejo el `Math.min` puesto y
        // corto por L.length, o sea que la comprobacion pasaba con la lista
        // entera. Hay que mirar el sitio donde se usa, que es el que decide.
        truth("the lock detail is capped", P.find("Math.min(L.length,20)") != std::string::npos,
              "renderLocks ya no calcula un tope");
        truth("and the cap is what cuts the list",
              P.find("L.slice(0,tope)") != std::string::npos,
              "el tope esta calculado pero la lista no se corta con el");
        const size_t d = P.find("  .lockdet{");
        if (!truth("the detail box has its own rules", d != std::string::npos)) return;
        const size_t fin = P.find('}', d);
        const std::string regla = P.substr(d, fin - d);
        truth("with a height cap", regla.find("max-height") != std::string::npos, regla);
        truth("and its own scroll", regla.find("overflow-y:auto") != std::string::npos, regla);
    }

    void folding_the_setup_gives_the_room_away() {
        const std::string P = WEBUI_PAGE;

        struct Tope { const char* sel; double v; bool ok; };
        Tope base = { "\n  .grid.strat{max-width:", 0.0, false };
        Tope abierto = { "main.plegado .grid.strat{max-width:", 0.0, false };
        Tope* dos[2] = { &base, &abierto };
        for (Tope* t : dos) {
            const size_t i = P.find(t->sel);
            if (i == std::string::npos) continue;
            const size_t j = i + std::strlen(t->sel);
            std::string num;
            for (size_t k = j; k < P.size() && P[k] != 'p' && P[k] != '}'; ++k) num += P[k];
            t->ok = parse_double(trim(num), t->v);
        }
        if (!truth("the grid has a width cap", base.ok, "no se encuentra .grid.strat"))
            return;
        if (!truth("and a different one when the setup is folded", abierto.ok,
                   "plegar esconde el montaje y no le da el sitio a nadie"))
            return;
        truth("and folded is the wider of the two", abierto.v > base.v,
              std::to_string(abierto.v) + " no es mayor que " + std::to_string(base.v));

        // Y que la columna se esconda de verdad, que es la otra mitad.
        truth("folding hides the setup column",
              P.find("main.plegado .col-l{display:none}") != std::string::npos);
        truth("and there is something to fold it with",
              P.find("function toggleSetup()") != std::string::npos);
    }

    void a_class_name_means_one_thing() {
        const std::string P = WEBUI_PAGE;

        const std::string escrita = "class=" + std::string(1, '"') + "pc" +
                                    std::string(1, '"');
        truth("nobody writes the card class by hand",
              P.find(escrita) == std::string::npos,
              "alguien escribio " + escrita + " a mano: la clase de carta la "
              "pone cardFace(), y a cualquier otra cosa le pega un fondo de "
              "carta encima");

        // Y sigue existiendo: si se renombrara la clase sin tocar esto, la
        // comprobacion de arriba pasaria siempre y no vigilaria nada.
        truth("and the card class is still the one with the card look",
              P.find("  .pc{") != std::string::npos &&
              P.find("aspect-ratio", P.find("  .pc{")) != std::string::npos,
              "no se encuentra la regla global de .pc con su aspect-ratio");
        truth("and cardFace is the one that puts it on",
              P.find("cardFace(card,'pc'") != std::string::npos);
    }

    void the_page_reads_what_the_server_sends(Session& S) {
        std::string e;
        if (!truth("state-field spot builds", spot(S, "Ah9h4h", e), e)) return;
        // Solved, because some fields are only sent once there is an answer --
        // and the page only reads those inside `if(state.solved)`. Checking an
        // unsolved state would report them missing and be wrong about it.
        S.solve(20, 0);
        WebUI W(S, 0, false);
        const std::string j = W.state_for_check();

        std::set<std::string> sends;          // top-level keys only
        int depth = 0;
        for (size_t i = 0; i < j.size(); ++i) {
            const char c = j[i];
            if (c == '{' || c == '[') { ++depth; continue; }
            if (c == '}' || c == ']') { --depth; continue; }
            if (c != '"') continue;
            size_t e = i + 1;
            while (e < j.size() && !(j[e] == '"' && j[e - 1] != 0x5c)) ++e;
            if (e >= j.size()) break;
            if (depth == 1) {
                size_t k = e + 1;
                while (k < j.size() && j[k] == ' ') ++k;
                if (k < j.size() && j[k] == ':') sends.insert(j.substr(i + 1, e - i - 1));
            }
            i = e;
        }
        truth("the server sends a state worth checking", sends.size() > 20,
              std::to_string(sends.size()) + " top-level fields");

        const std::string P = WEBUI_PAGE;
        std::set<std::string> missing, reads;
        for (size_t i = 0; (i = P.find("state.", i)) != std::string::npos; ) {
            i += 6;
            // Not `state.x` where the dot was part of something longer.
            std::string name;
            size_t k = i;
            while (k < P.size() &&
                   (std::isalnum(static_cast<unsigned char>(P[k])) || P[k] == '_'))
                name += P[k++];
            if (name.empty()) continue;
            reads.insert(name);
            if (!sends.count(name)) missing.insert(name);
        }
        truth("the page reads a state worth checking", reads.size() > 20,
              std::to_string(reads.size()) + " fields read");
        truth("every field the page reads is one the server sends", missing.empty(),
              missing.empty() ? "" : ("not sent: " + *missing.begin() +
                                      (missing.size() > 1
                                       ? " (+" + std::to_string(missing.size() - 1) + ")"
                                       : "")));

        // Y el valor que el servidor manda cae en la rejilla que el campo acepta.
        //
        // Un <input type="number" min="1" step="50"> solo admite 1, 51, 101...
        // asi que un 300 que venga del servidor es un valor que su propio campo
        // rechaza: el navegador lo marca invalido y las flechitas saltan a 301.
        // No rompe nada -- el JavaScript lee .value igual -- y por eso pasa
        // desapercibido. Salio tres veces en una sola noche: el tope de
        // iteraciones, el limite de memoria y por poco la precision.
        //
        // Se emparejan los campos con el estado por la propia asignacion del
        // script, `getElementById('X').value=state.Y`, asi que no hay lista que
        // mantener.
        {
            struct Field { std::string id, from; double mn, step; };
            std::vector<Field> fields;
            for (size_t at = 0; (at = P.find("<input type=\"number\"", at)) != std::string::npos; ) {
                const size_t end = P.find('>', at);
                if (end == std::string::npos) break;
                const std::string tag = P.substr(at, end - at);
                at = end;
                auto attr = [&tag](const char* k, std::string& out) {
                    const size_t p = tag.find(std::string(k) + "=\"");
                    if (p == std::string::npos) return false;
                    const size_t a = p + std::strlen(k) + 2, b = tag.find('"', a);
                    if (b == std::string::npos) return false;
                    out = tag.substr(a, b - a);
                    return true;
                };
                std::string id, mn, st;
                if (!attr("id", id) || !attr("min", mn) || !attr("step", st)) continue;
                const std::string mark = "getElementById('" + id + "').value=state.";
                const size_t u = P.find(mark);
                if (u == std::string::npos) continue;
                const size_t v = P.find(';', u);
                std::string from = P.substr(u + mark.size(), v - u - mark.size());
                fields.push_back(Field{ id, from, std::atof(mn.c_str()), std::atof(st.c_str()) });
            }
            truth("hay campos numericos que emparejar con el estado", fields.size() >= 3,
                  std::to_string(fields.size()) + " emparejados");

            std::vector<std::string> off;
            for (const Field& f : fields) {
                if (f.step <= 0.0) continue;
                const std::string key = "\"" + f.from + "\":";
                const size_t p = j.find(key);
                if (p == std::string::npos) continue;
                const double v = std::atof(j.c_str() + p + key.size());
                const double k = (v - f.mn) / f.step;
                if (std::fabs(k - std::floor(k + 0.5)) > 1e-6)
                    off.push_back(f.id + " vale " + fmt_sci(v) + " con min " +
                                  fmt_sci(f.mn) + " y paso " + fmt_sci(f.step));
            }
            truth("y cada uno cae en la rejilla de su propio campo", off.empty(),
                  off.empty() ? "" : off[0]);
        }
    }

    // Sin stack detrás no hay nada que decidir.
    //
    // Con los dos jugadores all-in de calles anteriores, ninguno puede apostar:
    // la recursión les daba un "Check" solitario a cada uno y los encadenaba al
    // showdown. Nodos de decisión de una sola acción, instanciados una vez por
    // runout -- la mitad del árbol en un flop con all-in. Lo encontró comparar
    // el árbol entero contra el de la referencia, que va directo al reparto.
    //
    // Ojo con la propiedad: NO es "ningún nodo de decisión tiene una sola
    // acción". Un OOP al que se le ha suprimido el donk tiene exactamente una,
    // y ese nodo hace falta porque la calle sigue con IP. La propiedad es sobre
    // la ronda entera.
    void nothing_to_decide_with_nothing_behind(Session& S) {
        std::string e;
        if (!truth("all-in spot builds", spot(S, "Ks7h2c", e), e)) return;
        std::vector<Sizing> b;
        b.push_back(Sizing{ 0.75, false });
        for (int st = 0; st < 3; ++st) {
            if (!truth("sets a big sizing", S.set_sizings(true, st, b, e), e)) return;
            if (!truth("and the all-in", S.set_allin(st, true, e), e)) return;
        }
        if (!truth("rebuilds", S.rebuild(e), e)) return;

        int rounds_broke = 0, decisions_after = 0, all_in_rounds = 0;
        long long wasted = 0;
        for (const RoundCtx& rc : S.tree().ctx) {
            if (cfg::STACK - rc.spent > 1e-9) continue;
            ++all_in_rounds;
            int dec = 0;
            for (const Node& nd : rc.tree.nodes) if (nd.type == NT_DECISION) ++dec;
            if (dec) { ++rounds_broke; decisions_after += dec; wasted += rc.instances * dec; }
        }
        truth("the spot really reaches a street with nobody behind", all_in_rounds > 0,
              "no all-in round in this tree, so this check proves nothing");
        truth("and that street has no decisions at all", rounds_broke == 0,
              std::to_string(decisions_after) + " decision nodes in " +
              std::to_string(rounds_broke) + " rounds, " + std::to_string(wasted) +
              " instanced");
    }

    // El objetivo de precision para de verdad, y por el motivo que dice.

    //

    // Un ajuste que se pone, no protesta y no hace nada es peor que no tenerlo,

    // y este tenia justo esa forma: el paron vivia solo en el worker asincrono,

    // asi que en la consola y en --script no habria hecho nada.

    // Lo que se ensena nunca es peor que lo que pediste.
    //
    // La interfaz lo promete con esas palabras y durante un tiempo fue mentira,
    // no por el solver sino por la escala: el objetivo se comparaba contra la
    // MEDIA de las dos mejores respuestas -- la convencion estandar -- y el numero
    // que salia al lado era la SUMA. Pedias 1% del bote, paraba diciendo que lo
    // habia alcanzado, y te ensenaba 1,4627%. El doble exacto, sin decirlo en
    // ninguna parte.
    //
    // Se comprueban las dos mitades, porque arreglar una sola deja la puerta
    // abierta a que se vuelvan a separar:
    //
    //   la promesa   tras parar por precision, lo PUBLICADO cabe en el objetivo
    //   la escala    lo publicado es exactamente la mitad de la suma
    //
    // Y se lee del JSON que sale por /api/state, que es el que pinta la pantalla,
    // no del ayudante: comprobar expl_shown() contra expl_shown() no dice nada.
    void what_it_shows_is_never_worse_than_what_you_asked(Session& S) {
        std::string e;
        if (!truth("accuracy-scale spot builds", spot(S, "Ah9h4h", e), e)) return;

        // Un objetivo flojo a proposito, para que pare por precision y no por
        // iteraciones: lo que se mira es la escala, no lo fino que llega.
        const double objetivo = 3.0;
        S.set_acc_target(objetivo);
        S.set_acc_stop(true);
        S.solve(4000, 0);
        if (!truth("it stopped on the accuracy target", S.acc_reached(),
                   "no llego a pararse por precision")) return;

        const double suma = S.exploitability();
        truth("there is an exploitability to read", suma >= 0.0,
              std::to_string(suma));

        // Lo que sale por la API, que es lo que lee la pagina.
        WebUI W(S, 0, false);
        const std::string j = W.state_for_check();
        double publicado = -1.0;
        {
            const std::string clave = "\"explPct\":";
            const size_t i = j.find(clave);
            if (!truth("the state carries explPct", i != std::string::npos)) return;
            publicado = std::atof(j.c_str() + i + clave.size());
        }

        truth("and what it shows is not worse than what was asked",
              publicado <= objetivo + 1e-9,
              "pedido " + std::to_string(objetivo) + "%, ensenado " +
              std::to_string(publicado) + "%");

        // Y la escala, dicha con una cuenta: la media de las dos mejores
        // respuestas, que es como la da la referencia.
        const double esperado = cfg::POT0 > 0.0 ? 100.0 * (suma * 0.5) / cfg::POT0 : -1.0;
        close_to("and it is the average of the two best responses",
                 publicado, esperado, 1e-6);
        truth("which is not the sum",
              suma <= 1e-12 || std::fabs(publicado - 2.0 * esperado) > 1e-9,
              "la suma y la media salen iguales, asi que no separan nada");

        // Y el objetivo se compara contra ESE numero y no contra otro, probado en
        // el borde: con el objetivo puesto justo en lo que se ensena se da por
        // alcanzado, y con la mitad no.
        //
        // Hace falta el borde. Mirar solo donde paro el solve no vale: aflojar el
        // objetivo al doble no lo notaba nadie, porque este spot se pasa tanto de
        // largo que con el objetivo al doble paraba en el mismo sitio. La
        // comprobacion parecia buena y no lo era.
        if (publicado > 1e-6) {
            // El objetivo se compara contra ESE numero y no contra otro, medido
            // por encima y por debajo. Una tabla y no un solo punto, porque con
            // un solo punto no basta: mirar donde para el solve no caza nada
            // (este spot se pasa tanto de largo que con el objetivo al doble
            // paraba igual), y probar justo en la mitad tampoco, porque al doblar
            // la regla la mitad cae EXACTAMENTE en el borde y decide el redondeo.
            // Las dos versiones parecian buenas y no vigilaban nada.
            //
            // El 0,9 es el que separa: con la regla buena no se alcanza, y con el
            // objetivo al doble si, con un 10% de margen que el redondeo del JSON
            // no se come.
            static const double factores[] = { 0.5, 0.9, 1.001, 2.0 };
            bool mal = false;
            std::string donde;
            for (double f : factores) {
                S.set_acc_target(publicado * f);
                const bool debe = (f >= 1.0);
                if (S.accuracy_met() != debe) {
                    mal = true;
                    donde += " objetivo x" + std::to_string(f) + " dice " +
                             (S.accuracy_met() ? "alcanzado" : "no alcanzado");
                }
            }
            truth("and the target means what it shows, above it and below it",
                  !mal, "con lo ensenado en " + std::to_string(publicado) + "%:" + donde);
            S.set_acc_target(objetivo);
        }
    }

    void the_accuracy_target_stops_the_solve(Session& S) {

        std::string e;

        if (!truth("accuracy spot builds", spot(S, "Ah9h4h", e), e)) return;



        S.set_acc_target(1.0);            // 1% del bote, holgado a proposito

        S.set_acc_stop(true);

        S.solve(4000, 0);

        const long long done = S.solver()->iterations_done();

        truth("para antes de llegar al tope", done < 4000,

              std::to_string(done) + " de 4000");

        truth("y dice que fue la precision quien lo paro", S.acc_reached());

        const double pct = 100.0 * expl_shown(S.exploitability()) / cfg::POT0;

        truth("y de verdad quedo por debajo del objetivo", pct <= 1.0,

              "quedo en " + fmt_sci(pct) + "% del bote");



        // Y AHORA como llama la consola de verdad, que es otra cosa.
        //
        // La comprobacion de aqui arriba pasa `report_every = 0`, y con eso el
        // solve avanza a trozos de 64 y mira la precision a menudo. La consola
        // llama con `n/10`, y eso ataba el trozo a la cadencia de los avisos:
        // pidiendo `solve 20000` no se miraba la precision hasta la iteracion
        // 2000. Medido antes de arreglarlo: objetivo del 1% del bote, 79
        // segundos, 2000 iteraciones y final en el 0,003% -- trescientas veces
        // mas fino de lo que se pidio, y el tiempo pagado entero.
        //
        // O sea que esta comprobacion pasaba en verde mientras el unico camino
        // que usa alguien estaba roto. Por eso ahora se prueba ese.
        S.set_acc_target(1.0);
        S.set_acc_stop(true);
        S.solve(4000, 400);
        const long long done2 = S.solver()->iterations_done();
        // El umbral tiene que estar por DEBAJO de la cadencia de avisos, que es
        // lo unico que distingue las dos versiones: con el trozo atado a los
        // avisos para en la 400 clavada, y con el trozo suelto para mucho antes.
        // La primera vez puse 512 y la mutacion paso por debajo: 400 <= 512.
        // Una comprobacion que no caza su mutacion no vale.
        truth("y para igual de pronto como la llama la consola", done2 < 400,
              std::to_string(done2) + " de 4000 con report_every=400");
        truth("y tambien dice que fue la precision", S.acc_reached());

        // Y lo que se ENSENA al acabar son las iteraciones hechas, no las
        // pedidas. Decia las pedidas: "20000 iterations in 79.80s" cuando
        // habian corrido 2000.
        const std::string linea = solve_summary(2000, 20000, 79.8, 16, true);
        truth("el resumen dice las iteraciones que se hicieron",
              linea.rfind("  2000 iterations in", 0) == 0, linea);
        truth("y cuenta que paro por la precision",
              linea.find("stopped on reaching the accuracy") != std::string::npos,
              linea);
        truth("y de cuantas venia",
              linea.find("20000") != std::string::npos, linea);
        // Y los avisos de progreso, por el contador GLOBAL: con el local, cada
        // trozo de 64 sacaba su primera y su ultima linea, o sea dos avisos
        // cada 64 iteraciones cuando se pidio uno cada mil.
        truth("un aviso cada mil es cada mil", should_report(1000, 1000));
        truth("y no al empezar cada trozo", !should_report(1, 1000));
        truth("ni al acabarlo", !should_report(64, 1000));
        truth("y sin cadencia no hay avisos", !should_report(1000, 0));

        const std::string llena = solve_summary(4000, 4000, 1.5, 16, false);
        truth("y cuando corre las que se le piden no cuenta nada raro",
              llena.rfind("  4000 iterations in", 0) == 0 &&
              llena.find("precision pedida") == std::string::npos, llena);

        // Y apagado, corre exactamente las que se le piden: el tope vuelve a ser

        // el criterio, que es lo que espera un script.

        S.set_acc_stop(false);

        S.solve(300, 0);

        same("apagado, corre las que se le piden",

             static_cast<long long>(S.solver()->iterations_done()), 300);

        truth("y ya no dice que paro por precision", !S.acc_reached());

    }



    // Dos tamanos escritos como los escribe la gente.
    //
    // Solo valia la coma. Con un espacio, "0.6 0.3" no daba error: daba UN
    // tamano del 60% y el 30% desaparecia, porque parse_double se queda con lo
    // que entiende y calla. Pedir dos tamanos y recibir uno sin aviso es la peor
    // forma de fallar: el arbol sale mas barato de lo que crees y las
    // frecuencias que miras no son las del spot que querias.
    void two_sizes_written_as_people_write_them(Session& S) {
        (void)S;
        struct Caso { const char* txt; int cuantos; double a, b; };
        const Caso ok[] = {
            { "60,30",     2, 0.3,  0.6  },
            { "60 30",     2, 0.3,  0.6  },
            { "60  30",    2, 0.3,  0.6  },
            { "60;30",     2, 0.3,  0.6  },
            { " 60 , 30 ", 2, 0.3,  0.6  },
            { "60%,30%",   2, 0.3,  0.6  },
            { "60% 30%",   2, 0.3,  0.6  },
            { "pot 50",    2, 0.5,  1.0  },
            { "75",        1, 0.75, 0.75 },
        };
        for (const Caso& c : ok) {
            std::vector<Sizing> v;
            std::string e;
            const std::string tag = std::string("'") + c.txt + "' ";
            if (!truth(tag + "se acepta", parse_sizings(c.txt, v, e), e)) continue;
            if (!same(tag + "da los tamanos que pone",
                      static_cast<long long>(v.size()), c.cuantos)) continue;
            std::vector<double> got;
            for (const Sizing& z : v) got.push_back(z.v);
            std::sort(got.begin(), got.end());
            must_be(tag + "el menor", got.front(), c.a, 1e-9);
            must_be(tag + "el mayor", got.back(), c.b, 1e-9);
        }

        // Y lo que tiene que doler, duele. Una coma decimal a la espanola es
        // ambigua aqui, porque la coma ya separa la lista.
        const char* mal[] = { "60abc", "x", "60,,,x", "" };
        for (const char* t : mal) {
            std::vector<Sizing> v;
            std::string e;
            truth(std::string("'") + t + "' se rechaza con un motivo",
                  !parse_sizings(t, v, e) && !e.empty(),
                  e.empty() ? "sin mensaje" : e);
        }

        // La coma decimal a la espanola ya no se rescata, y esto lo vigila.
        //
        // Con los tamanos en por ciento nadie escribe la coma: dos tercios es
        // "66". Lo unico que llegaba al rescate eran numeros de la escala vieja
        // -- "0,6" queriendo decir 0.6 del bote -- y esos hay que rechazarlos,
        // no arreglarlos. Se rechazan con un motivo, que es lo que se pide.
        const char* comas[] = { "0,6", "0,5", "0,33 0,75" };
        for (const char* c : comas) {
            std::vector<Sizing> v;
            std::string e2;
            truth(std::string("'") + c + "' se rechaza, ya no se rescata",
                  !parse_sizings(c, v, e2) && !e2.empty(),
                  e2.empty() ? "se acepto sin mas" : e2);
        }
        // Y el caso que NO se puede adivinar, escrito aqui para que se vea: la
        // coma separa, asi que "12,5" son dos tamanos y no doce y medio. Si
        // alguna vez deja de ser asi, esto lo dira.
        {
            std::vector<Sizing> v;
            std::string e2;
            if (truth("'12,5' son dos tamanos, no doce y medio",
                      parse_sizings("12,5", v, e2), e2)) {
                same("y son dos", static_cast<long long>(v.size()), 2);
                if (v.size() == 2) must_be("el primero es 12%", v[0].v, 0.12, 1e-9);
            }
        }
        // Mezclar los dos estilos no se adivina: se rechaza.
        {
            std::vector<Sizing> v;
            std::string e2;
            truth("'0.33,0,75' mezcla estilos y se rechaza",
                  !parse_sizings("33,0,75", v, e2) && !e2.empty(), e2);
        }

    }
    // Un cliente de HTTP del tamano justo para esta prueba: abrir, escribir,
    // leer hasta que cierren. Nada de reintentos ni de troceado -- si algo de
    // eso hiciera falta seria porque el servidor va mal, que es justo lo que
    // se mide.
    static sock_t dial(int port) {
        sock_t c = socket(AF_INET, SOCK_STREAM, 0);
        if (c == SOCK_INVALID) return c;
        sockaddr_in a;
        std::memset(&a, 0, sizeof(a));
        a.sin_family = AF_INET;
        a.sin_port = htons(static_cast<unsigned short>(port));
        a.sin_addr.s_addr = htonl(0x7F000001);
        if (connect(c, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) {
            SOCK_CLOSE(c);
            return SOCK_INVALID;
        }
        return c;
    }

    static std::string fetch(int port, const std::string& path) {
        sock_t c = dial(port);
        if (c == SOCK_INVALID) return "";
        // CRLF y no salto a secas: el servidor busca el fin de cabeceras con
        // "\r\n\r\n" exacto, asi que con saltos pelados nunca lo encuentra,
        // se queda en recv esperando el resto, y el cliente se queda esperando
        // la respuesta. Esta prueba se colgo asi la primera vez que corrio.
        const std::string req = "GET " + path + " HTTP/1.1\r\nHost: x\r\n"
                                "Connection: close\r\n\r\n";
        if (send(c, req.data(), static_cast<int>(req.size()), 0) <= 0) {
            SOCK_CLOSE(c);
            return "";
        }
        std::string all;
        char buf[4096];
        for (;;) {
            const int n = recv(c, buf, sizeof(buf), 0);
            if (n <= 0) break;
            all.append(buf, static_cast<size_t>(n));
        }
        SOCK_CLOSE(c);
        return all;
    }

    // Se conecta, escribe MEDIA peticion y cuelga. Es lo que hace un navegador
    // cuando cambias de pagina o cierras la pestana con una consulta en vuelo.
    static void abort_midway(int port) {
        sock_t c = dial(port);
        if (c == SOCK_INVALID) return;
        const char* half = "GET /api/progress HTTP/1.1";
        send(c, half, static_cast<int>(std::strlen(half)), 0);
        SOCK_CLOSE(c);
    }

    // Se conecta, manda una peticion a medias y NO cuelga: se queda ahi.
    // Distinto de abortar -- al abortar el recv devuelve 0 y el hilo se va
    // limpio. Aqui el hilo se queda en recv, y sin plazo se quedaria para
    // siempre. Devuelve el socket para poder soltarlo al final.
    static sock_t stall(int port) {
        sock_t c = dial(port);
        if (c == SOCK_INVALID) return c;
        // Una cabecera bien formada pero sin la linea en blanco que las cierra.
        const std::string h = "GET /api/state HTTP/1.1\r\nHost: x\r\n";
        send(c, h.data(), static_cast<int>(h.size()), 0);
        return c;
    }

    // Pide algo grande y NO LEE la respuesta nunca.
    //
    // Es el cliente que rompe la unica promesa que el servidor se hace por
    // escrito: "el cuerpo se construye bajo el cerrojo y se manda despues de
    // soltarlo, porque tener un mutex del solver cogido durante la E/S del
    // socket pararia al worker todo lo que el cliente tarde en leer". Si esa
    // promesa se rompe, el buffer del socket se llena, send() se queda
    // esperando con el cerrojo en la mano, y todos los demas detras. Este es
    // el navegador que se minimiza, la pestana que el sistema congela, o el
    // portatil que cierras a mitad.
    static sock_t deaf_request(int port, const char* path) {
        sock_t c = dial(port);
        if (c == SOCK_INVALID) return c;
        // Buffer de recepcion diminuto: en loopback, entre el buffer de envio
        // del servidor y el de recepcion del cliente caben unos 128 KB, y una
        // respuesta que quepa ahi se manda sin bloquear aunque nadie lea. Con
        // el buffer pequeno, el send() del servidor se queda esperando de
        // verdad, que es la unica forma de que este cliente pruebe algo.
        const int tiny = 1024;
        setsockopt(c, SOL_SOCKET, SO_RCVBUF,
                   reinterpret_cast<const char*>(&tiny), sizeof(tiny));
        const std::string req = std::string("GET ") + path + " HTTP/1.1\r\n"
                                "Host: x\r\nConnection: close\r\n\r\n";
        send(c, req.data(), static_cast<int>(req.size()), 0);
        return c;                       // y aqui se queda, sin leer una letra
    }

    // El servidor de verdad, con sockets de verdad, contestando mientras
    // resuelve.
    //
    // LO QUE ESTA PRUEBA NO HACE, y conviene leerlo antes de fiarse de ella:
    // NO caza el bloqueo que dejo la interfaz muerta a mitad de un solve.
    // Se escribio para eso. Se rompio el arreglo a proposito tres veces --
    // quitando el cerrojo al construir el estado, lo mismo con ocho clientes a
    // la vez, y devolviendo el respond() dentro del cerrojo -- y las tres veces
    // esta prueba siguio en verde. Asi que para ESE fallo no vale, y dejarla
    // aqui creyendo que si valdria es peor que no tenerla.
    //
    // El error de razonamiento fue suponer que un cliente que pide y no lee
    // deja al servidor bloqueado enviando. Medido: la pagina son 103 KB y en
    // loopback caben unos 128 KB entre el buffer de envio y el de recepcion,
    // asi que la respuesta sale entera sin bloquear aunque nadie la lea.
    //
    // LO QUE SI CUBRE, que no es nada y por eso se queda:
    //   - que el servidor levante y coja puerto,
    //   - que aguante ocho clientes concurrentes mezclando consultas enteras
    //     con cortes a media peticion, sin dejar ni una sin contestar y con el
    //     solve todavia vivo detras,
    //   - que el stop se oiga,
    //   - que la pagina siga entera despues,
    //   - y que se apague cuando se le dice.
    //
    // Y de escribirla salio un fallo de verdad, aunque no el buscado: se colgo
    // ella misma mandando las cabeceras con saltos de linea pelados, y al
    // mirar por que aparecio que un cliente a medias se quedaba con un hilo
    // para siempre. De ahi el plazo de recepcion en webui.hpp.
    // Dos solvers no pueden quedarse escuchando en el mismo puerto.
    //
    // En Windows, SO_REUSEADDR deja que el segundo proceso se ate igual, y los
    // dos se quedan sirviendo: comprobado a mano, dos solvers en el 8790 con
    // las conexiones repartidas sin regla. Y pasa, porque la gente hace doble
    // clic otra vez cuando parece que no arranca. Entonces montas un spot, lo
    // resuelves, refrescas, y te contesta la OTRA instancia con los rangos
    // vacios -- que se lee como "el programa me ha borrado el trabajo".
    //
    // Esto no lee el codigo, lo hace: levanta un servidor, mira en que puerto
    // quedo, e intenta levantar otro ahi mismo. El segundo tiene que fallar.
    // El spot con el que arranca el programa tiene que cargar y estar listo.
    //
    // Va dentro del binario, asi que nadie lo prueba al usarlo: si se escribe
    // mal un rango o un tamano, el programa arranca con los rangos vacios
    // exactamente como antes y nadie se entera de que habia un spot ahi.
    //
    // Y main() no intenta recuperarse de que falle -- un arranque a medias seria
    // peor -- asi que la garantia tiene que estar aqui.
    // Los escalones de la pildora de convergencia, que son una decision y no
    // un detalle.
    //
    // La explotabilidad ya salia, pero como un numero mas en una linea densa
    // del panel de EV: un 211% del bote se leia igual que un 0,2%. Ahora va
    // arriba y con color, y donde estan los cortes es lo unico que hay que
    // pensar. Salen de lo que dijo quien usa esto: del 2 al 5 por ciento una
    // solucion ya sirve de guia, que es para lo que se usa.
    //
    // Esto lee el texto de la pagina y no ejecuta nada -- aqui no hay motor de
    // JavaScript -- asi que vigila los NUMEROS, que es lo que no debe cambiar
    // sin querer. Que la funcion se llame de verdad desde los dos sitios no se
    // puede comprobar asi; eso se mira en el navegador.
    // El nombre de la casilla se tiene que leer sobre TODOS los fondos.
    //
    // La rejilla tenia la letra en gris sobre fondos oscuros y sobre el rojo de
    // apostar no se leia nada. Ahora va en negro, al estilo de la referencia, con
    // fondos claros -- y eso convierte cada color de accion en algo que se puede
    // equivocar: basta oscurecer uno y el nombre desaparece otra vez.
    //
    // Esto no mira si el color "esta puesto": saca los hexadecimales de la hoja
    // de estilos y CALCULA el contraste, con la formula de WCAG. Un color nuevo
    // demasiado oscuro cae aqui aunque este perfectamente escrito.
    static double luminancia(const std::string& hex) {
        auto canal = [&](int i) {
            const std::string dos = hex.substr(static_cast<size_t>(i), 2);
            const double c = static_cast<double>(std::strtol(dos.c_str(), nullptr, 16)) / 255.0;
            return (c <= 0.03928) ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * canal(0) + 0.7152 * canal(2) + 0.0722 * canal(4);
    }
    static double contraste(const std::string& a, const std::string& b) {
        const double la = luminancia(a), lb = luminancia(b);
        return ((la > lb ? la : lb) + 0.05) / ((la < lb ? la : lb) + 0.05);
    }
    // El valor de una variable CSS, "--fold:#7fa8d4" -> "7fa8d4".
    static std::string color_de(const std::string& P, const std::string& nombre) {
        const size_t i = P.find(nombre + ":#");
        if (i == std::string::npos) return std::string();
        return P.substr(i + nombre.size() + 2, 6);
    }

    void every_grid_label_can_be_read(Session& S) {
        (void)S;
        const std::string P = WEBUI_PAGE;

        // La letra de la rejilla DE ESTRATEGIA, que es la que se pinta con los
        // colores de accion. La de rangos del montaje comparte la clase `.grid`
        // pero es oscura con las casillas en azul, y lleva su propia letra
        // clara: leer `.grid .cell` aqui media el color de la otra.
        const size_t g = P.find("  .grid.strat .cell, .grid.strat .cell.on{");
        if (!truth("the grid cell has a style", g != std::string::npos)) return;
        const size_t c = P.find("color:#", g);
        if (!truth("and the label has a colour", c != std::string::npos)) return;
        const std::string letra = P.substr(c + 7, 6);

        // Todo fondo sobre el que puede caer esa letra.
        const char* vars[] = { "--fold", "--check", "--bet1", "--bet2", "--bet3",
                               "--out", "--out2" };
        int flojos = 0;
        for (const char* v : vars) {
            const std::string col = color_de(P, v);
            if (!truth(std::string("the page defines ") + v, col.size() == 6)) continue;
            const double r = contraste(letra, col);
            char m[120];
            std::snprintf(m, sizeof m, "%s sobre %s da %.2f, hace falta 4.5", v, col.c_str(), r);
            if (!truth(std::string("the label reads on ") + v, r >= 4.5, m)) ++flojos;
        }
        truth("so nothing in the grid is unreadable", flojos == 0,
              std::to_string(flojos) + " colores con poco contraste");
    }

    void the_convergence_bands_are_where_we_put_them() {
        const std::string P = WEBUI_PAGE;
        const size_t i = P.find("function bandaConv(");
        if (!truth("there is a convergence band function", i != std::string::npos)) return;
        const std::string cuerpo = P.substr(i, P.find("\n}", i) - i);

        struct Banda { const char* corte; const char* nombre; };
        const Banda bandas[] = {
            { "x < 0.5",  "convergida"    },
            { "x < 2",    "buena"         },
            { "x < 5",    "utilizable"    },
            { "x < 20",   "floja"         },
            { "",         "sin converger" },
        };
        for (const Banda& b : bandas) {
            if (*b.corte)
                truth(std::string("the band at ") + b.corte + " is still there",
                      cuerpo.find(b.corte) != std::string::npos, cuerpo);
            truth(std::string("and it is called '") + b.nombre + "'",
                  cuerpo.find(b.nombre) != std::string::npos, cuerpo);
        }
        // Y que exista la pildora donde se pinta.
        truth("and there is a pill to paint it on",
              P.find("id=\"convPill\"") != std::string::npos);
    }

    // Un numero se escribe igual en todas partes, y lo que la ayuda dice que
    // escribas es lo que el programa te devuelve escrito.
    //
    // MEDIDO en la pantalla antes de tocar nada, tres sitios:
    //
    //   la pildora de convergencia ensena "0.003% convergida" y su propio globo
    //   de ayuda, sobre ese mismo numero, decia "menos de 0,5% convergida".
    //   Punto arriba, coma abajo, a un palmo.
    //
    //   la barra de arriba sacaba los nodos con toLocaleString, que deja la
    //   decision al navegador: "43.271 nodos" en un Chrome en espanol y "43,271
    //   nodos" en uno en ingles, al lado de un "0.53 GB" que no cambia nunca. El
    //   mismo punto, dos significados en la misma linea.
    //
    //   la ayuda de tamanos ponia "2,5x" como ejemplo de subida con decimal, y
    //   `fmt_sizings` escribe "2.5x" en el campo en cuanto construyes el arbol.
    //   La ayuda y el programa no se ponian de acuerdo sobre como se escribe un
    //   numero en el mismo campo.
    //
    // El decimal es el PUNTO, y no es un gusto: es lo que lleva todo lo que sale
    // de aqui -- el fichero guardado, el CSV, el JSON de la API, la consola -- y
    // lo que usa la referencia. Los miles van con un espacio fino, que no se
    // confunde con un decimal en ningun idioma.
    //
    // Escribir con coma SIGUE valiendo. Lo que se unifica es lo que se ENSENA,
    // no lo que se acepta, y eso tambien se comprueba aqui abajo.
    void every_number_is_written_the_same_way() {
        // 1. Lo que el programa escribe. Esto es comportamiento, no letra.
        {
            std::vector<Sizing> v;
            std::string e;
            if (truth("a raise with a decimal is taken", parse_raises("2.5x", v, e), e) &&
                same("and it is one raise", static_cast<long long>(v.size()), 1)) {
                const std::string w = fmt_sizings(v);
                truth("and the program writes it back with a dot", w == "2.5x", w);
            }
        }
        // La coma sigue valiendo AL ESCRIBIR: quien la teclea la tiene, y le
        // vuelve escrita como la escribe el programa.
        {
            std::vector<Sizing> v;
            std::string e;
            if (truth("a raise typed with a comma still works", parse_raises("2,5x", v, e), e) &&
                same("and it is one raise too", static_cast<long long>(v.size()), 1)) {
                const std::string w = fmt_sizings(v);
                truth("and it comes back written the same way", w == "2.5x", w);
            }
        }
        // 2. El error nombra el tamano como el programa lo escribe.
        {
            std::vector<Sizing> v;
            std::string e;
            if (truth("a percentage in the raise field is refused",
                      !parse_raises("50", v, e), "se acepto un porcentaje")) {
                truth("and the error spells the decimal with a dot",
                      e.find("2.5x") != std::string::npos &&
                      e.find("2,5x") == std::string::npos, e);
            }
        }
        // 3. Y la pantalla, igual que el programa.
        const std::string P = WEBUI_PAGE;
        truth("the size help shows the dot",
              P.find("<code>2.5x</code>") != std::string::npos,
              "la ayuda no ensena 2.5x, que es lo que el programa escribe");
        truth("and still says the comma works",
              P.find("<code>2,5x</code>") != std::string::npos,
              "se ha dejado de contar que la coma tambien vale");
        truth("the accuracy legend uses the same decimal mark as the pill",
              P.find("menos de 0.5%") != std::string::npos &&
              P.find("0,5 a 2%") == std::string::npos,
              "el globo dice 0,5 y la pildora que explica dice 0.5");
        // 4. Ningun numero queda al gusto del navegador.
        truth("no number is left to the browser locale",
              P.find(".toLocaleString") == std::string::npos,
              "queda un toLocaleString: ese numero cambia segun quien abra la pagina");
        truth("there is one place that writes them",
              P.find("function num(x, dec){") != std::string::npos);
        // Los dos numeros de la barra de arriba, cada uno por su sitio exacto:
        // los nodos, y los GB con DOS decimales, que es lo que escribe la barra
        // y no la nota del arbol -- esa pone tres. Sin esa precision, buscar
        // solo "instNodes,0)+' nodos" valia tambien para la nota del arbol, y
        // una mutacion en la barra pasaba por debajo. Probado: pasaba.
        truth("and the top bar writes its nodes with it",
              P.find("num(state.instNodes,0)+t(' nodos") != std::string::npos &&
              P.find("num(state.memGB,2)+' GB'") != std::string::npos,
              "la barra de arriba ya no escribe sus numeros con num()");
        // 5. Y la pagina dice en que idioma esta, y lo dice SIEMPRE: arranca en
        //    ingles y al cambiar de idioma el atributo cambia con ella. Sin eso
        //    el navegador ofrece traducir lo que ya esta traducido y un lector
        //    de pantalla lo pronuncia en el idioma que no es.
        truth("and the page says which language it is in",
              P.find("<html lang=\"en\">") != std::string::npos,
              "el html no declara idioma de salida");
        truth("and keeps saying it when you change it",
              P.find("document.documentElement.lang = idioma;") != std::string::npos,
              "cambiar de idioma no cambia lo que declara el html");
    }

    // Lo que se ENSENA, en espanol. Y sin jerga en la barra de arriba.
    //
    // MEDIDO antes de tocar nada: 28 trozos en ingles seguian a la vista en una
    // interfaz que se dio por traducida hace tres commits. La barra de estado
    // decia "building tree...", "applying range...", "solved in 3.21s"; el globo
    // de la pildora de tamano estaba entero en ingles -- "Runout cards
    // equivalent under a suit permutation share one solve" -- y el mazo pedia
    // "click to add it to the flop". La traduccion se hizo a ojo y lo que no se
    // miro se quedo como estaba, porque no habia nada que lo vigilara.
    //
    // Y la barra de arriba hablaba en jerga: "palos colapsados x6" no se lo dice
    // a nadie. Lo que pasa ahi es que seis runouts que solo se diferencian en el
    // palo dan la misma estrategia y se resuelven una vez, asi que ahora pone
    // "6 runouts en uno" y el globo lo cuenta en una frase.
    //
    // LA REGLA, que es la misma que se puede comprobar sin un parser de HTML:
    // de cada linea que no sea comentario se miran los trozos ENTRECOMILLADOS
    // -- las cadenas de JavaScript de la pagina -- y el texto suelto del HTML
    // entre `>` y `<`, y se buscan palabras que en espanol no existen. Los
    // terminos de poker se quedan en ingles a proposito, como se pidio, y
    // ninguno de ellos es una de estas: son articulos, preposiciones y verbos
    // ingleses, que no aparecen en una frase espanola por accidente.
    //
    // HASTA DONDE LLEGA. Esto caza una FRASE en ingles, que es lo que se colaba.
    // No caza una palabra suelta -- un boton que ponga "Apply" pasa entero --
    // porque la lista tendria que llevar palabras que tambien son espanolas y
    // empezaria a dar falsos avisos sobre texto correcto. Y solo mira la pagina
    // web: la consola de texto tiene su propia ayuda y sigue en ingles.
    void nothing_on_screen_is_left_in_english() {
        const std::string P = WEBUI_PAGE;
        static const char* const MARCAS[] = {
            " the ", "The ", " and ", " with ", " that ", " this ", " which ",
            " every ", " what ", " when ", " because ", " instead ", " rather ",
            " your ", " you ", " they ", " are ", " is ", " was ", " will ",
            " would ", " cannot ", " from ", " for ", " of ", " to ", " it ",
            " so ", " its ", " each ", " there ", " here ", " have ", " but "
        };
        // La tabla de traducciones es inglesa a proposito: es el otro idioma,
        // no un descuido. Se salta entera, y de que este completa se encarga
        // `every_spanish_line_on_screen_has_an_english_one`.
        const size_t tab0 = P.find("const EN = {");
        const size_t tab1 = (tab0 == std::string::npos)
                          ? std::string::npos : P.find("\n};", tab0);
        int cuantos = 0;
        std::string muestra;
        bool en_bloque = false, en_html = false;
        size_t i = 0;
        while (i < P.size()) {
            size_t f = P.find('\n', i);
            if (f == std::string::npos) f = P.size();
            const std::string ln = P.substr(i, f - i);
            const size_t donde = i;
            i = f + 1;
            if (tab0 != std::string::npos && donde >= tab0 && donde <= tab1) continue;
            const std::string t = trim(ln);
            if (t.rfind("/*", 0) == 0) en_bloque = true;
            if (en_bloque) {
                if (t.find("*/") != std::string::npos) en_bloque = false;
                continue;
            }
            if (t.rfind("<!--", 0) == 0) en_html = true;
            if (en_html) {
                if (t.find("-->") != std::string::npos) en_html = false;
                continue;
            }
            if (t.rfind("//", 0) == 0 || t.rfind("*", 0) == 0) continue;

            std::vector<std::string> trozos;
            // Las cadenas de JavaScript, entre comillas simples.
            size_t k = 0;
            while (true) {
                const size_t a = ln.find('\'', k);
                if (a == std::string::npos) break;
                const size_t b = ln.find('\'', a + 1);
                if (b == std::string::npos) break;
                trozos.push_back(ln.substr(a + 1, b - a - 1));
                k = b + 1;
            }
            // Y el texto suelto del HTML, entre un `>` y el `<` siguiente.
            k = 0;
            while (true) {
                const size_t a = ln.find('>', k);
                if (a == std::string::npos) break;
                const size_t b = ln.find('<', a + 1);
                if (b == std::string::npos) break;
                if (b - a - 1 >= 4) trozos.push_back(ln.substr(a + 1, b - a - 1));
                k = b + 1;
            }

            for (const std::string& z : trozos)
                for (const char* m : MARCAS)
                    if (z.find(m) != std::string::npos) {
                        ++cuantos;
                        if (muestra.size() < 240) muestra += " | " + trim(z);
                        break;
                    }
        }
        truth("nothing on screen is left in English", cuantos == 0,
              std::to_string(cuantos) + " trozos en ingles a la vista:" + muestra);

        // Y las palabras sueltas que la regla de arriba NO caza, una por una.
        //
        // La regla busca frases -- articulos, preposiciones, verbos ingleses --
        // y un boton que ponga "Guardar tree" pasa entero. Salieron tres en la
        // pasada de antes de publicar: la pildora de calle decia "flop solve",
        // el boton de guardar "Guardar tree", y su texto de ayuda hablaba de
        // un "tree". Aqui quedan fijadas por su sitio, que es lo unico que se
        // puede comprobar sin inventarse una lista de palabras prohibidas que
        // daria falsos avisos sobre los terminos de poker.
        truth("the street pill says it in Spanish",
              P.find("t('desde el ')+state.streetName") != std::string::npos &&
              P.find("state.streetName+' solve'") == std::string::npos,
              "la pildora volvio a 'flop solve'");
        truth("and the save button too",
              P.find(">Guardar") != std::string::npos &&
              P.find("Guardar tree") == std::string::npos,
              "el boton volvio a 'Guardar tree'");

        // Y la barra de arriba, en palabras. El numero sale del grupo de
        // simetria que sobrevive a los rangos y a los locks, que es el que de
        // verdad se aprovecha.
        truth("the top bar says it in words",
              P.find("' runouts en uno'") != std::string::npos,
              "la barra volvio a la jerga de 'palos colapsados'");
        truth("and not in jargon",
              P.find("palos colapsados") == std::string::npos);
    }

    // Un arbol guardado ocupa exactamente lo que dice que va a ocupar.
    //
    // MEDIDO en la maquina del usuario antes de tocar nada: doce ficheros y
    // 7,21 GB en saves/, con 33,8 GB libres de 464 -- el disco al 93% -- y nada
    // en pantalla que lo dijera. La lista ensena el tamano de cada arbol, pero
    // nadie suma doce numeros al vuelo.
    //
    // Y el aviso de que no cabe llegaba DESPUES de escribir dos gigas: el
    // fichero a medias se limpia -- eso ya estaba bien hecho -- pero el disco
    // lleno no lo arregla nadie desde aqui. Ahora se mira antes, y se puede
    // mirar antes porque el tamano se sabe EXACTO: es la cuenta de lo que
    // `save_state` escribe, campo por campo, no una estimacion.
    //
    // Esta comprobacion es esa cuenta contra el fichero de verdad. Si alguien
    // anade un campo al formato y no lo suma aqui, el numero se separa y esto
    // lo dice; y en cuanto se separa, el aviso de "no cabe" deja de valer.
    void a_saved_tree_is_exactly_as_big_as_it_says(Session& S) {
        std::string e;
        // Un river, que es el arbol mas barato: esto ESCRIBE el fichero para
        // pesarlo, y un flop resuelto son medio giga de ida y vuelta en un
        // disco que ya esta al 93%.
        if (!truth("size spot builds", spot(S, "Ah9h4hKdQs", e), e)) return;
        S.solve(20, 0);
        const long long dice = S.tree_file_bytes();
        if (!truth("it says beforehand what it is going to take", dice > 0,
                   std::to_string(dice))) return;

        // Un nombre de esta comprobacion, que se borra al terminar. Los arboles
        // del usuario no se tocan.
        const char* nombre = "check_tamano_tmp";
        if (!truth("and it saves", S.save_tree(nombre, e), e)) return;
        const long long real = S.save_size(true, nombre);
        same("and the file is exactly that many bytes", real, dice);
        std::string e2;
        truth("and the check cleans up after itself",
              S.delete_save(true, nombre, e2), e2);

        // La decision de si cabe, con numeros inventados: es lo unico que no se
        // puede provocar de verdad sin llenar un disco.
        const long long G = 1024LL * 1024 * 1024;
        truth("a tree that fits is saved", Session::fits_on_disk(G, 10 * G));
        // Justo al borde: cabe por 100 MB, y 100 MB no es sitio. El margen son
        // 256 MB, asi que este es el caso que distingue "cabe" de "cabe pero
        // deja la maquina sin aire". La primera vez puse medio giga de sobra y
        // la comprobacion fallaba teniendo razon el codigo: media 9,5 contra 9
        // y eso SI pasa el margen.
        truth("one that would leave no room is refused",
              !Session::fits_on_disk(9 * G, 9 * G + 100LL * 1024 * 1024));
        truth("one that does not fit at all is refused",
              !Session::fits_on_disk(10 * G, 2 * G));
        truth("and not knowing the free space blocks nobody",
              Session::fits_on_disk(G, -1));

        // Y el aviso salta ANTES de escribir nada, que es de lo que se trata.
        // Se le dice cuanto sitio hay porque llenar un disco de verdad para
        // probarlo no es una opcion.
        // Y si una vuelta anterior lo dejo escrito -- una mutacion que quite el
        // aviso lo escribe de verdad -- se borra antes de mirar, o la vuelta
        // siguiente falla por culpa de la anterior. Paso.
        std::string e0;
        S.delete_save(true, "check_sitio_tmp", e0);
        std::string e3;
        truth("saving with no room is refused",
              !S.save_tree("check_sitio_tmp", e3, 0),
              "guardo igual con el disco lleno");
        truth("and it says what it needs and what there is",
              e3.find("GB") != std::string::npos, e3);
        truth("and it did not write a thing",
              S.save_size(true, "check_sitio_tmp") < 0,
              "quedo un fichero a medias del que se acaba de negar a escribir");

        // Y el disco de verdad se sabe leer.
        truth("the free space on the saves disk is readable",
              Session::free_bytes_for_saves() > 0,
              std::to_string(Session::free_bytes_for_saves()));

        // Lo que se ensena. El total y el sitio libre salen por la API, y la
        // pagina los pinta: sin eso la cuenta existe y no la ve nadie.
        //
        // HASTA DONDE LLEGA: esto mira la PAGINA, que es texto y esta aqui al
        // lado. Lo que no mira es que /api/saves siga mandando los numeros;
        // para eso habria que levantar un servidor dentro de esta comprobacion,
        // y no compensa por lo que cubre. Si algun dia dejan de mandarse, la
        // linea saldra a medias -- "Lo guardado ocupa 7,2 GB" sin el resto --
        // y no dira nada falso.
        const std::string P = WEBUI_PAGE;
        truth("the page has a place to say it",
              P.find("id=\"diskNote\"") != std::string::npos);
        truth("and it says what everything takes",
              P.find("Lo guardado ocupa") != std::string::npos);
        truth("and what is left on the disk",
              P.find("libres en el disco") != std::string::npos);
    }

    // Una FAMILIA entera en un solo lock, que es como se nodelockea de verdad.
    //
    // Lo pidio el usuario para las barras del reparto por categorias: "este tio
    // nunca frena con dos parejas" es una frase sobre una familia, y decirla
    // pintando cuarenta combos a mano es justo lo que hace que nadie use el
    // nodelock. Ahora el selector entiende los nombres de la referencia -- `lock two_pair
    // B=90%` -- y la barra de la pantalla manda exactamente eso.
    //
    // Lo que se vigila aqui es lo que puede salir mal sin que se note:
    //   que coja esa familia y NADA mas
    //   que la mire con el board DEL NODO, que en un river es otro board
    //   que el lock se escriba en el runout donde se leyo
    //   y que con dos tamanos de apuesta mueva el que se le dijo
    void a_whole_family_is_one_lock(Session& S) {
        std::string e;
        if (!truth("family spot builds", spot(S, "Ah9h4h", e), e)) return;
        // Una de cada cosa: AK y AQ son top pair, AA es set, A9s son dos
        // parejas, y KK y T9s no llevan as.
        if (!truth("a range with one of each", S.set_range(0, "AA,AKo,AQo,A9s,KK,T9s", e), e)) return;
        if (!truth("and something for IP", S.set_range(1, "JJ,TT,AQo", e), e)) return;
        if (!truth("the tree rebuilds", S.rebuild(e), e)) return;
        S.solve(100, 0);

        // 1. El selector coge la familia entera y nada mas. No se compara contra
        //    una lista escrita a mano: se cuenta cuantas hay de verdad en el
        //    rango y tienen que ser esas.
        std::vector<int> manos;
        if (!truth("top_pair picks hands", S.resolve_hands_public("top_pair", 0, manos, e), e))
            return;
        const Deal& D = S.deal();
        long long deberian = 0;
        for (int h = 0; h < D.num(); ++h)
            if (S.range(0)[static_cast<size_t>(h)] > 0.0 &&
                made_cat(D.combos[static_cast<size_t>(h)].c1,
                         D.combos[static_cast<size_t>(h)].c2, D.board) == MC_TOPPAIR)
                ++deberian;
        same("and it picks every one of them", static_cast<long long>(manos.size()), deberian);
        int intrusos = 0;
        for (int h : manos) {
            const Combo& k = D.combos[static_cast<size_t>(h)];
            if (made_cat(k.c1, k.c2, D.board) != MC_TOPPAIR) ++intrusos;
        }
        truth("and not one hand that is not top pair", intrusos == 0,
              std::to_string(intrusos) + " intrusos");

        // 2. Con el board DEL NODO. Tras el Kd y el 2s, AK ya no es top pair:
        //    son dos parejas. Un selector que mirara el board de partida las
        //    meteria igual, y el lock diria una cosa distinta de la que pone.
        S.go_root();
        const char* camino[] = { "X", "X", "Kd", "X", "X", "2s" };
        bool anduvo = true;
        for (const char* paso : camino)
            if (anduvo && !S.go(paso, e)) { anduvo = false; }
        if (!truth("there is a river six checks down", anduvo, e)) return;
        const int rci = S.cur_ctx(), rnid = S.cur_node();
        const std::vector<int> rslots = S.slots();
        // Y la sesion se vuelve a la raiz A PROPOSITO: el lock tiene que salir
        // igual, porque las cartas van en la peticion y no en donde ande nadie.
        S.go_root();
        S.clear_all_locks();
        int movidas = 0;
        if (!truth("a family moves at a node nobody is standing on",
                   S.add_lock_moved_at(rci, rnid, rslots, "top_pair", AK_BET, -1,
                                       LM_FIXED, 0.9, movidas, e), e)) return;
        truth("and it moved some", movidas > 0, std::to_string(movidas));
        int con_rey = 0, mal_runout = 0;
        for (const LockSpec& L : S.locks()) {
            if (L.hand_spec.find('K') != std::string::npos) ++con_rey;
            if (L.runout != "Kd2s") ++mal_runout;
        }
        truth("and AK is not in it: on this river it is two pair", con_rey == 0,
              std::to_string(con_rey) + " combos con rey");
        truth("and every lock was written on this runout", mal_runout == 0,
              std::to_string(mal_runout) + " en otro runout");

        // 3. Y con DOS tamanos de apuesta, mueve el que se le dijo. Es el caso
        //    del river del usuario -- 75% y 300% -- y el que hace que "la
        //    apuesta" no signifique nada por si sola.
        S.clear_all_locks();
        S.go_root();
        std::vector<Sizing> dos;
        if (!truth("two flop sizes parse", parse_sizings("33,100", dos, e), e)) return;
        if (!truth("and apply", S.set_sizings(true, 0, dos, e), e)) return;
        if (!truth("the tree takes them", S.rebuild(e), e)) return;
        S.solve(100, 0);
        const BetTree& bt = S.tree().ctx[0].tree;
        const int raiz = bt.root;
        const Node& rn = bt.nodes[static_cast<size_t>(raiz)];
        int b1 = -1, b2 = -1;
        for (int a = 0; a < rn.num_actions; ++a)
            if (bt.act(rn, a).kind == AK_BET) { if (b1 < 0) b1 = a; else b2 = a; }
        if (!truth("the root really has two bet sizes", b1 >= 0 && b2 >= 0)) return;
        // Se mueve el tamano que la familia NO usa, y se mira antes y despues.
        //
        // La primera version movia el grande, que en este spot top pair ya
        // apostaba el 83% por su cuenta: el assert pasaba con el lock puesto y
        // sin el, o sea que no vigilaba nada. Medido a mano en la consola. Si el
        // "antes" no esta lejos del objetivo, la comprobacion no vale.
        const NodeStats A0 = gather(*S.solver(), 0, raiz, 0, S.deal().identity());
        if (!truth("the node reads before the lock", A0.ok)) return;
        double antes = 0.0, wa = 0.0;
        for (int h : manos) {
            const int hs = A0.stored(h);
            const double ww = A0.weight[static_cast<size_t>(hs)];
            if (ww <= 0.0) continue;
            wa += ww;
            antes += ww * A0.freq(hs, b1);
        }
        if (!truth("top pair is at that node", wa > 0.0)) return;
        antes /= wa;
        truth("and the small size is not what it plays by itself", antes < 0.5,
              "ya jugaba " + fmt_sci(antes) + ", asi que moverla no probaria nada");

        int m3 = 0;
        if (!truth("the family moves onto the size it was not using",
                   S.add_lock_moved_at(0, raiz, S.slots(), "top_pair", AK_BET, b1,
                                       LM_FIXED, 0.9, m3, e), e)) return;
        S.solve(200, 0);
        const NodeStats N = gather(*S.solver(), 0, raiz, 0, S.deal().identity());
        if (!truth("the node reads back", N.ok)) return;
        double f1 = 0.0, f2 = 0.0, w = 0.0;
        for (int h : manos) {
            const int hs = N.stored(h);
            const double ww = N.weight[static_cast<size_t>(hs)];
            if (ww <= 0.0) continue;
            w  += ww;
            f1 += ww * N.freq(hs, b1);
            f2 += ww * N.freq(hs, b2);
        }
        if (!truth("and top pair is still there", w > 0.0)) return;
        f1 /= w; f2 /= w;
        char m[200];
        std::snprintf(m, sizeof m, "antes %.3f, ahora la pequena %.3f y la grande %.3f",
                      antes, f1, f2);
        truth("the size it was told to move is the one that moved",
              std::fabs(f1 - 0.9) < 0.05, m);
        truth("and it came from the other one", f2 < 0.2, m);

        // 4. Y la pantalla lo ofrece: la barra es una celda `fambar` que lleva
        //    el nombre de la familia en un `data-fam`, y lo que manda al mover
        //    es ESE NOMBRE, no una lista de trescientos combos.
        const std::string P = WEBUI_PAGE;
        // Por el PRINCIPIO de la clase: la celda anade " lk" cuando la familia
        // ya esta bloqueada, asi que buscar la clase entera se rompia al
        // anadir la marca. Paso.
        truth("the category panel has bars to drag",
              P.find("<td class=\"fambar") != std::string::npos &&
              P.find("  td.fambar{") != std::string::npos,
              "el reparto por categorias se quedo sin barras");
        truth("and each one carries its family name",
              P.find("data-fam=\"") != std::string::npos);
        truth("and moving one sends the family by name",
              P.find("hands:fam") != std::string::npos);
        truth("and the exact action with it",
              P.find("acti:ai") != std::string::npos);
    }

    // Poner un lock NO tira la solucion que hay en pantalla.
    //
    // Lo conto el usuario en cuanto probo las barras: "al moverla lo mas minimo
    // y soltarla desaparece toda la estrategia, y hasta que no le das a solvear
    // no vuelve". Con eso no se puede mover mas de una barra -- hay que
    // resolver entre una y otra, y la segunda se hace a ciegas -- que es
    // justo lo contrario del flujo para el que existe el nodelock, y que este
    // mismo codigo describe: quitarle los pagos a un grupo, hacer que otro
    // apueste mas, y LUEGO resolver.
    //
    // Estaba en `add_lock_at`, que acababa con `solved_ = false`. Lo que se ve
    // sigue siendo una solucion de verdad -- la de antes del lock --, asi que
    // ahora se queda y lo que se dice es que hay locks SIN APLICAR.
    void a_lock_does_not_throw_the_solve_away(Session& S) {
        std::string e;
        if (!truth("pending-lock spot builds", spot(S, "Ah9h4h", e), e)) return;
        if (!truth("a range to lock", S.set_range(0, "AA,AKo,AQo,KK,T9s", e), e)) return;
        if (!truth("and one for IP", S.set_range(1, "JJ,TT,AQo", e), e)) return;
        if (!truth("the tree rebuilds", S.rebuild(e), e)) return;
        S.clear_all_locks();
        S.solve(100, 0);
        if (!truth("there is a solution to start with", S.solved())) return;
        truth("and nothing pending yet", !S.locks_pending());

        const BetTree& bt = S.tree().ctx[0].tree;
        const int raiz = bt.root;
        const Node& rn = bt.nodes[static_cast<size_t>(raiz)];
        const int ib = bt.action_index(rn, AK_BET);
        if (!truth("the root can bet", ib >= 0)) return;

        int m1 = 0;
        if (!truth("one family moves",
                   S.add_lock_moved_at(0, raiz, S.slots(), "top_pair", AK_BET, ib,
                                       LM_FIXED, 0.9, m1, e), e)) return;
        truth("and the solution is still there", S.solved(),
              "el lock se llevo por delante el solve que habia");
        truth("and it says the lock is not in it yet", S.locks_pending());

        // Y la segunda barra se puede mover SIN resolver en medio, que es lo
        // que se pedia. Se mueve `underpair`, que es lo que KK es en un board
        // con as: `overpair` no existe aqui, y la primera version de esto lo
        // pedia y fallaba por eso. Ademas tiene que poder LEER la estrategia para saber de
        // donde parte, o el movimiento saldria de la nada.
        int m2 = 0;
        if (!truth("and a second family moves without solving in between",
                   S.add_lock_moved_at(0, raiz, S.slots(), "underpair", AK_BET, ib,
                                       LM_FIXED, 0.2, m2, e), e)) return;
        truth("and both are waiting",
              static_cast<long long>(S.locks().size()) >= m1 + m2);
        truth("with the solution still on screen", S.solved());

        // Y al resolver dejan de estar pendientes.
        S.solve(100, 0);
        truth("solving takes them in", !S.locks_pending());
        truth("and there is still a solution", S.solved());

        // Lo que se ensena mientras tanto: la pildora de convergencia dice que
        // el numero es el de antes, porque ese es el que se lee como "ya esta".
        const std::string P = WEBUI_PAGE;
        truth("the page knows about pending locks",
              P.find("state.locksPending") != std::string::npos);
        truth("and the convergence pill says so",
              P.find("locks sin aplicar") != std::string::npos);
        S.clear_all_locks();
    }

    // Cada familia tiene su deshacer, y deshacer una no toca a las demas.
    //
    // Lo pidio el usuario junto con las barras: "un boton de reset por familia
    // para devolverlo al estado original". El unico camino que habia quitaba
    // TODOS los locks del nodo, asi que arrepentirse de las dobles parejas se
    // llevaba por delante lo que hubieras hecho con los colores.
    void each_family_has_its_own_undo(Session& S) {
        std::string e;
        if (!truth("undo spot builds", spot(S, "Ah9h4h", e), e)) return;
        if (!truth("a range with two families", S.set_range(0, "AA,AKo,AQo,A9s,KK", e), e))
            return;
        if (!truth("and one for IP", S.set_range(1, "JJ,TT,AQo", e), e)) return;
        if (!truth("the tree rebuilds", S.rebuild(e), e)) return;
        S.clear_all_locks();
        S.solve(100, 0);
        const BetTree& bt = S.tree().ctx[0].tree;
        const int raiz = bt.root;
        const int ib = bt.action_index(bt.nodes[static_cast<size_t>(raiz)], AK_BET);
        if (!truth("the root can bet", ib >= 0)) return;

        int a = 0, b = 0;
        if (!truth("top pair moves",
                   S.add_lock_moved_at(0, raiz, S.slots(), "top_pair", AK_BET, ib,
                                       LM_FIXED, 0.8, a, e), e)) return;
        if (!truth("and two pair moves too",
                   S.add_lock_moved_at(0, raiz, S.slots(), "two_pair", AK_BET, ib,
                                       LM_FIXED, 0.8, b, e), e)) return;
        same("both are in", static_cast<long long>(S.locks().size()), a + b);

        int quitados = 0;
        if (!truth("undoing top pair works",
                   S.remove_locks_for(0, raiz, S.slots(), "top_pair", quitados), e))
            return;
        same("and it took exactly its own", static_cast<long long>(quitados), a);
        same("leaving the other family alone",
             static_cast<long long>(S.locks().size()), b);

        // Y lo que queda es two_pair de verdad, no un resto de la otra.
        int ajenos = 0;
        std::vector<int> suyas;
        std::string e2;
        if (truth("two pair still resolves", S.resolve_hands_public("two_pair", 0, suyas, e2), e2)) {
            std::set<std::string> nombres;
            for (int h : suyas) {
                const Combo& k = S.deal().combos[static_cast<size_t>(h)];
                nombres.insert(card_str(k.c1) + card_str(k.c2));
            }
            for (const LockSpec& L : S.locks())
                if (!nombres.count(no_space(L.hand_spec))) ++ajenos;
        }
        truth("and every lock left belongs to it", ajenos == 0,
              std::to_string(ajenos) + " que no son de two_pair");

        // Deshacer algo que no esta puesto no rompe nada ni miente. Y tiene
        // que ser una familia que EXISTE en el rango -- KK es underpair en un
        // board con as -- porque con una que no existe la funcion se va antes
        // de llegar a esto, y entonces no se prueba lo que se cree. Paso: con
        // `flush` esta comprobacion daba verde con el codigo roto.
        int cero = 0;
        truth("undoing what is not there says so",
              !S.remove_locks_for(0, raiz, S.slots(), "underpair", cero));
        same("and removes nothing", static_cast<long long>(cero), 0);
        same("and the rest is untouched",
             static_cast<long long>(S.locks().size()), b);

        // Lo que se ensena: la barra se queda donde la sueltas -- si no, parece
        // que el arrastre no hizo nada -- y cada familia tocada lleva su boton.
        const std::string P = WEBUI_PAGE;
        truth("the page keeps what you asked for until it is solved",
              P.find("function pendPon(") != std::string::npos &&
              P.find("(ped||g.f).forEach(") != std::string::npos,
              "la fila volveria a pintar el solve viejo y el arrastre pareceria perdido");
        truth("and each touched family has its undo",
              P.find("class=\"undo\"") != std::string::npos &&
              P.find("resetFamilia(") != std::string::npos);
        S.clear_all_locks();
    }

    // Los botones que manda pulsar el README existen con ese nombre.
    //
    // MEDIDO en la pasada de antes de publicar: el arranque de cinco minutos
    // mandaba pulsar "Pick flop", "Build tree" y "Go", que son los nombres de
    // antes de traducir la interfaz. Lo primero que lee alguien que se baja
    // esto no coincidia con lo que ve en pantalla.
    //
    // Se lee el fichero de al lado. Si no esta -- porque esto corre desde otro
    // sitio -- no se inventa nada y se dice que no se comprobo, que es distinto
    // de decir que esta bien.
    // El programa dice cual es.
    //
    // En cuanto esto sale de una maquina y llega a un Discord, la primera
    // pregunta ante cualquier problema es "que version tienes". Sin un numero
    // a la vista, la respuesta es "actualiza y prueba", que no es una
    // respuesta. Sale en --help, en la barra de arriba y en el titulo de la
    // pestana, y de un solo sitio para que no se contradigan.
    void the_build_says_which_one_it_is() {
        const std::string v = cfg::VERSION;
        truth("there is a version", v.size() >= 3, "'" + v + "'");
        truth("and it looks like one", v.find('.') != std::string::npos, v);
        const std::string P = WEBUI_PAGE;
        truth("the top bar has a place for it",
              P.find("id=" + std::string(1, '"') + "ver" + std::string(1, '"')) !=
                  std::string::npos);
        // La linea EXACTA que lo rellena. Buscar "state.version" a secas valia
        // tambien para el titulo de la pestana, asi que vaciar la barra pasaba
        // por debajo. Probado: pasaba.
        truth("and it fills it with what the server says",
              P.find("vr.textContent='v'+state.version") != std::string::npos,
              "la barra tiene el hueco pero nadie lo rellena");
    }

    // Lee un fichero de al lado, si esta.
    static bool leer_fichero(const char* ruta, std::string& out) {
        std::ifstream f(ruta);
        if (!f) return false;
        out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        return true;
    }

    // Los workflows tienen que ser YAML de verdad.
    //
    // MEDIDO el dia que se publico: los dos estaban rotos y NINGUNO habia
    // llegado a ejecutarse nunca. GitHub los daba por fallidos en cero
    // segundos con un "workflow file issue" y sin mas detalle.
    //
    // El fallo era este, dos veces:
    //
    //     run: "$SOLVER" --bench | tail -20
    //
    // Un escalar que EMPIEZA por comilla es una cadena entrecomillada, y lo que
    // venga detras de la comilla de cierre es un error de sintaxis. Con un
    // bloque `run: |` delante no pasa.
    //
    // Aqui no hay con que parsear YAML, asi que se comprueba exactamente la
    // regla que se rompio, que ademas es la unica forma razonable de escribir
    // una orden con comillas dentro.
    void the_workflows_are_not_broken_yaml() {
        static const char* const FICH[] = { ".github/workflows/build.yml",
                                            ".github/workflows/release.yml" };
        int vistos = 0, malas = 0;
        std::string cuales;
        for (const char* f : FICH) {
            std::string doc;
            if (!leer_fichero(f, doc)) continue;
            ++vistos;
            size_t i = 0;
            while (i < doc.size()) {
                size_t fin = doc.find('\n', i);
                if (fin == std::string::npos) fin = doc.size();
                const std::string linea = trim(doc.substr(i, fin - i));
                i = fin + 1;
                if (linea.rfind("run:", 0) != 0) continue;
                const std::string resto = trim(linea.substr(4));
                if (resto.empty()) continue;
                // Un bloque (`|`, `>`) vale. Empezar por comilla, no.
                if (resto[0] == '"' || resto[0] == '\'') {
                    ++malas;
                    if (cuales.size() < 160)
                        cuales += std::string(" | ") + f + ": " + linea.substr(0, 60);
                }
            }
        }
        if (vistos == 0) {
            truth("the workflows are not next to us, so this was not checked", true);
            return;
        }
        truth("no workflow starts a run: with a quote", malas == 0,
              std::to_string(malas) + " lineas que no son YAML valido:" + cuales);
    }

    // Con una sala de poker abierta, esto no se abre.
    //
    // Lo que se comprueba aqui no es si en ESTA maquina hay una sala abierta --
    // eso depende de quien corra la bateria -- sino la regla: que la lista
    // reconoce los clientes que tiene que reconocer, y sobre todo que NO se
    // lleva por delante lo que no es una sala.
    //
    // Lo segundo importa mas que lo primero. Un falso negativo deja pasar una
    // sala; un falso positivo deja el programa inservible para alguien que
    // tiene PokerTracker abierto, que es justo la herramienta con la que se
    // estudia. Por eso la lista busca marcas y no la palabra "poker".
    void it_refuses_to_open_next_to_a_poker_room() {
        // 1. Las salas que se pidieron, con los nombres con los que salen sus
        //    clientes en el administrador de tareas.
        struct Caso { const char* proceso; const char* sala; };
        static const Caso SALAS[] = {
            { "PokerStars.exe",        "PokerStars" },
            { "PokerStarsUpdate.exe",  "PokerStars" },
            { "pokerstars.eu.exe",     "PokerStars" },
            { "GGPoker.exe",           "GGPoker" },
            { "Winamax.exe",           "Winamax" },
            { "winamax poker.exe",     "Winamax" },
            { "888poker.exe",          "888poker" },
            { "Poker888.exe",          "888poker" },
            { "PacificPoker.exe",      "888poker" },
            { "CoinPoker.exe",         "CoinPoker" },
            { "TitanPoker.exe",        "iPoker (Titan)" },
            { "iPokerClient.exe",      "iPoker" },
        };
        int mal = 0;
        std::string cuales;
        for (const Caso& c : SALAS) {
            const std::string vista = rooms::room_of_process(c.proceso);
            if (vista == c.sala) continue;
            ++mal;
            if (cuales.size() < 200)
                cuales += std::string(" | ") + c.proceso + " -> '" + vista + "'";
        }
        truth("every room in the list is recognised", mal == 0,
              std::to_string(mal) + " sin reconocer:" + cuales);

        // 2. Y LO QUE NO ES UNA SALA, no se toca. Un falso positivo aqui deja
        //    el programa inservible para quien tiene abierto justo lo que se
        //    usa para estudiar.
        static const char* const INOCENTES[] = {
            "PokerTracker4.exe", "HoldemManager3.exe", "Hand2Note.exe",
            "Flopzilla.exe", "PokerStove.exe", "GTOPlus.exe", "pokersnowie.exe",
            "solver.exe", "chrome.exe", "notepad.exe", "Discord.exe",
            "PokerJuice.exe", "equilab.exe",
        };
        int falsos = 0;
        std::string quienes;
        for (const char* p : INOCENTES) {
            const std::string vista = rooms::room_of_process(p);
            if (vista.empty()) continue;
            ++falsos;
            if (quienes.size() < 200)
                quienes += std::string(" | ") + p + " -> '" + vista + "'";
        }
        truth("and nothing else is mistaken for one", falsos == 0,
              std::to_string(falsos) + " falsos positivos:" + quienes);

        // 3. Mirar los procesos de la maquina funciona. No se mira QUE hay --
        //    depende de quien corra esto -- sino que la lista no viene vacia,
        //    que es lo que pasaria si la llamada al sistema fallara y dejaria
        //    el guardia apagado sin decirlo.
        const std::vector<std::string> procs = rooms::process_names();
#if defined(_WIN32) || defined(__linux__)
        truth("the machine's processes can be read", procs.size() > 5,
              "solo se ven " + std::to_string(procs.size()) + " procesos");
#else
        truth("processes are not read on this system, and that is said", true);
#endif

        // 4. Y el aviso dice QUE sala y POR QUE.
        const std::string aviso = rooms::why_not("PokerStars");
        truth("the message names the room",
              aviso.find("PokerStars") != std::string::npos, aviso.substr(0, 80));
        truth("and says why", aviso.size() > 120 &&
              (aviso.find("real-time") != std::string::npos ||
               aviso.find("tiempo") != std::string::npos),
              "el aviso no explica por que");
    }

    void the_readme_names_buttons_that_exist() {
        std::string en_doc, es_doc;
        const bool hay_en = leer_fichero("README.md", en_doc);
        const bool hay_es = leer_fichero("README.es.md", es_doc);
        if (!hay_en || !hay_es) {
            truth("the READMEs are not next to us, so this was not checked", true);
            return;
        }
        truth("both READMEs have something in them",
              en_doc.size() > 500 && es_doc.size() > 500,
              std::to_string(en_doc.size()) + " y " + std::to_string(es_doc.size()) +
              " bytes");

        const std::string P = WEBUI_PAGE;
        // Lo que el arranque manda pulsar, en los dos idiomas: como esta escrito
        // en la pantalla y como lo traduce la tabla. Los dos READMEs tienen que
        // nombrarlo cada uno en SU idioma -- el ingles con los nombres que ve
        // quien abre el programa, que de fabrica esta en ingles, y el castellano
        // con los del marcado.
        struct Boton { const char* es; const char* en; };
        static const Boton BOTONES[] = {
            { "Elegir flop",   "Pick flop" },
            { "Montar árbol",  "Build tree" },
            { "Resolver",      "Solve" },
            { "Precisión deseada (% del bote)", "Accuracy target (% of pot)" },
        };
        for (const Boton& b : BOTONES) {
            truth(std::string("the page has '") + b.es + "'",
                  P.find(b.es) != std::string::npos,
                  "el README lo manda pulsar y en la pantalla no esta");
            // La pareja EXACTA de la tabla, no el ingles suelto: "Solve" sale en
            // veinte sitios de la pagina y encontrarlo no dice que la tabla
            // traduzca el boton.
            truth(std::string("and the English table turns it into '") + b.en + "'",
                  P.find(std::string("'") + b.es + "':'" + b.en) != std::string::npos,
                  std::string("la tabla ya no lleva '") + b.es + "' a '" + b.en + "'");
            truth("and the Spanish README names it",
                  es_doc.find(b.es) != std::string::npos,
                  std::string("el arranque en castellano ya no nombra '") + b.es + "'");
            truth("and the English one names the English button",
                  en_doc.find(b.en) != std::string::npos,
                  std::string("el arranque en ingles ya no nombra '") + b.en + "'");
        }
        // Y los nombres que ya no existen en ningun idioma no pueden volver.
        static const char* const VIEJOS[] = { "Tree building parameters",
                                              "Go", "Apply range" };
        int fantasmas = 0;
        std::string cuales;
        for (const char* v : VIEJOS) {
            const std::string patron = std::string("**") + v + "**";
            if (en_doc.find(patron) != std::string::npos ||
                es_doc.find(patron) != std::string::npos) {
                ++fantasmas; cuales += std::string(" ") + v;
            }
        }
        truth("and neither sends you to buttons that no longer exist",
              fantasmas == 0, cuales);
    }

    // ---------------------------------------------------------------------
    // Lo que la consola IMPRIME, no lo que el fuente parece imprimir.
    //
    // Hasta aqui ninguna comprobacion miraba la pantalla de la consola, y ahi
    // estaba escondido un por ciento dentro de un printf sin argumentos desde
    // quien sabe cuando. Se redirige stdout a un fichero, se corre lo que sea, y
    // se lee. Si no se puede redirigir -- no deberia pasar -- se dice y no se da
    // por bueno.
    struct Grabado {
        std::string path;
        int  saved = -1;
        bool on = false;

        bool empezar() {
            path = "solver_check_stdout.tmp";
            std::fflush(stdout);
#ifdef _WIN32
            saved = _dup(_fileno(stdout));
#else
            saved = dup(fileno(stdout));
#endif
            if (saved < 0) return false;
            if (!std::freopen(path.c_str(), "w+", stdout)) { devolver(); return false; }
            on = true;
            return true;
        }
        void devolver() {
            if (saved < 0) return;
            std::fflush(stdout);
#ifdef _WIN32
            _dup2(saved, _fileno(stdout));
            _close(saved);
#else
            dup2(saved, fileno(stdout));
            close(saved);
#endif
            saved = -1;
        }
        std::string parar() {
            if (!on) return std::string();
            devolver();
            on = false;
            std::ifstream f(path.c_str(), std::ios::binary);
            std::string s((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
            f.close();
            std::remove(path.c_str());
            // Windows escribe el salto de linea como dos bytes cuando el
            // destino es un fichero. Lo que se compara es el texto, no como
            // lo guarda el sistema, asi que fuera.
            std::string limpio;
            limpio.reserve(s.size());
            for (char c : s) if (c != 13) limpio.push_back(c);
            return limpio;
        }
    };

    // Los comandos, tal cual los escribiria alguien, y lo que sale por pantalla.
    bool console_says(Session& S, const std::string& cmds, std::string& out) {
        Grabado g;
        if (!g.empezar()) { out.clear(); return false; }
        {
            std::istringstream in(cmds);
            Console c(S);
            c.run(in, false);
        }
        out = g.parar();
        return true;
    }

    static int digitos_seguidos(const std::string& s) {
        int mejor = 0, run = 0;
        for (char c : s) {
            if (c >= '0' && c <= '9') { ++run; if (run > mejor) mejor = run; }
            else run = 0;
        }
        return mejor;
    }

    // `help` sale por pantalla tal y como esta escrito.
    //
    // MEDIDO: la linea de `set accuracy` decia "less than this % of the pot"
    // dentro de un printf sin argumentos, y en pantalla salia "less than this
    // 25235616201f the pot" -- el por ciento seguido de espacio y o, leido como
    // una conversion a octal de un argumento que nadie paso. Es lo primero que
    // lee quien abre la consola.
    //
    // Se compara lo impreso con el texto: asi se caza tanto un por ciento comido
    // (printf) como uno doblado a mano (que ahora saldria como dos).
    void the_help_says_what_it_is_written_to_say() {
        Session S;
        std::string salida;
        if (!truth("the console output can be read back",
                   console_says(S, "help\n", salida),
                   "no se pudo redirigir stdout")) return;
        const std::string texto = CONSOLE_HELP;
        truth("the help is all there", salida.find(texto) != std::string::npos,
              "lo impreso no contiene el texto del help: " +
              std::to_string(salida.size()) + " bytes impresos, " +
              std::to_string(texto.size()) + " de texto");
        truth("and it still explains the accuracy target",
              salida.find("less than this % of the pot") != std::string::npos,
              "la linea de set accuracy no dice el por ciento del bote");
        // Un por ciento comido imprime un numero de la pila: cualquier tirada
        // larga de digitos ahi es un argumento que nadie paso.
        truth("and no % of it turned into a number",
              digitos_seguidos(salida) <= 4,
              "hay " + std::to_string(digitos_seguidos(salida)) +
              " digitos seguidos en el help");
        truth("and no % is doubled", texto.find("%%") == std::string::npos,
              "dos por ciento seguidos se imprimen tal cual: fputs no interpreta");
    }

    // Recien abierto el programa, ningun comando se muere.
    //
    // MEDIDO: `made` era un SEGFAULT limpio antes del primer solve. here() hace
    // *S.solver() y ahi todavia no hay solver, asi que el programa se cerraba de
    // golpe -- sin mensaje -- por escribir el segundo comando que lista el help.
    // Las demas vistas preguntaban primero; esa se quedo fuera.
    //
    // Asi que se escriben TODAS sin haber solveado: las que necesitan solucion
    // tienen que decirlo, y ninguna puede tumbar el programa.
    void a_fresh_start_does_not_die_on_any_command(Session& S) {
        std::string e;
        if (!truth("a fresh session for the console", spot(S, "AhKs2d5c7h", e), e)) return;
        static const char* const PIDEN[] = {
            "made", "hands", "combos", "grid", "freq", "br", "csv fuera.csv",
            "report", "runouts", "freqs", "expl"
        };
        std::string cmds;
        for (const char* c : PIDEN) cmds += std::string(c) + "\n";
        static const char* const NO_PIDEN[] = {
            "show", "tree", "lines", "locks", "saves", "estimate", "pwd", "help"
        };
        for (const char* c : NO_PIDEN) cmds += std::string(c) + "\n";

        std::string salida;
        if (!truth("and its output can be read back", console_says(S, cmds, salida),
                   "no se pudo redirigir stdout")) return;
        // Si algo de lo de arriba se muriera no llegariamos aqui: se cae el
        // proceso entero. Lo que se comprueba es que cada una CONTESTO.
        // Se cuentan LINEAS DE ERROR, no una frase: cada vista dice lo suyo
        // -- `br` habla de que no hay estrategia a la que responder -- y lo que
        // importa es que las once contesten en vez de tumbar el programa.
        int refusals = 0;
        size_t p = 0;
        const std::string aviso = std::string(1, char(10)) + "  ! ";
        while ((p = salida.find(aviso, p)) != std::string::npos) { ++refusals; p += aviso.size(); }
        const long long piden = static_cast<long long>(sizeof PIDEN / sizeof PIDEN[0]);
        same("every view that needs a solve says so instead of dying", refusals, piden);
        truth("and the ones that do not need it answer anyway",
              salida.find("BOARD") != std::string::npos &&
              salida.find("lines") != std::string::npos,
              "falta la salida de show o de lines");
        std::remove("fuera.csv");
    }

    // Un si o un no que no se entiende es un error, no un no.
    //
    // MEDIDO: `set iso yes` dejaba el agrupado de palos APAGADO -- 2796 nodos
    // instanciados se convertian en 9608, el doble de memoria y de tiempo -- por
    // la palabra mas natural que se puede escribir ahi. Tres sitios comparaban a
    // mano contra on, 1 y true, y daban por falso todo lo demas.
    void an_unrecognised_yes_no_is_an_error(Session& S) {
        bool v = false;
        static const char* const SI[] = { "on", "1", "true", "yes", "y", "si", "s",
                                          "ON", " On ", "enabled" };
        static const char* const NO[] = { "off", "0", "false", "no", "n", "none",
                                          "OFF", " Off ", "disabled" };
        static const char* const NADA[] = { "maybe", "", "2", "-1", "onoff", "ye", "sx" };
        int bien = 0;
        for (const char* t : SI)   if (parse_onoff(t, v) && v)  ++bien;
        for (const char* t : NO)   if (parse_onoff(t, v) && !v) ++bien;
        for (const char* t : NADA) if (!parse_onoff(t, v))      ++bien;
        same("a yes is a yes, a no is a no, and the rest is a refusal", bien,
             static_cast<long long>(sizeof SI / sizeof SI[0] +
                                    sizeof NO / sizeof NO[0] +
                                    sizeof NADA / sizeof NADA[0]));

        // Y por donde se escribe de verdad: la consola.
        std::string e;
        if (!truth("a session to type it into", spot(S, "AhKs2d5c7h", e), e)) return;
        const bool antes = cfg::ISO;
        cfg::ISO = true;
        std::string salida;
        if (!truth("and its output can be read back",
                   console_says(S, "set iso yes\n", salida), "no se pudo redirigir")) {
            cfg::ISO = antes;
            return;
        }
        truth("set iso yes leaves the suit collapsing ON", cfg::ISO,
              "un yes apago el agrupado de palos");

        console_says(S, "set iso maybe\n", salida);
        truth("and a word it does not know is refused out loud",
              salida.find("iso on|off") != std::string::npos,
              "no dijo nada: " + salida.substr(0, 120));
        truth("and it changes nothing", cfg::ISO,
              "un maybe apago el agrupado de palos");
        cfg::ISO = antes;
        std::string e2;
        spot(S, "AhKs2d5c7h", e2);
    }

    // Un cambio de arbol rechazado no se lleva por delante la solucion.
    //
    // MEDIDO: veinte iteraciones de river, y luego `set bets` con doce tamanos
    // donde caben ocho acciones. El cambio se rechaza -- correctamente -- y el
    // arbol se queda como estaba... y la solucion desaparecia: `hands` pasaba a
    // decir que no habia solucion y que solveases primero. El culpable era el
    // segundo rebuild del revert, que rehacia un arbol identico y de paso tiraba
    // el solver. En un flop de diez minutos eso son diez minutos por un campo
    // mal escrito que el programa NO llego a aplicar.
    void a_refused_setting_does_not_throw_the_solve_away(Session& S) {
        std::string e;
        if (!truth("a solved river to refuse things on", spot(S, "AhKs2d5c7h", e), e)) return;
        // El spot de prueba viene sin subidas, y sin subidas no hay escalera que
        // se pueda ir de las manos: la parte del stack de abajo no probaria nada.
        std::vector<Sizing> sube;
        { Sizing z; z.v = 3.0; z.xbet = true; sube.push_back(z); }
        if (!truth("with raises in it", S.set_sizings(false, ST_RIVER, sube, e), e)) return;
        S.solve(40, 0);
        if (!truth("and it solves", S.solved(), S.build_error())) return;
        const long long hechas = S.solver() ? S.solver()->iterations_done() : 0;
        truth("it really solved", S.solved() && hechas > 0, std::to_string(hechas));

        // Los tamanos son FRACCION del bote: 0,10 es un 10% de bote, no diez.
        // De 15 en 15 puntos y no de 10 en 10: dos tamanos que se diferencian en
        // menos de MERGE_PCT se juntan en uno, asi que una lista mas apretada
        // cabria de sobra en ocho acciones y no probaria nada. Y el mas grande,
        // 175% de un bote de 20, son 35 fichas: por debajo del umbral de all-in,
        // que con un stack de 100 esta en 67. Doce acciones de verdad.
        std::vector<Sizing> muchos;
        for (int k = 0; k < 12; ++k) { Sizing z; z.v = 0.10 + 0.15 * k; muchos.push_back(z); }
        std::string e2;
        truth("twelve sizings in one node are refused",
              !S.set_sizings(true, ST_RIVER, muchos, e2), "las acepto");
        truth("and the solve is still there", S.solved(), "se quedo sin solucion");
        same("with the same iterations behind it",
             S.solver() ? S.solver()->iterations_done() : 0, hechas);
        truth("and the tree is still the one it was",
              S.tc().bets[ST_RIVER][0].size() == 1,
              std::to_string(S.tc().bets[ST_RIVER][0].size()) + " tamanos en el arbol");

        // Y lo mismo con el stack, que es un global y se quedaba puesto.
        const double stack_antes = cfg::STACK;
        std::string e3;
        truth("a stack that makes the raises never end is refused",
              !S.set_stack(1e23, e3), "lo acepto");
        truth("and the stack is the one it was", cfg::STACK == stack_antes,
              std::to_string(cfg::STACK));
        truth("and the solve survived that too", S.solved(), "se quedo sin solucion");
    }

    // Una ventana estrecha no aplasta la solucion contra el borde.
    //
    // MEDIDO a 375px de ancho: el montaje se queda en sus 340px de minimo y a la
    // columna de la derecha le sobran veinte, asi que "resuelve para ver el
    // reparto" sale UNA PALABRA POR LINEA. A 760px ya entra bien, y 760 es
    // justo media pantalla de un portatil: el solver a un lado y Discord al
    // otro, que es como se va a usar el lunes.
    void a_narrow_window_does_not_squeeze_the_solution() {
        const std::string P = WEBUI_PAGE;
        const size_t i = P.find("@media (max-width:");
        if (!truth("the page has a rule for a narrow window", i != std::string::npos,
                   "no hay ningun @media en la hoja de estilo")) return;
        // El umbral, leido del propio texto.
        std::string num;
        for (size_t k = i + std::strlen("@media (max-width:");
             k < P.size() && P[k] != 'p' && P[k] != ')'; ++k) num += P[k];
        double px = 0.0;
        truth("and it says at what width", parse_double(trim(num), px), num);
        truth("and it covers half a laptop screen", px >= 600.0,
              "corta en " + num + "px, y media pantalla son unos 700");

        const size_t fin = P.find("\n  }", i);
        if (!truth("and the rule has a body", fin != std::string::npos)) return;
        const std::string regla = P.substr(i, fin - i);
        truth("the two columns stop sharing the width",
              regla.find("flex-wrap:wrap") != std::string::npos, regla);
        truth("and the setup column takes the whole of it",
              regla.find(".col-l") != std::string::npos &&
              regla.find("flex:1 1 100%") != std::string::npos, regla);
        truth("and its 52vw cap does not survive in there",
              regla.find("max-width:100%") != std::string::npos, regla);
    }

    // Que las barras se arrastran lo dice la pantalla, no solo el raton.
    //
    // Una celda con un numero no parece un control. El cursor cambia al pasar
    // por encima y hay un title, pero las dos cosas hay que descubrirlas
    // pasando el raton justo por ahi: quien abre esto el lunes ve una tabla.
    // Asi que el panel lo dice con letras, y de paso nombra el boton de
    // deshacer, que es lo que quita el miedo a probar.
    void the_bars_say_out_loud_that_they_are_bars() {
        const std::string P = WEBUI_PAGE;
        const size_t i = P.find("id=\"madePanel\"");
        if (!truth("the made-hand panel is there", i != std::string::npos)) return;
        const size_t fin = P.find("id=\"madeTable\"", i);
        if (!truth("with its note above the table", fin != std::string::npos)) return;
        const std::string panel = P.substr(i, fin - i);
        truth("and the note says the cells are bars",
              panel.find("es una barra") != std::string::npos, panel);
        truth("and that they are dragged",
              panel.find("strala") != std::string::npos, panel);
        truth("and it names the undo button",
              panel.find("\xE2\x86\xBA") != std::string::npos,
              "no nombra el simbolo del boton de deshacer");
        // Y lo que hace el raton sigue ahi: el texto de arriba miente si no.
        truth("the cell still shows it can be dragged",
              P.find("td.fambar{position:relative;cursor:ew-resize") != std::string::npos,
              "la celda ya no cambia el cursor");
    }

    // ¿Esto que se lee en la pantalla esta en español?
    //
    // Una palabra suelta de poker -- "flop", "AKs", "eq%" -- no lo esta, y no
    // hace falta traducirla. Se mira si lleva una palabra de las que solo salen
    // en español, o una letra con tilde. La puntuacion no separa: «las fuertes»
    // lleva un "las" dentro aunque el « este pegado.
    // ¿Esto es texto que alguien LEE en la pantalla?
    //
    // MEDIDO: la primera version preguntaba "¿esto parece español?" y buscaba
    // palabras como "de" o "que", asi que una palabra suelta -- Rangos,
    // Guardar, Nombre, Avanzado -- no le parecia español y se quedaba fuera.
    // Probado borrando la traduccion de "Rangos": la comprobacion pasaba en
    // verde con el titulo del panel sin traducir.
    //
    // Asi que la regla es la de verdad: TODO lo que se lee esta en la tabla,
    // aunque se escriba igual en los dos idiomas. Lo unico que se salta es lo
    // que no son palabras -- numeros, simbolos, una carta suelta.
    static bool se_lee(const std::string& s) {
        if (s.size() < 2) return false;
        // Una entidad no es una palabra: &nbsp; es un espacio y en la pantalla
        // no se lee nada. Fuera antes de mirar si hay letras.
        std::string t;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '&') {
                const size_t f = s.find(';', i);
                if (f != std::string::npos && f - i <= 8) { i = f; continue; }
            }
            t += s[i];
        }
        // Una letra de las normales. Los simbolos -- ·, −, ↺, ♥ -- no son texto
        // que nadie traduzca, y las palabras con tilde llevan letras normales
        // igual ("árbol" tiene r, b, o, l).
        for (unsigned char c : t)
            if (std::isalpha(c)) return true;
        return false;
    }

    static bool parece_espanol(const std::string& s) {
        if (s.size() < 4) return false;
        bool letra = false;
        for (unsigned char c : s)
            if (std::isalpha(c) || c >= 0x80) { letra = true; break; }
        if (!letra) return false;
        // Las vocales con tilde y la eñe empiezan todas por este byte en UTF-8.
        for (size_t i = 0; i + 1 < s.size(); ++i)
            if (static_cast<unsigned char>(s[i]) == 0xC3) return true;
        std::string plano = " ";
        for (unsigned char c : s)
            plano += (std::isalnum(c) ? static_cast<char>(std::tolower(c)) : ' ');
        plano += " ";
        static const char* const PAL[] = {
            " el ", " la ", " los ", " las ", " de ", " que ", " no ", " para ",
            " con ", " una ", " un ", " se ", " es ", " por ", " del ", " al ",
            " en ", " lo ", " si ", " sin ", " ya ", " hay ", " son ", " aqui "
        };
        for (const char* w : PAL)
            if (plano.find(w) != std::string::npos) return true;
        return false;
    }

    // ---------------------------------------------------------------------
    // La tabla de ingles: donde empieza, donde acaba, y sus claves.
    //
    // La pagina se traduce por el TEXTO EN ESPAÑOL, asi que la tabla es
    // clave-en-español -> valor-en-ingles. Las entradas pueden ocupar dos
    // lineas, asi que no se puede parear con una expresion regular ingenua: se
    // cogen todos los literales en orden y los pares son las claves.
    struct Tabla {
        size_t ini = std::string::npos, fin = std::string::npos;
        std::vector<std::string> claves, valores;
        bool ok() const { return ini != std::string::npos && !claves.empty(); }
    };

    // Los espacios de dentro se aprietan y los de los lados se quitan, que es
    // lo que hace el navegador antes de buscar en la tabla.
    static std::string apretado(const std::string& s) {
        std::string r;
        bool esp = false;
        for (char c : s) {
            const bool b = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
            if (b) { esp = true; continue; }
            if (esp && !r.empty()) r += ' ';
            esp = false;
            r += c;
        }
        return r;
    }

    static Tabla leer_tabla(const std::string& P) {
        Tabla t;
        t.ini = P.find("const EN = {");
        if (t.ini == std::string::npos) return t;
        t.fin = P.find("\n};", t.ini);
        if (t.fin == std::string::npos) return t;
        // Literales entre comillas simples, saltandose las lineas de comentario.
        size_t i = t.ini;
        int n = 0;
        while (i < t.fin) {
            // ¿linea de comentario?
            size_t sol = P.rfind('\n', i);
            sol = (sol == std::string::npos) ? 0 : sol + 1;
            const std::string li = trim(P.substr(sol, P.find('\n', i) - sol));
            if (li.rfind("//", 0) == 0) {
                i = P.find('\n', i);
                if (i == std::string::npos) break;
                ++i;
                continue;
            }
            const size_t a = P.find('\'', i);
            if (a == std::string::npos || a >= t.fin) break;
            size_t b = a + 1;
            while (b < t.fin && P[b] != '\'') b += (P[b] == '\\') ? 2 : 1;
            if (b >= t.fin) break;
            const std::string lit = P.substr(a + 1, b - a - 1);
            if (n % 2) t.valores.push_back(lit); else t.claves.push_back(apretado(lit));
            ++n;
            i = b + 1;
        }
        return t;
    }

    // Todo lo que se lee en la pantalla tiene su ingles.
    //
    // MEDIDO al montar el cambio de idioma: de 194 trozos de texto del HTML,
    // faltaba uno que no se veia a simple vista -- «las fuertes siempre
    // apuestan», dentro de un <i> en la ayuda del congelado -- y en ingles se
    // quedaba en español en medio de un parrafo traducido.
    //
    // Se mira el HTML fijo, que es el que se puede recorrer entero de una vez:
    // lo que hay entre un `>` y el `<` siguiente es un texto de la pantalla, y
    // si tiene pinta de español tiene que estar en la tabla.
    void every_spanish_line_on_screen_has_an_english_one() {
        const std::string P = WEBUI_PAGE;
        const Tabla T = leer_tabla(P);
        if (!truth("the page carries an English table", T.ok(),
                   "no se encontro `const EN = {`")) return;
        same("and the table is paired up", static_cast<long long>(T.claves.size()),
             static_cast<long long>(T.valores.size()));
        truth("and it is not a stub", T.claves.size() > 300,
              std::to_string(T.claves.size()) + " entradas");

        std::set<std::string> claves(T.claves.begin(), T.claves.end());
        const size_t b0 = P.find("<body>"), b1 = P.find("<script>");
        if (!truth("the page has a body to read", b0 != std::string::npos &&
                   b1 != std::string::npos && b1 > b0)) return;

        int sin_traducir = 0;
        std::string muestra;
        size_t i = b0;
        while (i < b1) {
            const size_t a = P.find('>', i);
            if (a == std::string::npos || a >= b1) break;
            const size_t c = P.find('<', a + 1);
            if (c == std::string::npos || c >= b1) break;
            const std::string txt = apretado(P.substr(a + 1, c - a - 1));
            i = c + 1;
            if (!se_lee(txt)) continue;
            if (claves.count(txt)) continue;
            ++sin_traducir;
            if (muestra.size() < 200) muestra += " | " + txt.substr(0, 70);
        }
        truth("every Spanish line of the page has an English one",
              sin_traducir == 0,
              std::to_string(sin_traducir) + " sin traducir:" + muestra);

        // Y los atributos que se leen, que tambien son pantalla.
        int attr = 0;
        std::string m2;
        static const char* const ATTRS[] = { "title=\"", "placeholder=\"" };
        for (const char* at : ATTRS) {
            size_t k = b0;
            while ((k = P.find(at, k)) != std::string::npos && k < b1) {
                k += std::strlen(at);
                const size_t f = P.find('"', k);
                if (f == std::string::npos) break;
                const std::string v = apretado(P.substr(k, f - k));
                k = f + 1;
                if (!se_lee(v) || claves.count(v)) continue;
                ++attr;
                if (m2.size() < 160) m2 += " | " + v.substr(0, 60);
            }
        }
        truth("and so does every tooltip", attr == 0,
              std::to_string(attr) + " sin traducir:" + m2);
    }

    // Una cadena que se escribe en español y se lee en inglés, y los dos botones
    // que la cambian.
    //
    // Sin esto, borrar el observador o el boton deja una tabla enorme que no
    // traduce nada: la tabla seguiria completa y las comprobaciones de arriba
    // seguirian en verde.
    void the_gear_changes_the_language() {
        const std::string P = WEBUI_PAGE;
        truth("there is a gear in the top bar",
              P.find("id=\"gearBtn\"") != std::string::npos &&
              P.find("onclick=\"toggleOpts()\"") != std::string::npos,
              "no hay boton de opciones");
        truth("and it opens a panel", P.find("id=\"optsPanel\"") != std::string::npos);
        truth("with the two languages in it",
              P.find("ponIdioma(&quot;es&quot;)") != std::string::npos &&
              P.find("ponIdioma(&quot;en&quot;)") != std::string::npos,
              "faltan los botones de idioma");
        truth("the choice survives a reload",
              P.find("localStorage.setItem('solverLang'") != std::string::npos &&
              P.find("localStorage.getItem('solverLang')") != std::string::npos,
              "el idioma no se guarda");
        // Lo que de verdad traduce: el observador. Sin el, todo lo que la pagina
        // pinta despues de arrancar se queda en español.
        // Que el texto "new MutationObserver" aparezca no dice nada: probado con
        // un `false &&` delante, la comprobacion pasaba y la pagina se quedaba a
        // medio traducir. Tiene que empezar la linea, y tiene que arrancar.
        truth("and everything that appears later is translated too",
              P.find("\nnew MutationObserver(ms=>{") != std::string::npos &&
              P.find("m.addedNodes.forEach(traduce)") != std::string::npos &&
              P.find("}).observe(document.body, {childList:true, subtree:true});")
                  != std::string::npos,
              "el observador no esta puesto o no llega a arrancar");
        truth("and changing it rebuilds what was already written",
              P.find("applyState(false);") != std::string::npos,
              "cambiar de idioma no rehace los textos con numeros dentro");
        // Se abre en ingles. La pagina esta escrita en español y se traduce al
        // arrancar; quien quiera el original lo tiene a un clic.
        truth("English is what it opens in",
              P.find("localStorage.getItem('solverLang') || 'en'") != std::string::npos,
              "el idioma de fabrica ya no es el ingles");
        truth("and it says so before the first paint",
              P.find("<html lang=\"en\">") != std::string::npos,
              "el html sigue declarandose en otro idioma");
    }

    // La tabla no guarda traducciones de textos que ya no existen.
    //
    // Es la otra mitad: la de arriba dice que no falta nada, y esta que no
    // sobra. Una clave que ya no aparece en la pagina es un texto que alguien
    // cambio sin tocar su traduccion, o sea una linea que en ingles se quedaria
    // en español sin que nadie se entere.
    void the_english_table_does_not_rot() {
        const std::string P = WEBUI_PAGE;
        const Tabla T = leer_tabla(P);
        if (!truth("the English table can be read", T.ok())) return;

        // La pagina, apretada y con las costuras de las cadenas partidas
        // quitadas: 'una linea '+ 'y la siguiente' es UN texto en la pantalla.
        std::string plano = apretado(P.substr(T.fin));
        plano += " " + apretado(P.substr(0, T.ini));
        for (;;) {
            const size_t k = plano.find("'+ '");
            if (k == std::string::npos) break;
            plano.erase(k, 4);
        }
        int huerfanas = 0;
        std::string muestra;
        for (const std::string& k : T.claves) {
            if (k.empty()) continue;
            if (plano.find(k) != std::string::npos) continue;
            ++huerfanas;
            if (muestra.size() < 200) muestra += " | " + k.substr(0, 60);
        }
        truth("no translation is left over from a text that no longer exists",
              huerfanas == 0,
              std::to_string(huerfanas) + " huerfanas:" + muestra);
    }

    // Lo que PIDE traduccion y no la tiene.
    //
    // El observador traduce lo que hay en el marcado. Lo que arma el codigo con
    // numeros dentro no pasa por ahi: lo envuelve t(). Y t() no se queja -- si
    // la clave no esta en la tabla devuelve el castellano tal cual, la pantalla
    // se queda a medias en ingles y solo se ve mirandola.
    //
    // MEDIDO al poner el ingles de salida: ocho textos se veian en castellano en
    // una interfaz que llevaba semanas dandose por traducida -- "buena" en la
    // pildora de convergencia, "ver cuales" en la barra de locks, "-- ninguno --"
    // en las tres listas de guardado. Nadie los habia visto porque el castellano
    // era el idioma de fabrica y en castellano estaban bien.
    //
    // La regla es mecanica: si alguien escribe t('algo'), es que ese algo se
    // tiene que traducir. Si no esta en la tabla, falta.
    // Una clave con un espacio pegado no traduce NUNCA.
    //
    // Los dos caminos que traducen -- t() y el observador -- aprietan el texto
    // y le quitan los espacios de los lados ANTES de buscar en la tabla, y
    // luego devuelven los espacios que tenia el original. Asi que la clave se
    // guarda limpia. Una escrita como 'showdown: tu ' no la encuentra nadie:
    // no da error, devuelve el castellano, y la pantalla se queda a medias.
    //
    // MEDIDO: habia cinco, tres de ellas del dialogo de scripts y llevaban
    // semanas ahi. Dos comprobaciones miraban ya esta tabla y ninguna las vio,
    // porque las dos aprietan los dos lados antes de comparar y el fallo
    // consiste justo en eso.
    void no_key_has_a_space_stuck_to_it() {
        const std::string P = WEBUI_PAGE;
        const Tabla T = leer_tabla(P);
        if (!truth("the English table can be read for the spaces", T.ok())) return;
        int sucias = 0;
        std::string cuales;
        for (const std::string& k : T.claves) {
            if (k.empty()) continue;
            const char a = k.front(), b = k.back();
            const bool esp = (a == ' ' || a == '\t' || b == ' ' || b == '\t');
            if (!esp) continue;
            ++sucias;
            if (cuales.size() < 200) cuales += " | [" + k + "]";
        }
        truth("no key carries a space on either side", sucias == 0,
              std::to_string(sucias) + " que no traducen nunca:" + cuales);
    }

    void nothing_asks_to_be_translated_and_is_not() {
        const std::string P = WEBUI_PAGE;
        const Tabla T = leer_tabla(P);
        if (!truth("the English table can be read for this too", T.ok())) return;
        std::vector<std::string> claves = T.claves;
        std::sort(claves.begin(), claves.end());

        int sin_clave = 0, mirados = 0;
        std::string muestra;
        for (size_t i = 0; (i = P.find("t('", i)) != std::string::npos; ) {
            // Dentro de la tabla no: alli t( no es una llamada.
            if (i > T.ini && i < T.fin) { i += 3; continue; }
            // `t(` de verdad, y no el final de otra palabra -- `format('...')`.
            const char antes = i ? P[i - 1] : ' ';
            const bool palabra = (antes >= 'a' && antes <= 'z') ||
                                 (antes >= 'A' && antes <= 'Z') ||
                                 (antes >= '0' && antes <= '9') ||
                                 antes == '_' || antes == '.';
            if (palabra) { i += 3; continue; }
            const size_t a = i + 3;
            const size_t b = P.find('\'', a);
            if (b == std::string::npos) break;
            // Solo los literales sueltos: t('x') entero. Un t('x'+y) no es una
            // clave y no se puede comprobar aqui.
            if (P.compare(b, 2, "')") != 0) { i = b; continue; }
            const std::string k = apretado(P.substr(a, b - a));
            i = b + 2;
            if (k.empty()) continue;
            ++mirados;
            if (std::binary_search(claves.begin(), claves.end(), k)) continue;
            ++sin_clave;
            if (muestra.size() < 240) muestra += " | " + k.substr(0, 60);
        }
        truth("there is a pile of text asking to be translated", mirados > 100,
              std::to_string(mirados) + " llamadas a t() con literal");
        truth("and every one of them has an English line",
              sin_clave == 0,
              std::to_string(sin_clave) + " sin traducir:" + muestra);
    }

    // En la calle donde empieza el solve no hay donk, y OOP abre con sus bets.
    //
    // Un donk es liderar CONTRA quien fue agresivo en la calle anterior, y en la
    // calle de salida no hubo agresor: la apuesta de OOP es su apertura, y sale
    // de sus tamaños de bet. Es lo mismo que hace la referencia.
    //
    // Hubo una nota en la pantalla explicando esto donde iria el campo. Se quito
    // a peticion del usuario -- la pantalla se estaba llenando de parrafos -- asi
    // que lo que queda por comprobar es lo unico que de verdad importa: que el
    // arbol lo haga bien.
    void the_street_the_solve_starts_on_says_why_it_has_no_donk(Session& S) {
        const std::string P = WEBUI_PAGE;
        // El campo de donk solo existe donde un donk puede existir.
        const size_t d = P.find("const donkRow");
        if (!truth("the donk row is still built by hand", d != std::string::npos)) return;
        const std::string trozo = P.substr(d, 700);
        truth("and only where a donk can exist",
              trozo.find("s.donkable") != std::string::npos &&
              trozo.find("Tamaños de donk") != std::string::npos,
              "el campo de donk ya no depende de que la calle pueda tenerlo");
        truth("and it does not explain itself with a paragraph",
              P.find("Aquí no hay") == std::string::npos,
              "volvio la nota que se quito");

        // Y lo que de verdad importa: que OOP PUEDA liderar la calle de salida.
        // Si esto se rompiera, la nota estaria mintiendo.
        std::string e;
        if (!truth("a flop spot to look at", spot(S, "Ah9h4h", e), e)) return;
        const BetTree& bt = S.tree().ctx[0].tree;
        const Node& raiz = bt.nodes[static_cast<size_t>(bt.root)];
        truth("OOP acts first on the flop", raiz.type == NT_DECISION && raiz.player == 0,
              "la raiz del flop no es una decision de OOP");
        int apuestas = 0;
        for (int a = 0; a < raiz.num_actions; ++a)
            if (bt.act(raiz, a).kind == AK_BET) ++apuestas;
        truth("and it can lead with its bet sizings", apuestas > 0,
              "OOP no tiene ninguna apuesta en la calle de salida");
    }

    // Todo lo de Avanzado vive en el engranaje.
    //
    // Estaba dentro de un <details> al fondo del panel de montar el arbol, que
    // es donde nadie lo encontraba: el limite de memoria se nombra en el
    // mensaje que sale cuando un arbol no cabe, y habia que explicar por donde
    // se llegaba. Ahora esta donde se buscan las opciones.
    //
    // Lo que se comprueba es que los campos SIGUEN SIENDO LOS MISMOS: applyTree
    // y el panel los leen por id, asi que un campo que se quede fuera del panel
    // no da error, simplemente deja de poder tocarse.
    void the_gear_holds_every_advanced_option() {
        const std::string P = WEBUI_PAGE;
        const size_t p0 = P.find("id=\"optsPanel\"");
        if (!truth("there is an options panel", p0 != std::string::npos)) return;
        const size_t p1 = P.find("</span>\n</header>", p0);
        if (!truth("and it closes", p1 != std::string::npos)) return;
        const std::string panel = P.substr(p0, p1 - p0);

        static const char* const CAMPOS[] = {
            "id=\"maxMem\"", "id=\"threads\"", "id=\"iters\"",
            "id=\"allinPct\"", "id=\"rakePct\"", "id=\"rakeCap\""
        };
        for (const char* c : CAMPOS) {
            const bool dentro = panel.find(c) != std::string::npos;
            truth(std::string("the gear holds ") + c, dentro,
                  "ese campo no esta en el panel de opciones");
            // Y en un solo sitio: dos campos con el mismo id y el navegador lee
            // el primero, que puede no ser el que se ve.
            size_t n = 0, k = 0;
            while ((k = P.find(c, k)) != std::string::npos) { ++n; k += 4; }
            same("and only there", static_cast<long long>(n), 1);
        }
        truth("and a button that applies them",
              panel.find("onclick=\"aplicaOpciones()\"") != std::string::npos,
              "el panel no tiene boton de aplicar");
        // El plegado del montaje esconde la columna entera; si Avanzado siguiera
        // ahi, plegar esconderia el limite de memoria.
        truth("nothing advanced is left in the tree panel",
              P.find("Avanzado</summary>") == std::string::npos,
              "sigue habiendo un <details> de Avanzado en el montaje");
    }

    // Poner el numero que ya estaba puesto no cuesta la solucion.
    //
    // MEDIDO en cuanto el panel tuvo boton de Aplicar: resolver (135
    // iteraciones), abrir el engranaje, cambiar SOLO el limite de memoria y dar
    // a Aplicar -- y la solucion desaparecia. El panel manda todos sus campos
    // juntos, y tres de ellos rehacian el arbol o invalidaban el solve aunque
    // llevaran el mismo valor que ya tenian: el rake, el umbral de all-in y los
    // hilos.
    //
    // Es el mismo principio que ya tenian los cambios rechazados: lo que no
    // cambia nada no puede costar diez minutos de solve.
    void putting_back_the_same_numbers_costs_nothing(Session& S) {
        std::string e;
        if (!truth("a solved river to re-apply things on", spot(S, "AhKs2d5c7h", e), e)) return;
        S.solve(40, 0);
        if (!truth("and it solved", S.solved(), S.build_error())) return;
        const long long hechas = S.solver() ? S.solver()->iterations_done() : 0;

        // 1. Por la sesion, uno a uno.
        std::string e2;
        truth("the same rake is accepted", S.set_rake(cfg::RAKE_PCT, cfg::RAKE_CAP, e2), e2);
        truth("and the solve is still there", S.solved(), "el rake tiro la solucion");
        truth("the same all-in threshold is accepted",
              S.set_allin_thresh(cfg::ALLIN_THRESH, e2), e2);
        truth("and the solve is still there too", S.solved(), "el umbral tiro la solucion");
        truth("the same pot is accepted", S.set_pot(cfg::POT0, e2), e2);
        truth("the same stack is accepted", S.set_stack(cfg::STACK, e2), e2);
        same("and none of them cost an iteration",
             S.solver() ? S.solver()->iterations_done() : 0, hechas);

        // 2. Y por donde se manda de verdad: la peticion entera del panel.
        WebUI W(S, 0, false);
        char q[320];
        std::snprintf(q, sizeof q,
                      "maxmem=%.2f&threads=%d&iters=%d&allinPct=%.6f&rakePct=%.6f&rakeCap=%.2f",
                      cfg::MAX_MEM_GB, cfg::THREADS, S.iters(),
                      cfg::ALLIN_THRESH, cfg::RAKE_PCT, cfg::RAKE_CAP);
        const std::string r = W.config_for_check(q);
        truth("the whole panel can be applied", r.find("\"ok\":true") != std::string::npos,
              r.substr(0, 160));
        truth("and pressing Apply without touching anything keeps the solve",
              S.solved(), "dar a Aplicar sin tocar nada tiro la solucion");
        same("with every iteration still behind it",
             S.solver() ? S.solver()->iterations_done() : 0, hechas);

        // 3. Y que un cambio DE VERDAD sigue contando: sin esto, un guardia que
        //    no dejara cambiar nada pasaria las dos de arriba.
        const double rake_antes = cfg::RAKE_PCT;
        W.config_for_check("rakePct=0.05");
        truth("but a rake that really changes still rebuilds", !S.solved(),
              "cambiar el rake ya no rehace el arbol");
        must_be("and the rake is the new one", cfg::RAKE_PCT, 0.05, 1e-9);
        std::string e3;
        S.set_rake(rake_antes, cfg::RAKE_CAP, e3);
    }

    // Un rango guardado son LOS DOS.
    //
    // Un rango suelto no dice nada: "BU vs BB 25bb" es un par, y guardar un lado
    // obliga a acordarse de cual era el otro -- y a guardarlo con otro nombre, y
    // a cargarlo en el lado correcto. Se guardan juntos y se cargan juntos.
    //
    // Lo que hay que cuidar al cambiar esto es lo de siempre con los ficheros:
    // los que ya estan guardados llevan un rango solo y ninguna etiqueta. Se
    // siguen leyendo, y aqui se comprueba con uno escrito a mano.
    void a_saved_range_is_the_pair(Session& S) {
        std::string e;
        if (!truth("a spot with two ranges", spot(S, "AhKs2d5c7h", e), e)) return;
        const std::string oop0 = S.range_spec(0), ip0 = S.range_spec(1);
        truth("and both sides have one", !oop0.empty() && !ip0.empty(),
              "OOP '" + oop0 + "' IP '" + ip0 + "'");

        // Los ficheros de prueba, fuera antes de empezar: uno que sobreviva a una
        // pasada anterior -- se quedo uno de una mutacion -- hace que esto mida el
        // fichero viejo en vez de lo que hizo el programa ahora.
        const std::string nombre = "chk-par";
        std::error_code ec;
        std::filesystem::remove(Session::range_path(nombre), ec);
        std::filesystem::remove(Session::range_path("chk-medio"), ec);
        std::filesystem::remove(Session::range_path("chk-viejo"), ec);
        if (!truth("the pair saves", S.save_range(nombre, e), e)) return;

        // Se cambian los dos, para que volver a lo guardado se note en ambos.
        std::string e2;
        truth("both sides can be changed", S.set_range(0, "AA", e2) &&
              S.set_range(1, "KK", e2), e2);
        if (!truth("and the pair loads back", S.load_range(nombre, 0, e2), e2)) return;
        truth("OOP is the one that was saved", S.range_spec(0) == oop0,
              "quedo '" + S.range_spec(0) + "'");
        truth("and IP is too -- not the one on screen", S.range_spec(1) == ip0,
              "quedo '" + S.range_spec(1) + "'");

        // Media pareja no es una pareja.
        truth("clearing IP is allowed", S.set_range(1, "", e2), e2);
        truth("but then there is nothing to save",
              !S.save_range("chk-medio", e2), "guardo media pareja");
        truth("and it says which side is missing", e2.find("IP") != std::string::npos, e2);
        truth("and it did not write the file",
              !std::filesystem::exists(Session::range_path("chk-medio")),
              "escribio el fichero igual");

        // Y los ficheros de antes: un rango suelto, sin etiqueta.
        {
            std::FILE* f = std::fopen(Session::range_path("chk-viejo").c_str(), "wb");
            if (truth("an old one-sided file can be written", f != nullptr)) {
                const std::string t = "AA,KK,QQ\n";
                std::fwrite(t.data(), 1, t.size(), f);
                std::fclose(f);
                truth("and it still loads, into the side you ask for",
                      S.load_range("chk-viejo", 1, e2), e2);
                truth("that side has it", S.range_spec(1) == "AA,KK,QQ",
                      "quedo '" + S.range_spec(1) + "'");
                truth("and the other side was left alone", S.range_spec(0) == oop0,
                      "quedo '" + S.range_spec(0) + "'");
            }
        }

        std::string e3;
        S.delete_range(nombre, e3);
        S.delete_range("chk-viejo", e3);
        std::filesystem::remove(Session::range_path("chk-medio"), ec);
        truth("and the check cleans up its files",
              !std::filesystem::exists(Session::range_path(nombre)) &&
              !std::filesystem::exists(Session::range_path("chk-viejo")) &&
              !std::filesystem::exists(Session::range_path("chk-medio")),
              "quedaron ficheros de prueba en saves/ranges");
    }

    // Lo que se quita, se quita entero.
    //
    // El panel de frecuencia de linea se fue porque no se usaba, y de eso lo
    // peligroso no es quitarlo: es dejarse una referencia. El HUD leia
    // `lineFreq`, que lo llenaba el panel; con el panel fuera y la linea dentro,
    // eso es un ReferenceError en mitad de pintar el nodo -- y lo que se ve es
    // media pantalla vacia, sin nada que explique por que.
    //
    // El numero sigue existiendo: `freqs` en la consola.
    void taking_the_line_panel_out_left_nothing_behind() {
        const std::string P = WEBUI_PAGE;
        static const char* const RASTROS[] = {
            "lineFreq", "verFreqs", "renderFreqs", "freqTable", "freqBtn",
            "freqPanel", "/api/freqs"
        };
        for (const char* r : RASTROS)
            truth(std::string("the page has no ") + r + " left",
                  P.find(r) == std::string::npos,
                  "quedo una referencia al panel que ya no existe");
        // Lo que SI tiene que seguir: la consola.
        truth("the console still has `freqs`",
              std::string(CONSOLE_HELP).find("freqs [n|all|file]") != std::string::npos,
              "se llevo por delante el comando de la consola");
    }

    // Las notas largas: una linea, y el resto en un dialogo cuando se pide.
    //
    // MEDIDO contandolas: la pantalla llevaba quince parrafos de ayuda de entre
    // 130 y 400 caracteres alrededor de los campos. Explicar esta bien, pero asi
    // no se lee ninguno: el campo que buscas esta debajo de tres explicaciones
    // que ya te sabes.
    //
    // La primera version desplegaba el resto al pasar el raton por encima. Se
    // quito: eso se dispara solo -- mueves el raton para llegar a un campo, el
    // parrafo se abre y lo de debajo se mueve. Ahora el resumen lleva un enlace
    // azul con la manita, que es como se ve que algo se pincha, y el detalle sale
    // en un dialogo.
    void the_long_notes_are_one_line_until_you_ask() {
        const std::string P = WEBUI_PAGE;
        truth("the long half of a note stays hidden",
              P.find(".note.nx .det{display:none}") != std::string::npos,
              "el detalle de las notas ya no esta escondido");
        truth("and nothing opens it just by pointing at it",
              P.find(".note.nx:hover .det") == std::string::npos,
              "volvio el desplegado al pasar el raton");
        // Una `i` en un circulo azul, con la manita. Es la senal que usa todo el
        // mundo para "aqui hay mas informacion", ocupa nada y no hay que
        // traducirla. Antes ponia "por que" con letras, y eso queda de aficionado.
        truth("the link looks like a link",
              P.find("border:1px solid var(--accent);" ) != std::string::npos &&
              P.find("border-radius:50%;color:var(--accent);font-size:10px;font-weight:700;")
                  != std::string::npos,
              "el icono de la nota ya no es un circulo azul");
        truth("and it says it can be clicked",
              P.find("font-style:italic;cursor:pointer;vertical-align:1px;user-select:none}")
                  != std::string::npos,
              "el icono perdio la manita");
        truth("and it opens the dialog",
              P.find("onclick=\"abreNota(this)\"") != std::string::npos &&
              P.find("id=\"noteDlg\"") != std::string::npos,
              "el enlace no abre ningun dialogo");
        truth("which shows that note and not another",
              P.find("noteBody').innerHTML=det.innerHTML") != std::string::npos,
              "el dialogo no se llena con el detalle de la nota que se pincho");
        truth("and it closes",
              P.find("onclick=\"cierraNota()\"") != std::string::npos,
              "el dialogo de la nota no se puede cerrar");

        // Y la regla de verdad: NINGUNA nota larga se queda sin plegar. Contar
        // cuantas hay plegadas no vale -- probado quitandole el plegado a una: con
        // diez de once la comprobacion pasaba y el parrafo volvia a la pantalla.
        int largas = 0;
        std::string cuales;
        const size_t b0 = P.find("<body>"), b1 = P.find("<script>");
        size_t k = b0;
        while (k < b1) {
            const size_t a = P.find("<div class=\"note", k);
            if (a == std::string::npos || a >= b1) break;
            const size_t cierra = P.find(">", a);
            const size_t fin = P.find("</div>", a);
            if (cierra == std::string::npos || fin == std::string::npos) break;
            const std::string clase = P.substr(a, cierra - a);
            // El texto que se lee, sin etiquetas.
            std::string txt;
            bool dentro = false;
            for (size_t i = cierra + 1; i < fin; ++i) {
                if (P[i] == '<') dentro = true;
                else if (P[i] == '>') dentro = false;
                else if (!dentro) txt += P[i];
            }
            k = fin + 6;
            if (apretado(txt).size() < 130) continue;      // corta, se queda entera
            // Una nota puede decir que es el manual y quedarse entera: la del
            // dialogo de tamaños se abre porque la has pedido, y plegar ahi seria
            // esconder lo unico que ese dialogo tiene dentro.
            if (clase.find("manual") != std::string::npos) continue;
            if (clase.find("note nx") != std::string::npos) continue;
            ++largas;
            if (cuales.size() < 120) cuales += " | " + apretado(txt).substr(0, 50);
        }
        truth("and no long note is left unfolded", largas == 0,
              std::to_string(largas) + " sin plegar:" + cuales);
    }

    // La barra de familia parece un control, y dice en que estado esta.
    //
    // Era un relleno translucido detras de un numero, dentro de una celda de
    // tabla. MEDIDO mirandolo: no parece un control, parece una celda con el
    // fondo sucio -- y lo que hay que hacer con ella es arrastrarla. Ahora es un
    // medidor: carril, relleno del color de la accion, tirador donde se agarra,
    // y el numero dentro.
    //
    // Y lo que mas confundia: una familia bloqueada Y APLICADA y una bloqueada
    // SIN aplicar se veian igual. Son dos cosas distintas -- la segunda le falta
    // un solve -- y ahora van en dos colores, el segundo el mismo ambar del boton
    // de Resolver cuando tiene locks pendientes.
    void a_family_bar_looks_like_a_control() {
        const std::string P = WEBUI_PAGE;
        // Las cuatro piezas del medidor.
        static const char* const PIEZAS[] = {
            ".ftrack{", ".ftrack .ffill{", ".ftrack .fedge{", ".ftrack .fnum{"
        };
        for (const char* z : PIEZAS)
            truth(std::string("the meter has ") + z, P.find(z) != std::string::npos,
                  "falta una pieza de la barra");
        truth("and the cell builds all four",
              P.find("class=\"ftrack\"") != std::string::npos &&
              P.find("class=\"ffill\"") != std::string::npos &&
              P.find("class=\"fedge\"") != std::string::npos &&
              P.find("class=\"fnum\"") != std::string::npos,
              "la celda no monta el medidor entero");
        truth("the grip shows up where you grab it",
              P.find("td.fambar:hover .fedge{opacity:.9}") != std::string::npos,
              "el tirador ya no aparece al pasar por encima");

        // Dos estados, dos colores.
        truth("a lock that is already in the solve is one colour",
              P.find("td.fambar.lk .ftrack{box-shadow:inset 3px 0 0 var(--accent)}")
                  != std::string::npos, "el lock aplicado perdio su color");
        truth("and one that still needs a solve is another",
              P.find("td.fambar.pend .ftrack{box-shadow:inset 3px 0 0 var(--warn")
                  != std::string::npos, "el lock sin aplicar no se distingue");
        truth("and the cell says which one it is",
              P.find("(ped?' pend':'')") != std::string::npos,
              "la celda ya no marca lo que esta sin aplicar");

        // El numero que va con el raton mientras se arrastra.
        truth("there is a badge for the drag",
              P.find("#fambadge{") != std::string::npos &&
              P.find("id=\"fambadge\"") != std::string::npos,
              "no hay chapa que siga al raton");
        truth("and it starts hidden",
              P.find("#fambadge{position:fixed;z-index:80;display:none") != std::string::npos,
              "la chapa se ve sin arrastrar nada");
        // display='' devuelve a la regla de la hoja, que es display:none -- o sea
        // que la chapa no se veia nunca. Probado: asi salio.
        truth("and the drag really shows it",
              P.find("b.style.display='block'") != std::string::npos,
              "la chapa se ensena con display='' y la hoja dice none");

        // Lo que se arrastra se mide sobre el carril, no sobre la celda: la celda
        // tiene aire a los lados y con el dentro el 100% cae antes del final.
        truth("and the drag is measured over the track",
              P.find("const t2=el.querySelector('.ftrack');") != std::string::npos,
              "el arrastre vuelve a medirse sobre la celda entera");
    }

    // El peso de cada familia se ve de un vistazo, no leyendo trece numeros.
    void the_family_table_is_read_at_a_glance() {
        const std::string P = WEBUI_PAGE;
        truth("the weight column is a bar", P.find(".wtrack{") != std::string::npos &&
              P.find("class=\"wtrack\"") != std::string::npos,
              "el % de rango volvio a ser solo un numero");
        // En proporcion a la familia mas grande: repartido sobre 100, una familia
        // del 3% y otra del 6% se ven igual de vacias y la barra no dice nada.
        truth("and it is scaled to the biggest family",
              P.find("const maxW=node.made.reduce") != std::string::npos &&
              P.find("100*g.w/maxW") != std::string::npos,
              "la barra de peso ya no se escala a la mayor");
        truth("the equity carries its own colour",
              P.find("background:hsl('+(1.2*Math.max(0,Math.min(100,g.eq)))") != std::string::npos,
              "la equity volvio a ser un numero suelto");
        truth("and the header says which colour is which action",
              P.find(".achip{") != std::string::npos &&
              P.find("class=\"achip\"") != std::string::npos,
              "la cabecera no dice de que color es cada accion");
        truth("and the numbers line up",
              P.find("#madeTable td{font-variant-numeric:tabular-nums}") != std::string::npos,
              "los numeros de la tabla ya no son de ancho fijo");
    }

    // La rejilla ensena el numero cuando lo que pinta es un numero.
    //
    // El color dice "esta verde", pero la pregunta que sigue siempre es CUANTO,
    // y estaba a un hover de distancia, casilla a casilla. la referencia lo escribe
    // dentro de la casilla en los modos de equity y de EV, y tiene razon.
    //
    // En estrategia no: ahi la casilla ya lleva el reparto pintado a lo ancho y
    // un numero encima seria ruido sobre lo que ya se ve.
    void the_grid_writes_the_number_it_is_painting() {
        const std::string P = WEBUI_PAGE;
        truth("the cell has a place for the number",
              P.find("\n  .grid.strat .cell .val{") != std::string::npos,
              "no hay sitio para el numero en la casilla");
        truth("and the grid writes it",
              P.find("<div class=\"val\">") != std::string::npos,
              "la rejilla no escribe el numero");
        truth("in equity and in EV, and only there",
              P.find("(cell && mode!==\'strat\')") != std::string::npos,
              "el numero sale tambien en el modo estrategia, encima del reparto");
        truth("equity with one decimal, EV with two",
              P.find("mode===\'eq\' ? cell.eq.toFixed(1) : cell.ev.toFixed(2)")
                  != std::string::npos,
              "los decimales del numero de la casilla cambiaron");
        // La letra de la casilla es NEGRA, como en la referencia. Sobre un rojo oscuro no
        // se lee, y con el numero dentro eso pasa de detalle a problema.
        truth("and the colour never gets too dark to read on",
              P.find("(38+cell.eq*0.14)") != std::string::npos &&
              P.find("(38+tb*14)") != std::string::npos,
              "la escala de color perdio el suelo de luz");
    }

    // El peso de la mano, al pasar el raton por la rejilla de montaje.
    //
    // El azul dice que la mano esta dentro y mas o menos cuanto, pero "esto es
    // un 60 o un 75?" no se lee en un tono de azul, y es justo el numero que hay
    // que saber para retocar un rango. Va en el title de la casilla.
    //
    // Esto mira el fuente de la pagina, no el navegador: aqui no hay quien
    // ejecute el JavaScript. Lo que se exige es que las tres piezas esten --
    // nombre de la mano, numero, y que decir cuando no esta dentro --, porque
    // quitar cualquiera de las tres deja un title que no sirve.
    void the_grid_says_what_a_hand_weighs() {
        const std::string P = WEBUI_PAGE;
        const size_t at = P.find("cells[k].title=");
        if (!truth("the setup grid puts the weight on the cell",
                   at != std::string::npos,
                   "la rejilla de montaje no escribe title ninguno")) return;
        const std::string linea = P.substr(at, 240);
        truth("saying which hand it is",
              linea.find("title=clsName(") != std::string::npos,
              "el title ya no empieza por el nombre de la mano");
        truth("and what it weighs, in per cent",
              linea.find("num(100*Math.min(w,1)") != std::string::npos &&
              linea.find("+'%'") != std::string::npos,
              "el title ya no lleva el peso en porcentaje");
        truth("and saying so when the hand is not in",
              linea.find("t('fuera del rango')") != std::string::npos,
              "una mano fuera del rango no dice nada al pasar el raton");
        truth("in both languages",
              P.find("'fuera del rango':'") != std::string::npos,
              "el texto de fuera del rango no esta en la tabla de ingles");
    }

    // El motor contesta en el idioma que le piden.
    //
    // La interfaz esta entera en español o entera en ingles, pero los errores
    // salian del motor siempre en ingles: una pantalla en español que de pronto
    // contesta "branching factor 12 exceeds MAX_ACTIONS 8" esta a medias, y
    // justo cuando alguien necesita entender algo.
    //
    // El idioma lo dice la pagina en cada peticion. Aqui se comprueba lo unico
    // que importa: que el mismo error salga en los dos idiomas y que no sea el
    // mismo texto.
    void the_engine_answers_in_the_language_it_was_asked(Session& S) {
        std::string e;
        if (!truth("a spot to get errors from", spot(S, "AhKs2d5c7h", e), e)) return;

        struct Caso {
            const char* que;
            const char* es;      // una palabra que solo sale en español
            const char* en;      // y otra que solo sale en ingles
        };
        // Cada uno se provoca de verdad, no se lee de una tabla.
        const bool antes = msg::EN;

        std::string es_rango, en_rango, es_carta, en_carta, es_arbol, en_arbol,
                    es_pot, en_pot;
        for (int paso = 0; paso < 2; ++paso) {
            msg::EN = (paso == 1);
            std::string m;

            std::string e1;
            S.set_range(0, "ZZ", e1);
            (paso ? en_rango : es_rango) = e1;

            std::string e2;
            S.set_board("Xx9h4h", e2);
            (paso ? en_carta : es_carta) = e2;

            // Doce tamaños donde caben ocho acciones.
            std::vector<Sizing> muchos;
            for (int k = 0; k < 12; ++k) { Sizing z; z.v = 0.10 + 0.15 * k; muchos.push_back(z); }
            std::string e3;
            S.set_sizings(true, ST_RIVER, muchos, e3);
            (paso ? en_arbol : es_arbol) = e3;

            std::string e4;
            S.set_pot(-1.0, e4);
            (paso ? en_pot : es_pot) = e4;
        }
        msg::EN = antes;

        truth("a bad range token answers in Spanish",
              es_rango.find("esto no es una mano") != std::string::npos, es_rango);
        truth("and in English", en_rango.find("unrecognised range token") != std::string::npos,
              en_rango);
        truth("a bad card answers in Spanish",
              es_carta.find("eso no es una carta") != std::string::npos, es_carta);
        truth("and in English", en_carta.find("not a card") != std::string::npos, en_carta);
        truth("a tree that does not fit answers in Spanish",
              es_arbol.find("acciones en un nodo") != std::string::npos, es_arbol);
        truth("and in English", en_arbol.find("actions at one node") != std::string::npos,
              en_arbol);
        truth("a bad pot answers in Spanish",
              es_pot.find("el bote tiene que ser") != std::string::npos, es_pot);
        truth("and in English", en_pot.find("pot must be") != std::string::npos, en_pot);

        // Y que no sean el mismo texto, que es la forma de que esto pase en
        // verde sin traducir nada.
        truth("and the two are not the same text",
              es_rango != en_rango && es_carta != en_carta &&
              es_arbol != en_arbol && es_pot != en_pot,
              "algun mensaje sale igual en los dos idiomas");

        // Y una peticion que no dice nada se contesta en ingles, que es como
        // sale de fabrica el programa. El español hay que pedirlo, igual que en
        // la pagina: se publica, y lo que se lee sin haber elegido nada tiene
        // que entenderlo cualquiera.
        {
            Params vacio, en, es;
            en.parse("lang=en");
            es.parse("lang=es");
            WebUI::language_from(vacio, vacio);
            const bool sin_pedir = msg::EN;
            WebUI::language_from(vacio, en);
            const bool pidiendo_en = msg::EN;
            WebUI::language_from(vacio, es);
            const bool pidiendo_es = msg::EN;
            msg::EN = antes;
            truth("a request that says nothing gets English", sin_pedir,
                  "sin pedir idioma contesta en español");
            // Y el programa entero arranca igual: lo que se lee antes de la
            // primera peticion -- un spot que no carga al abrir, por ejemplo --
            // tambien sale en ingles.
            truth("and so does the program before anyone asks", arranca_en_ingles_,
                  "msg::EN arranca en false");
            truth("and one that asks for English gets English", pidiendo_en,
                  "pedir ingles no cambia nada");
            truth("and one that asks for Spanish gets Spanish", !pidiendo_es,
                  "pedir español no cambia nada");
        }

        std::string e5;
        spot(S, "AhKs2d5c7h", e5);
    }

    // Y el cable: la pagina lo dice en cada peticion, el servidor lo lee.
    void the_page_says_which_language_it_wants() {
        const std::string P = WEBUI_PAGE;
        truth("every POST carries the language",
              P.find("Object.assign({}, form, {lang:idioma})") != std::string::npos,
              "las peticiones con formulario ya no llevan el idioma");
        truth("and every GET too",
              P.find("'lang='+idioma") != std::string::npos,
              "las peticiones sin formulario ya no llevan el idioma");
    }

    // El tope de tiempo para de verdad, y dice que fue el reloj.
    //
    // Es la pieza que hace que una lista de boards quepa en una noche: sin el,
    // el primero que no alcanza la precision pedida se come las horas de los
    // otros diecinueve.
    //
    // MEDIDO: con un tope de 20s en un flop grande paraba a los 35. El bucle
    // miraba la hora cada 64 iteraciones, y en ese arbol una iteracion cuesta un
    // cuarto de segundo: diecisiete segundos entre miradas. Ahora el trozo se
    // ajusta al reloj y no pasa de dos segundos.
    void a_time_limit_actually_stops_the_solve(Session&) {
        // El spot de FABRICA, no uno de juguete: en un arbol pequeño una
        // iteracion cuesta milisegundos y el tope se cumple solo. Lo que hay
        // que medir es lo que pasa cuando una iteracion cuesta un cuarto de
        // segundo, que es donde el trozo fijo de 64 se pasaba doce segundos.
        Session S;
        std::string e;
        if (!truth("the built-in spot, which is a slow one",
                   S.load_config_text(DEFAULT_SPOT, e), e)) return;
        // Sin objetivo de precision: lo unico que puede parar esto es el reloj.
        S.set_acc_stop(false);
        // Lo que cuesta MONTAR el solver se mide aparte y se descuenta: solve()
        // lo monta cada vez, y en este spot son medio giga. Lo que se juzga es el
        // tiempo del calculo, que es lo que el tope manda.
        const auto b0 = std::chrono::steady_clock::now();
        S.solve(1, 0);
        const double montar = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - b0).count();
        // Lo que cuesta UNA iteracion en ESTA maquina. Sin esto, el margen de
        // "paro cerca del tope" es un numero fijo en segundos, y un numero fijo
        // en segundos dice cosas distintas en una maquina que va cuatro veces
        // mas lenta.
        //
        // MEDIDO: en la maquina del autor una iteracion de este spot cuesta
        // 0,35 s; en el runner de la CI, varias veces mas. Con el margen fijo
        // de 4 segundos la comprobacion fallaba alli y pasaba aqui, que es la
        // peor clase de comprobacion que hay.
        const auto i0 = std::chrono::steady_clock::now();
        S.solve(4, 0);
        const double por_vuelta = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - i0).count() / 4.0;

        S.set_timeout_secs(1.0);
        const auto t0 = std::chrono::steady_clock::now();
        S.solve(300, 0);
        const double secs = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count() - montar;
        S.set_timeout_secs(0.0);

        truth("it stopped on the clock", S.timeout_reached(),
              "no dice que parase por el tope de tiempo");
        // Un segundo de tope, mas lo que tarde la tanda que estuviera corriendo.
        // Con un tope puesto la primera tanda son cuatro vueltas, asi que lo
        // peor que puede pasar es pasarse cuatro vueltas DE ESTA MAQUINA, y un
        // segundo mas de margen por el ruido.
        //
        // Con tandas fijas de 64 esto se iba a cinco segundos y medio en este
        // spot, que es lo que la comprobacion viene a distinguir: parar cerca
        // del tope, y no mucho despues. Y 300 vueltas seguidas serian
        // trescientas veces `por_vuelta`, asi que el margen sigue siendo
        // estrecho por donde importa.
        const double margen = 2.0 + 4.0 * por_vuelta;
        truth("and it stopped near it, not long after",
              secs >= 0.8 && secs < margen,
              "tardo " + fmt_num(secs) + "s de calculo con un tope de 1 y un " +
              "margen de " + fmt_num(margen) + "s (" + fmt_num(por_vuelta) +
              "s por vuelta)");
        truth("and what it did solve is usable",
              S.solved() && S.solver() && S.solver()->iterations_done() > 0,
              "no dejo nada resuelto");
        // Y lo dice en el resumen, que es donde se lee por la mañana.
        const std::string linea = solve_summary(50, 100000, secs, 8, false, true);
        truth("and the summary says the clock is why",
              linea.find("stopped on the time limit") != std::string::npos, linea);

    }

    // Dos flops que son el mismo con los palos cambiados de nombre.
    //
    // Ah9h4h y As9s4s tienen la misma solucion: si se renombran los palos, uno
    // es el otro. En una lista para dejar de noche, resolver los dos es pagar
    // dos veces por la misma respuesta -- y en un flop eso son horas.
    void two_flops_that_are_the_same_board_are_one() {
        struct Par { const char* a; const char* b; bool iguales; };
        static const Par CASOS[] = {
            { "Ah9h4h", "As9s4s", true  },   // monocolor, otro palo
            { "Ah9h4c", "As9s4d", true  },   // dos de un palo y uno de otro
            { "Ah9c4d", "As9h4c", true  },   // arcoiris
            { "9h9s4d", "9c9d4h", true  },   // pareja en el board
            { "Ah9h4h", "Ah9h4c", false },   // color contra dos palos
            { "Ah9h4c", "Ah9c4c", false },   // que dos van juntos importa
            { "Ah9h4h", "Ah9h5h", false },   // otro rango es otro board
        };
        int bien = 0;
        std::string fallos;
        for (const Par& p : CASOS) {
            std::vector<int> a, b;
            std::string e1, e2;
            if (!parse_board(p.a, a, e1) || !parse_board(p.b, b, e2)) continue;
            const bool iguales = flop_canon(a[0], a[1], a[2]) == flop_canon(b[0], b[1], b[2]);
            if (iguales == p.iguales) ++bien;
            else fallos += std::string(" ") + p.a + "/" + p.b;
        }
        same("a flop is the same board as another only when it really is",
             bien, static_cast<long long>(sizeof CASOS / sizeof CASOS[0]));
        truth("and no pair is judged wrong", fallos.empty(), fallos);
    }

    // El script de varios boards: lo escribe el servidor y se puede correr.
    //
    // Lo que se comprueba es que el texto sea EJECUTABLE: que cargue el spot,
    // que ponga los tres topes, y que por cada board haga las tres ordenes. Un
    // script que se genera bonito y no corre es peor que no tenerlo, porque eso
    // se descubre a la mañana siguiente.
    void the_many_board_script_is_a_script_that_runs(Session& S) {
        std::string e;
        if (!truth("a spot to script", spot(S, "AhKs2d5c7h", e), e)) return;
        WebUI W(S, 0, false);

        const std::string q =
            "name=chk-script&boards=Ah9h4h%0AKd7c2s%0A%23 un comentario%0A&"
            "acc=0.5&timeout=600&iters=4000&save=1&pattern={board}";
        const std::string r = W.script_for_check(q);
        if (!truth("the script is generated", r.find("\"ok\":true") != std::string::npos,
                   r.substr(0, 200))) return;

        // Lo que tiene que decir, en orden.
        static const char* const LINEAS[] = {
            "load config chk-script",
            "set accuracy 0.5",
            "set stopacc on",
            "set timeout 600",
            "set iters 4000",
            "board Ah9h4h",
            "save tree Ah9h4h",
            "board Kd7c2s",
            "save tree Kd7c2s"
        };
        size_t desde = 0;
        int en_orden = 0;
        for (const char* l : LINEAS) {
            const size_t k = r.find(l, desde);
            if (k == std::string::npos) continue;
            ++en_orden;
            desde = k;
        }
        same("and it says everything it has to, in order", en_orden,
             static_cast<long long>(sizeof LINEAS / sizeof LINEAS[0]));
        // Un comentario de la lista no es un board.
        truth("a commented line is not a board",
              r.find("board # un comentario") == std::string::npos, "colo el comentario");

        // Y el spot queda guardado, que es de donde lo saca el script.
        truth("and the spot it loads is on disk",
              std::filesystem::exists(Session::save_path(false, "chk-script")),
              "no guardo la config que el script carga");

        // Por la mañana el log son mil lineas: lo primero que se busca es por
        // cual iba y a que hora. Cada board lleva su marca delante, y la lista
        // termina diciendo que termino -- si no esta esa linea, es que se murio.
        truth("each board is announced in the log",
              r.find("echo === 1/2  Ah9h4h") != std::string::npos &&
              r.find("echo === 2/2  Kd7c2s") != std::string::npos,
              "el script no marca por que board va");
        truth("and the list says when it is done",
              r.find("echo === terminada la lista") != std::string::npos ||
              r.find("echo === finished the list") != std::string::npos,
              "el script no dice que termino");
        // Que el help lo mencione no vale: probado quitando la linea que lo
        // despacha, el help seguia diciendo que existe y la comprobacion pasaba.
        // Hay que ESCRIBIR uno y mirar lo que sale.
        {
            Session S2;
            std::string salida;
            if (truth("the console can be asked to echo",
                      console_says(S2, "echo por aqui iba\n", salida),
                      "no se pudo redirigir")) {
                truth("and it writes the line",
                      salida.find("por aqui iba") != std::string::npos,
                      "la consola no escribio la linea: " + salida.substr(0, 120));
                // Con la hora del reloj: [HH:MM:SS]
                bool hora = false;
                for (size_t i = 0; i + 9 < salida.size(); ++i)
                    if (salida[i] == '[' && std::isdigit(static_cast<unsigned char>(salida[i+1])) &&
                        std::isdigit(static_cast<unsigned char>(salida[i+2])) && salida[i+3] == ':' &&
                        std::isdigit(static_cast<unsigned char>(salida[i+4])) &&
                        std::isdigit(static_cast<unsigned char>(salida[i+5])) && salida[i+6] == ':' &&
                        salida[i+9] == ']') hora = true;
                truth("with the time of day in front of it", hora,
                      "sin hora no sirve para saber cuando se murio: " + salida.substr(0, 120));
            }
        }

        // Un nombre que no distingue los boards guardaria los veinte arboles uno
        // encima de otro, y eso no se ve hasta que por la mañana hay UNO.
        const std::string choca = W.script_for_check(
            "name=chk-script&boards=Ah9h4h%0AKd7c2s&acc=1&timeout=0&iters=100&"
            "save=1&pattern=mi-arbol");
        truth("a name that does not tell the boards apart is refused",
              choca.find("\"ok\":false") != std::string::npos, choca.substr(0, 160));
        truth("and it says what to put in it",
              choca.find("{board}") != std::string::npos, choca.substr(0, 200));
        // Pero con UN solo board no hay nada que distinguir: ahi no molesta.
        const std::string uno = W.script_for_check(
            "name=chk-script&boards=Ah9h4h&acc=1&timeout=0&iters=100&save=1&pattern=mi-arbol");
        truth("with a single board that name is fine",
              uno.find("\"ok\":true") != std::string::npos, uno.substr(0, 160));

        // Un board mal escrito se dice AHORA, no a las tres de la mañana.
        const std::string mal = W.script_for_check(
            "name=chk-script&boards=Ah9h4h%0AXx9h4h&acc=1&timeout=0&iters=100&save=0");
        truth("a bad board stops the whole thing before it starts",
              mal.find("\"ok\":false") != std::string::npos, mal.substr(0, 160));
        truth("and it says which line",
              mal.find("2") != std::string::npos, mal.substr(0, 200));

        std::error_code ec;
        std::filesystem::remove(Session::save_path(false, "chk-script"), ec);
        truth("and the check cleans up after itself",
              !std::filesystem::exists(Session::save_path(false, "chk-script")),
              "quedo la config de prueba");
    }

    // Los rangos que vienen dentro del programa.
    //
    // Quien se baja el .exe se encuentra la lista vacia, y montar un rango de
    // 25bb a mano es media hora antes de poder resolver nada. Vienen siete, del
    // autor: LJ, HJ, CO y BU contra BB y contra BU.
    //
    // Lo delicado no es meterlos: es que no molesten. Se escriben SOLO si no hay
    // ninguno, asi que los tuyos no se tocan y uno que borres no vuelve al
    // arrancar -- que es lo que convertiria un regalo en una molestia.
    void the_ranges_that_come_in_the_box() {
        truth("there are ranges in the box",
              sizeof DEFAULT_RANGES / sizeof DEFAULT_RANGES[0] >= 5,
              "vienen menos de cinco");

        // Cada uno tiene que ser LOS DOS lados y tiene que cargar de verdad.
        Session S;
        std::string e;
        if (!truth("a board to load them onto", S.set_board("Ah9h4h", e), e)) return;
        int bien = 0, malos = 0;
        std::string cuales;
        for (const RangoDeFabrica& r : DEFAULT_RANGES) {
            const std::string t(r.texto);
            const bool pareja = t.rfind("OOP ", 0) == 0 &&
                                t.find("\nIP ") != std::string::npos;
            if (!pareja) { ++malos; cuales += std::string(" ") + r.nombre; continue; }
            // Y que el texto se pueda leer: un rango de fabrica que no parsea es
            // peor que ninguno, porque falla en la primera pantalla.
            const size_t nl = t.find('\n');
            std::vector<double> w;
            std::string e2;
            if (!parse_range(trim(t.substr(4, nl - 4)), S.deal(), w, e2)) {
                ++malos; cuales += std::string(" ") + r.nombre + "(" + e2 + ")";
                continue;
            }
            int vivos = 0;
            for (double x : w) if (x > 0.0) ++vivos;
            if (vivos > 0) ++bien;
            else { ++malos; cuales += std::string(" ") + r.nombre + "(vacio)"; }
        }
        same("and every one of them is a pair that loads",
             bien, static_cast<long long>(sizeof DEFAULT_RANGES / sizeof DEFAULT_RANGES[0]));
        truth("and none is broken", malos == 0, cuales);

        // LA REGLA: carpeta vacia si, carpeta con algo dentro no.
        //
        // Con la carpeta de verdad delante esto no se puede comprobar entero --
        // el caso que importa es "el usuario borro uno de los siete", y para
        // montarlo habria que borrarle un rango. Por eso la regla esta sola en
        // una funcion que no toca el disco, y se le pregunta con la lista que
        // haga falta.
        truth("an empty folder gets them",
              Session::should_install_defaults(std::vector<std::string>()),
              "en una carpeta vacia no se instalaria nada");
        std::vector<std::string> seis;
        for (const RangoDeFabrica& r : DEFAULT_RANGES) seis.push_back(r.nombre);
        if (!seis.empty()) seis.pop_back();      // uno borrado a mano
        truth("and one deleted by hand never comes back",
              !Session::should_install_defaults(seis),
              "con seis guardados se rellenaria el septimo");
        truth("nor does a folder with a range of your own get touched",
              !Session::should_install_defaults(
                  std::vector<std::string>(1, "mi-rango")),
              "se instalarian encima de los tuyos");

        // Y de verdad, contra el disco: si ya hay rangos no escribe nada, y la
        // segunda vez no escribe nunca.
        Session S2;
        const bool habia = !S2.list_ranges().empty();
        const int puso = S2.install_default_ranges();
        if (habia) {
            truth("with ranges already saved it writes nothing", puso == 0,
                  "escribio " + std::to_string(puso) + " encima de los que habia");
        } else {
            truth("on an empty folder it writes them", puso > 0,
                  "no escribio ninguno en una carpeta vacia");
            truth("and then the list is not empty", !S2.list_ranges().empty(),
                  "los escribio y la lista sigue vacia");
        }
        Session S3;
        truth("and a second time it never writes again",
              S3.install_default_ranges() == 0,
              "uno que borres volveria a aparecer al arrancar");
    }


    // ---------------------------------------------------------------------
    //  EL ENTRENADOR
    //
    //  Jugar el arbol contra la solucion. Lo que hay que vigilar no es que
    //  "funcione": es que lo que te ensena sea LA SOLUCION y no un numero
    //  parecido, y que lo que te puntua sea de verdad lo que te costo.
    // ---------------------------------------------------------------------

    // Una mano repartida es una mano posible, y el consejo es el del solver.
    void playing_the_tree_is_playing_the_solution(Session& S) {
        std::string e;
        if (!truth("a spot to play", spot(S, "Ah9h4hKd2s", e), e)) return;
        S.solve(120, 0);
        if (!truth("solved before playing", S.solved())) return;

        Trainer T;
        T.set_side(0);
        T.set_advice(true);
        if (!truth("a hand is dealt", T.new_hand(S, e), e)) return;

        const Deal& D = S.deal();
        // 1. Las cuatro cartas son cuatro cartas distintas, y ninguna esta en
        //    el board. Repartir una que ya esta en la mesa es el fallo que
        //    convierte esto en un juguete roto.
        int c[4] = { T.hero()[0], T.hero()[1], T.villain()[0], T.villain()[1] };
        int repes = 0, enboard = 0;
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) if (c[i] == c[j]) ++repes;
            for (int b : T.board()) if (c[i] == b) ++enboard;
        }
        same("the four cards are four different cards", repes, 0);
        same("and none of them is on the board", enboard, 0);

        // 2. Tu mano sale de TU rango.
        const int mi = D.combo_at[static_cast<size_t>(c[0]) * 52 + static_cast<size_t>(c[1])] >= 0
                     ? D.combo_at[static_cast<size_t>(c[0]) * 52 + static_cast<size_t>(c[1])]
                     : D.combo_at[static_cast<size_t>(c[1]) * 52 + static_cast<size_t>(c[0])];
        if (truth("your hand is a combo of this board", mi >= 0)) {
            truth("and it is in your range",
                  S.range(0)[static_cast<size_t>(mi)] > 0.0,
                  "te reparte una mano que no llevas");
        }

        // 3. Lo que te ofrece son las acciones DEL NODO, no otras.
        const RoundCtx& rc = S.tree().ctx[0];
        const Node& n0 = rc.tree.nodes[static_cast<size_t>(rc.tree.root)];
        same("it offers exactly the actions of the node",
             static_cast<long long>(T.codes().size()), n0.num_actions);
        int distintas = 0;
        for (int a = 0; a < n0.num_actions && a < static_cast<int>(T.codes().size()); ++a)
            if (T.codes()[static_cast<size_t>(a)] != rc.tree.act(n0, a).code) ++distintas;
        same("with the same codes", distintas, 0);

        // 4. Y EL CONSEJO ES LA SOLUCION. No "algo parecido": los mismos
        //    numeros que pinta la rejilla para esa mano en ese nodo. Si esto
        //    se calculara aparte, el entrenador podria estar corrigiendote con
        //    una estrategia que no es la que tienes en pantalla.
        long long inst = 0;
        int perm = D.identity();
        std::vector<int> sinslots;
        D.locate(sinslots, inst, perm);
        NodeStats N = gather(*S.solver(), 0, rc.tree.root, inst, perm, false);
        if (truth("the node can be read", N.ok)) {
            double peor_f = 0.0, peor_ev = 0.0;
            for (int a = 0; a < n0.num_actions; ++a) {
                peor_f  = std::max(peor_f,  std::fabs(N.freq(mi, a) - T.freqs()[static_cast<size_t>(a)]));
                peor_ev = std::max(peor_ev, std::fabs(N.ev_action(mi, a) - T.evs()[static_cast<size_t>(a)]));
            }
            truth("the advice is the solution, frequency by frequency",
                  peor_f < 1e-9, "se desvia " + fmt_sci(peor_f));
            truth("and EV by EV", peor_ev < 1e-9, "se desvia " + fmt_sci(peor_ev));
        }

        // 5. Jugar SIEMPRE lo mejor no cuesta nada, y jugar siempre lo peor si.
        //    Las dos: una sola de ellas la pasaria un marcador que siempre
        //    devuelve cero, y ese es justo el fallo que deja el entrenador sin
        //    servir para nada.
        for (int paso = 0; paso < 40 && T.phase() == Trainer::YOURS; ++paso) {
            int mejor = 0;
            for (size_t a = 1; a < T.evs().size(); ++a)
                if (T.evs()[a] > T.evs()[static_cast<size_t>(mejor)]) mejor = static_cast<int>(a);
            if (!T.act(S, T.codes()[static_cast<size_t>(mejor)], e)) break;
        }
        truth("playing the best action every time costs nothing",
              T.lost() < 1e-9, "perdio " + fmt_sci(T.lost()) + " jugando lo mejor");
        truth("and the hand did end", T.ended(), "la mano no termino");
        truth("and it counts as played", T.hands_played() >= 1);

        Trainer T2;
        T2.set_side(0);
        T2.set_advice(true);
        double peor = 0.0;
        int manos = 0;
        for (int intento = 0; intento < 12 && peor <= 0.0; ++intento) {
            if (!T2.new_hand(S, e)) break;
            ++manos;
            for (int paso = 0; paso < 40 && T2.phase() == Trainer::YOURS; ++paso) {
                int malo = 0;
                for (size_t a = 1; a < T2.evs().size(); ++a)
                    if (T2.evs()[a] < T2.evs()[static_cast<size_t>(malo)]) malo = static_cast<int>(a);
                if (!T2.act(S, T2.codes()[static_cast<size_t>(malo)], e)) break;
            }
            peor = T2.lost();
        }
        truth("and playing the worst one costs money",
              peor > 1e-6,
              "jugando lo peor en " + std::to_string(manos) + " manos no perdio nada");

        // 7. Y EL BOT juega su estrategia, no lo que le da la gana. Lo menos
        //    probable que ha hecho en todas esas manos tiene que ser algo que
        //    su mano hace: en cuanto tomara una accion de frecuencia cero,
        //    esto seria cero. Un bot que juega al azar se ve igual de bien en
        //    pantalla que uno bueno, y esta es la unica forma de distinguirlos
        //    sin jugarle diez mil manos.

        // 6. La misma semilla, la misma mano. Es lo que hace que "repetir esta
        //    mano" sea repetirla y no otra parecida.
        Trainer T3;
        T3.set_side(0);
        if (!truth("a hand to repeat", T3.new_hand(S, e), e)) return;
        const int h0 = T3.hero()[0], h1 = T3.hero()[1];
        const int v0 = T3.villain()[0], v1 = T3.villain()[1];
        const std::vector<int> b0 = T3.board();
        if (!truth("and it can be repeated", T3.repeat_hand(S, e), e)) return;
        truth("the same seed deals the same hand",
              T3.hero()[0] == h0 && T3.hero()[1] == h1 &&
              T3.villain()[0] == v0 && T3.villain()[1] == v1 &&
              T3.board() == b0,
              "repetir la mano reparte otra cosa");
    }

    // Empezar donde tu digas, y que eso signifique algo.
    void the_hand_can_start_anywhere_in_the_tree(Session& S) {
        std::string e;
        if (!truth("a flop to start inside", spot(S, "Ah9h4h", e), e)) return;
        S.solve(80, 0);
        if (!truth("solved before playing it", S.solved())) return;

        // El nodo de despues de una apuesta: ahi hay dinero puesto y hay que
        // pagar. Se busca en el arbol en vez de escribir un numero a mano,
        // que se queda viejo en cuanto cambian los tamanos.
        const RoundCtx& rc = S.tree().ctx[0];
        const Node& raiz = rc.tree.nodes[static_cast<size_t>(rc.tree.root)];
        int tras_bet = -1;
        for (int a = 0; a < raiz.num_actions; ++a)
            if (rc.tree.act(raiz, a).kind == AK_BET) tras_bet = rc.tree.child(raiz, a);
        if (!truth("there is a node after a bet", tras_bet >= 0)) return;
        const Node& nb = rc.tree.nodes[static_cast<size_t>(tras_bet)];

        Trainer T;
        T.set_side(nb.player);
        T.set_start(0, tras_bet, std::vector<int>());
        if (!truth("a hand starting there", T.new_hand(S, e), e)) return;
        must_be("and the pot is that node's pot", T.pot(), nb.pot, 1e-9);
        truth("and there is something to call", T.to_call() > 0.0,
              "empieza frente a una apuesta y dice que no hay nada que pagar");
        truth("and it says where it started", !T.starts_at_root());

        // Y la mano que te reparte ahi es una mano que LLEGA ahi. Repartir del
        // rango de partida colaria manos que en ese nodo no existen, y estarias
        // practicando un spot que no se juega: es el fallo que no se ve, porque
        // por pantalla todo parece correcto.
        long long inst2 = 0;
        int perm2 = S.deal().identity();
        std::vector<int> nada;
        S.deal().locate(nada, inst2, perm2);
        NodeStats NB = gather(*S.solver(), 0, tras_bet, inst2, perm2, false);
        if (truth("that node can be read", NB.ok)) {
            // Contra los PESOS, no contra las manos que salieron.
            //
            // Probado repartiendo del rango de partida: doce manos seguidas
            // salieron todas con alcance en el nodo y la comprobacion pasaba
            // en verde. Claro que pasaban: el alcance es una probabilidad, casi
            // ninguna mano vale exactamente cero, y mirando lo que sale no se
            // distingue una cosa de la otra hasta que llevas cientos de manos.
            // Lo que se puede comprobar de una vez es la regla: los pesos con
            // los que reparte SON el alcance de ese nodo.
            const std::vector<double>& mios  = T.deal_weights(true);
            const std::vector<double>& suyos = T.deal_weights(false);
            const bool soy_yo = (nb.player == T.side());
            const std::vector<double>& esperados_mios =
                soy_yo ? NB.v.own_reach : NB.v.opp_reach;
            const std::vector<double>& esperados_suyos =
                soy_yo ? NB.v.opp_reach : NB.v.own_reach;
            double peor = 0.0;
            if (mios.size() == esperados_mios.size() &&
                suyos.size() == esperados_suyos.size()) {
                for (size_t h = 0; h < mios.size(); ++h) {
                    peor = std::max(peor, std::fabs(mios[h] - esperados_mios[h]));
                    peor = std::max(peor, std::fabs(suyos[h] - esperados_suyos[h]));
                }
            } else {
                peor = 1.0;
            }
            truth("and it deals with the reach of that node, not the starting range",
                  peor < 1e-12, "los pesos se desvian " + fmt_sci(peor));
        }

        // Y en una calle de mas abajo: se reparte una carta que no estaba.
        int turno = -1;
        for (size_t i = 0; i < S.tree().ctx.size(); ++i)
            if (S.tree().ctx[i].street == 1) { turno = static_cast<int>(i); break; }
        if (truth("there is a turn context", turno >= 0)) {
            Trainer T2;
            T2.set_side(0);
            T2.set_start(turno, -1, std::vector<int>());
            if (truth("a hand starting on the turn", T2.new_hand(S, e), e)) {
                same("the board has the turn card on it",
                     static_cast<long long>(T2.board().size()), 4);
            }
            // Y ninguna carta que salga puede estar en la mano de nadie.
            //
            // UNA mano no vale: con cuatro cartas tapadas de cuarenta y nueve,
            // repartir sin mirarlas choca una vez de cada doce, y probado
            // quitando el filtro la comprobacion pasaba en verde. Cuarenta
            // manos son ochenta cartas repartidas, y ahi no se escapa.
            Trainer T3;
            T3.set_side(0);
            int choques = 0, cartas = 0, jugadas = 0;
            for (int k = 0; k < 40; ++k) {
                if (!T3.new_hand(S, e)) break;
                for (int paso = 0; paso < 30 && T3.phase() == Trainer::YOURS; ++paso)
                    if (!T3.act(S, T3.codes()[0], e)) break;
                ++jugadas;
                for (size_t i = 3; i < T3.board().size(); ++i) {
                    ++cartas;
                    const int x = T3.board()[i];
                    if (x == T3.hero()[0] || x == T3.hero()[1] ||
                        x == T3.villain()[0] || x == T3.villain()[1]) ++choques;
                }
            }
            truth("enough hands to see it", jugadas >= 20 && cartas >= 20,
                  std::to_string(jugadas) + " manos, " + std::to_string(cartas) + " cartas");
            same("and no card ever comes that somebody is holding", choques, 0);

            // Y EL BOT juega su estrategia, no lo que le da la gana.
            //
            // Lo menos probable que ha hecho en esas cuarenta manos tiene que
            // ser algo que su mano hace: en cuanto tomara una accion de
            // frecuencia cero, esto seria cero. Un bot que reparte uniforme se
            // ve igual de bien en pantalla que uno bueno, y esta es la unica
            // forma de distinguirlos sin jugarle diez mil manos.
            //
            // Cuarenta manos y no una: probado con el bot al azar, en UNA mano
            // puede no tocar ninguna accion imposible y la comprobacion pasaba.
            // Y EL BOT juega su estrategia, no lo que le da la gana.
            //
            // Lo que se mira es la frecuencia MEDIA de lo que ha jugado, segun
            // su propia estrategia. Un bot que tira el dado con sus
            // frecuencias saca la suma de los cuadrados, que es alta porque
            // casi todas las manos juegan casi siempre lo mismo; uno que
            // reparte uniforme saca 1/acciones, que es lo mas bajo que se
            // puede sacar.
            //
            // MEDIDO aqui: 0,74 de media en 77 decisiones de cuarenta manos.
            // Con dos y tres acciones por nodo, el uniforme daria 0,45. El
            // suelo se pone en 0,60: lejos de los dos numeros.
            //
            // El primer intento fue "nunca juega una accion de frecuencia
            // cero", y no vale: con ochenta iteraciones casi ninguna accion
            // vale exactamente cero, asi que el bot al azar pasaba en verde.
            truth("and the bot plays its own strategy, not just anything",
                  T3.villain_freq_mean() > 0.60 && T3.villain_decisions() >= 30,
                  "media " + fmt_sci(T3.villain_freq_mean()) + " en " +
                  std::to_string(T3.villain_decisions()) + " decisiones (" +
                  std::to_string(T3.villain_no_strategy()) + " nodos sin estrategia)");
        }
    }

    // El consejo apagado NO VIAJA.
    //
    // Esconderlo con CSS no lo esconde: se abre el inspector, se mira la
    // respuesta y ahi esta la frecuencia que decias no querer ver. Si esta
    // apagado, no sale del motor.
    void the_advice_does_not_travel_when_it_is_off(Session& S) {
        std::string e;
        if (!truth("a spot for the advice", spot(S, "Ah9h4hKd2s", e), e)) return;
        S.solve(60, 0);
        if (!truth("solved for the advice", S.solved())) return;

        WebUI W(S, 0, false);
        const std::string con = W.train_for_check("op=new&side=0&advice=on");
        truth("with the advice on it travels",
              con.find("\"freq\"") != std::string::npos,
              "con el consejo encendido no manda las frecuencias");
        const std::string sin = W.train_for_check("op=new&side=0&advice=off");
        truth("and with it off there is not a frequency in the answer",
              sin.find("\"freq\"") == std::string::npos,
              "el consejo apagado viaja igual y se ve en el inspector");
        truth("and not an EV either",
              sin.find("\"ev\"") == std::string::npos,
              "los EV viajan con el consejo apagado");
        // Y las cartas del rival, tampoco, hasta que se ensenan.
        truth("nor does the villain's hand before the showdown",
              sin.find("\"villain\":null") != std::string::npos,
              "las cartas del rival viajan antes de tiempo");

        // Y una mano a medias con el arbol cambiado debajo no revienta.
        //
        // Cambiar el board tira la solucion y reconstruye. El entrenador se
        // quedo guardando un contexto y un nodo del arbol ANTERIOR, y seguir
        // jugando seria leer indices de un arbol que ya no existe: no un aviso
        // feo, memoria de otro sitio. Se corta y se dice.
        std::string e2;
        truth("a hand in progress", W.train_for_check("op=new&side=0").find("\"on\":true")
              != std::string::npos);
        truth("the board changes underneath", S.set_board("2c7d9s", e2), e2);
        const std::string tras = W.train_for_check("op=act&action=X");
        truth("and acting on it is refused, not crashed",
              tras.find("\"ok\":false") != std::string::npos,
              "sigue jugando sobre un arbol que ya no esta: " + tras.substr(0, 120));
    }

    void the_built_in_spot_loads() {
        Session S;
        std::string e;
        if (!truth("the built-in spot loads", S.load_config_text(DEFAULT_SPOT, e), e)) return;
        truth("and it leaves both ranges filled",
              S.live_combos(0) > 0 && S.live_combos(1) > 0,
              "OOP " + std::to_string(S.live_combos(0)) +
              ", IP " + std::to_string(S.live_combos(1)));
        truth("and a board to play it on", S.deal().board.size() >= 3,
              std::to_string(S.deal().board.size()) + " cartas");
        // Listo para solve, que es de lo que se trata: quien abre el programa
        // tiene que poder darle al boton sin escribir nada.
        truth("and it is ready to solve without typing anything", S.ranges_ready(),
              S.not_ready_reason());

        // Y sobre todo: que quepa. Abrir el programa cuesta 19 MB -- los
        // buferes no se reservan hasta que se solvea -- asi que lo que diga
        // este numero se paga entero en el PRIMER clic en Solve, que es
        // justamente cuando alguien esta estrenando el programa.
        //
        // La config completa de la que sale este spot son 230.417 nodos y 2,91
        // GB, y cinco minutos de converger. Recortando los tamanos a uno por
        // calle se queda en 43.271 nodos y 0,53 GB, sigue teniendo subidas y
        // converge en minuto y medio.
        //
        // El tope es generoso a proposito: no persigue un numero, impide que
        // alguien devuelva aqui una config grande sin darse cuenta.
        const Session::MemUse mu = S.memory_use();
        truth("and it fits in memory without anyone asking for it", mu.total < 1.0,
              "el spot de arranque ocupa " + std::to_string(mu.total) + " GB");
    }

    void two_solvers_cannot_share_a_port(Session& S) {
        std::string e;
        if (!truth("port spot builds", spot(S, "Ah9h4h", e), e)) return;

        WebUI A(S, 0, false);
        A.set_quiet(true);
        std::thread ta([&A]() { A.run(); });
        int port = 0;
        for (int i = 0; i < 400 && port == 0; ++i) {
            port = A.bound_port();
            if (!port) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (!truth("the first one comes up", port != 0, "no llego a escuchar")) {
            A.shutdown(); ta.join(); return;
        }

        // El segundo, en el puerto que acaba de coger el primero.
        //
        // En un hilo y con espera acotada, y esto NO es precaucion de sobra: la
        // primera version llamaba a B.run() de frente, dando por hecho que si no
        // puede atarse vuelve enseguida. Con el fallo puesto a mano SI se ata,
        // entra en el bucle de aceptar y no vuelve nunca -- o sea que la
        // comprobacion no fallaba, se COLGABA, y la suite entera con ella. Una
        // que se cuelga es tan inutil como una que no mira nada, y peor de
        // encontrar.
        WebUI B(S, port, false);
        B.set_quiet(true);
        std::atomic<int> salida(-999);
        std::thread tb([&B, &salida]() { salida.store(B.run()); });
        for (int i = 0; i < 400 && salida.load() == -999 && B.bound_port() == 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        const int suyo = B.bound_port();
        truth("and the second one refuses that port", suyo == 0,
              "el segundo se ato al mismo puerto y se quedo escuchando en " +
              std::to_string(suyo));
        truth("and it comes back with an error", salida.load() > 0,
              "run() devolvio " + std::to_string(salida.load()));

        B.shutdown();
        tb.join();

        // Y el primero sigue siendo el que atiende.
        truth("while the first one is still the one serving", A.bound_port() == port);

        A.shutdown();
        ta.join();
    }

    void the_server_serves_while_it_solves(Session& S) {
        std::string e;
        if (!truth("server spot builds", spot(S, "Ah9h4h", e), e)) return;

        WebUI W(S, 0, false);            // puerto 0: lo elige el sistema
        W.set_quiet(true);
        std::thread srv([&W]() { W.run(); });

        // Esperar a que este escuchando. Sin esto la prueba mediria la carrera
        // entre dos hilos, no el servidor.
        int port = 0;
        for (int i = 0; i < 400 && port == 0; ++i) {
            port = W.bound_port();
            if (!port) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (!truth("the server comes up", port != 0, "no llego a escuchar")) {
            W.shutdown();
            srv.join();
            return;
        }

        const bool solving = S.solve_async(4000);
        truth("and a solve is running behind it", solving, "no arranco el solve");

        // La tormenta, y tiene que ser CONCURRENTE.
        //
        // La primera version pedia las cosas de una en una desde este hilo y
        // pasaba con el fallo original puesto a mano: claro, lo que se rompia
        // era una CARRERA entre construir el estado y el solver tocandolo, y en
        // serie no hay carrera que valer. Ocho clientes a la vez, cada uno
        // mezclando consultas enteras con cortes a media peticion, que es como
        // llega de verdad cuando el navegador pregunta el progreso cada poco y
        // ademas cambias de nodo.
        std::atomic<int> served{0}, ok200{0};
        {
            std::vector<std::thread> storm;
            for (int t = 0; t < 8; ++t) {
                storm.push_back(std::thread([&, t]() {
                    for (int i = 0; i < 40; ++i) {
                        if ((i + t) % 3 == 0) { abort_midway(port); continue; }
                        const char* p = ((i + t) % 2) ? "/api/progress" : "/api/state";
                        const std::string r = fetch(port, p);
                        served.fetch_add(1);
                        if (r.find("200 OK") != std::string::npos) ok200.fetch_add(1);
                    }
                }));
            }
            for (std::thread& th : storm) th.join();
        }
        // Y que el solve siguiera vivo durante la tormenta, porque si no esto
        // mide un servidor parado y no prueba nada de lo que dice probar.
        truth("the solve outlived the storm", S.busy(), "el solve ya habia terminado");
        truth("every request in the storm was answered",
              ok200.load() == served.load() && served.load() > 100,
              std::to_string(ok200.load()) + " de " + std::to_string(served.load()));

        // Clientes que se quedan a medias y no cuelgan. Cada uno se lleva un
        // hilo, y sin plazo de recepcion se lo quedaria para siempre. Lo que
        // se mide aqui es lo rapido: que teniendolos colgando el servidor
        // siga atendiendo a todo el mundo.
        std::vector<sock_t> colgados;
        for (int i = 0; i < 8; ++i) {
            const sock_t c = stall(port);
            if (c != SOCK_INVALID) colgados.push_back(c);
        }
        const std::string bajo = fetch(port, "/api/progress");
        truth("it serves with eight clients hanging half-open",
              bajo.find("200 OK") != std::string::npos,
              bajo.empty() ? "no contesto nada" : bajo.substr(0, 60));
        for (sock_t c : colgados) SOCK_CLOSE(c);

        // Clientes sordos: piden la pagina entera -- lo bastante grande para
        // llenar el buffer del socket -- y no leen. Con la respuesta enviada
        // dentro del cerrojo, el primero de estos deja el servidor muerto.
        // El plazo es corto a proposito: lo que se mide es que conteste YA, no
        // que acabe contestando.
        std::vector<sock_t> sordos;
        for (int i = 0; i < 4; ++i) {
            // La tabla de runouts es la respuesta grande que ademas coge el
            // cerrojo. La pagina es mas grande todavia pero se sirve sin
            // cerrojo, asi que como cliente sordo no prueba nada.
            const sock_t c = deaf_request(port, "/api/runouts");
            if (c != SOCK_INVALID) sordos.push_back(c);
            const sock_t d = deaf_request(port, "/api/state");
            if (d != SOCK_INVALID) sordos.push_back(d);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
        const std::string tras = fetch(port, "/api/state");
        const double espera = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - t0).count();
        truth("it still answers with eight deaf clients attached",
              tras.find("200 OK") != std::string::npos,
              tras.empty() ? "no contesto nada" : tras.substr(0, 60));
        truth("and answers them promptly", espera < 5.0,
              "tardo " + std::to_string(espera) + " s");
        for (sock_t c : sordos) SOCK_CLOSE(c);

        // Y lo que de verdad se rompio: el estado, que si toca el solve.
        const std::string st = fetch(port, "/api/state");
        truth("state answers while solving",
              st.find("200 OK") != std::string::npos &&
              st.find("\"board\"") != std::string::npos,
              st.empty() ? "no contesto nada" : st.substr(0, 60));

        S.request_stop();
        for (int i = 0; i < 600 && S.busy(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        truth("and the stop was heard", !S.busy(), "el solve no paro");

        // Terminada la tormenta el servidor tiene que seguir entero: la pagina
        // completa, no solo los dos sitios que no cogen el cerrojo.
        const std::string pg = fetch(port, "/");
        truth("the page still comes back afterwards",
              pg.find("200 OK") != std::string::npos &&
              pg.find("DCFR Solver") != std::string::npos,
              pg.empty() ? "no contesto nada" : pg.substr(0, 60));

        W.shutdown();
        srv.join();
        truth("and it shuts down when told", W.bound_port() == 0, "se quedo escuchando");
    }

    void the_page_is_not_broken() {
        const std::string P = WEBUI_PAGE;

        // 1. Nothing declared twice at the top level of the script. A redeclared
        //    const is a SyntaxError, and a SyntaxError anywhere means no script.
        std::map<std::string, int> seen;
        std::vector<std::string> dup;
        size_t pos = 0;
        while (pos < P.size()) {
            size_t eol = P.find('\n', pos);
            if (eol == std::string::npos) eol = P.size();
            const std::string line = P.substr(pos, eol - pos);
            pos = eol + 1;
            if (line.empty() || line[0] == ' ' || line[0] == '\t') continue;
            static const char* kw[] = { "const ", "let ", "function ", "async function " };
            for (const char* k : kw) {
                const size_t kl = std::strlen(k);
                if (line.compare(0, kl, k) != 0) continue;
                size_t i = kl;
                std::string name;
                while (i < line.size() &&
                       (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_' ||
                        line[i] == '$'))
                    name += line[i++];
                if (name.empty()) break;
                if (++seen[name] == 2) dup.push_back(name);
                break;
            }
        }
        if (!truth("nothing in the page is declared twice", dup.empty(),
                   dup.empty() ? "" : ("redeclared: " + dup[0])))
            return;

        // 2. Every quoted string closes on the line it opens.
        //
        //    A stray apostrophe -- "the board's" inside a single-quoted string
        //    -- ends the string early and the rest of the line parses as
        //    garbage. That is a SyntaxError, and a SyntaxError anywhere means
        //    no script at all: every button dead, every panel empty, the range
        //    grid and the per-player sizing boxes simply absent. Which is the
        //    exact failure part 1 exists to prevent, and part 1 does not catch
        //    it -- nothing is declared twice, the file is just unparseable.
        //
        //    In this page every string closes on its own line and concatenation
        //    is done with `+`, so a line that ends inside a string is a broken
        //    line. Only the script is scanned: the CSS above it has block
        //    comments full of prose, apostrophes included.
        {
            const size_t s0 = P.find("<script");
            const size_t s1 = P.find("</script>", s0 == std::string::npos ? 0 : s0);
            std::vector<int> unclosed;
            int line = 1;
            const size_t stop = (s1 == std::string::npos) ? P.size() : s1;
            for (size_t i = (s0 == std::string::npos ? P.size() : s0); i < stop; ) {
                size_t eol = P.find('\n', i);
                if (eol == std::string::npos || eol > stop) eol = stop;
                char q = 0;
                for (size_t k = i; k < eol; ++k) {
                    const char c = P[k];
                    if (q) {
                        if (c == 0x5c) { ++k; continue; }   // 0x5c: la barra invertida escapa lo siguiente
                        if (c == q) q = 0;
                    } else if (c == 0x27 || c == 0x22) {   // comilla simple o doble
                        q = c;
                    } else if (c == '/' && k + 1 < eol && P[k + 1] == '/') {
                        break;                    // el resto de la linea es comentario
                    }
                }
                if (q) unclosed.push_back(line);
                ++line;
                i = eol + 1;
            }
            truth("every string in the page closes on its own line", unclosed.empty(),
                  unclosed.empty() ? "" :
                      (std::to_string(unclosed.size()) +
                       " lines end inside a string, the first at script line " +
                       std::to_string(unclosed[0])));
            if (!unclosed.empty()) return;
        }

        // 3. Every id the script reaches for exists in the markup. A typo there
        //    is a null dereference the moment that code path runs, which takes
        //    the rest of the handler with it.
        std::vector<std::string> missing;
        const std::string needle = "getElementById('";
        for (size_t i = P.find(needle); i != std::string::npos; i = P.find(needle, i + 1)) {
            const size_t a = i + needle.size();
            const size_t b = P.find('\'', a);
            if (b == std::string::npos) break;
            const std::string id = P.substr(a, b - a);
            // getElementById('oopb'+i) builds the id at run time, so the
            // literal half of it is not an element and never was.
            if (id.empty()) continue;
            if (b + 1 < P.size() && P[b + 1] == '+') continue;
            if (P.find("id=\"" + id + "\"") == std::string::npos)
                missing.push_back(id);
        }
        truth("every id the script asks for is in the page", missing.empty(),
              missing.empty() ? "" : ("no element with id '" + missing[0] + "'"));

        // 4. Every function a button calls is defined somewhere in the script.
        //
        //    Un boton que llama a una funcion que ya no existe no da ningun
        //    aviso: la pagina carga entera, el boton se ve normal, y al
        //    pulsarlo no pasa nada. Es lo que queda cuando se retira una
        //    funcion y su boton se queda, o al reves. La cuadricula tenia
        //    fillRange(0) y fillRange(1) para "Clear" y "All"; al quitar "All"
        //    la funcion paso a llamarse clearRange, y el boton que sobrevive
        //    tiene que haberse enterado.
        std::vector<std::string> dead;
        const std::string oc = "onclick=\"";
        for (size_t i = P.find(oc); i != std::string::npos; i = P.find(oc, i + 1)) {
            size_t a = i + oc.size();
            std::string name;
            while (a < P.size() &&
                   (std::isalnum(static_cast<unsigned char>(P[a])) || P[a] == '_' || P[a] == '$'))
                name += P[a++];
            if (name.empty() || a >= P.size() || P[a] != '(') continue;  // no es una llamada
            if (name == "if" || name == "for" || name == "while" || name == "return") continue;
            if (P.find("function " + name + "(") == std::string::npos)
                dead.push_back(name);
        }
        truth("every button calls a function that exists", dead.empty(),
              dead.empty() ? "" : ("nothing defines " + dead[0] + "()"));

        // 5. Toda variable de color que se usa esta declarada.
        //
        //    var(--acc) no existia -- la declarada es --accent -- y CSS no
        //    protesta: la propiedad entera se descarta y el elemento se queda
        //    con el color que hereda. Se ve casi bien, que es peor que verse
        //    mal, porque nadie lo mira dos veces. Estaba en cuatro sitios.
        std::vector<std::string> undef;
        const std::string vv = "var(--";
        for (size_t i = P.find(vv); i != std::string::npos; i = P.find(vv, i + 1)) {
            size_t a = i + 4;                       // apunta a los dos guiones
            size_t b = a;
            while (b < P.size() && P[b] != ')' && P[b] != ',' && P[b] != ' ') ++b;
            const std::string name = P.substr(a, b - a);
            if (name.size() <= 2) continue;
            // var(--warn,#e0b33a) trae su propio color de reserva: si la
            // variable no existe se usa ese, que es CSS valido y deliberado.
            if (b < P.size() && P[b] == ',') continue;
            if (P.find(name + ":") == std::string::npos) undef.push_back(name);
        }
        truth("every colour the page uses is declared", undef.empty(),
              undef.empty() ? "" : (undef[0] + " is used but never defined"));
    }

    // A chance node averages over the cards the hero can see, and the divisor
    // is NOT that count: every villain hand is two cards that are neither on the
    // board nor in the hero's hand, so it blocks exactly two of them. Dividing
    // by the hero's count lands short by (D-2)/D, and it compounds per street.
    //
    // Nothing in this file could catch that. Every closed-form toy here is on a
    // river, where there is no chance node at all, and the error is a single
    // scale factor per hand, so it leaves the STRATEGY alone and cancels out of
    // the zero-sum check. It took solving the same spot in another solver to see it.
    //
    // So here is a multi-street spot whose value is arithmetic rather than a
    // recorded number. Board Ks 7h 2c 9d, OOP holds AdAc, IP holds KhKd. IP has
    // three kings; OOP has a pair of aces and one card to come. OOP cannot make
    // a straight or a flush here, and any river that pairs the board fills IP
    // up, so OOP wins exactly when the river is one of the two aces left. Two
    // cards out of the forty-four neither player holds:
    //
    //     EV(OOP) = pot * 2/44 = 100 * 0.0454545... = 4.545454...
    void a_chance_node_averages_over_the_right_cards(Session& S) {
        std::string e;
        cfg::POT0 = 100.0;
        cfg::STACK = 200.0;
        S.clear_all_locks();
        if (!truth("the two-hand turn spot builds", S.set_board("Ks7h2c9d", e), e)) return;
        if (!truth("OOP holds exactly AdAc", S.set_range(0, "AdAc", e), e)) return;
        if (!truth("IP holds exactly KhKd", S.set_range(1, "KhKd", e), e)) return;
        const std::vector<Sizing> none;
        for (int st = 0; st < 3; ++st) {
            if (!truth("no bets anywhere", S.set_sizings(true, st, none, e), e)) return;
            if (!truth("no raises anywhere", S.set_sizings(false, st, none, e), e)) return;
            if (!truth("no bare all-in", S.set_allin(st, false, e), e)) return;
        }
        if (!truth("pot 100", S.set_pot(100.0, e), e)) return;
        if (!truth("stack 200", S.set_stack(200.0, e), e)) return;
        S.solve(40, 0);
        if (!truth("it solved", S.solver() != nullptr)) return;

        // Nobody can do anything but check, so the value is pure showdown and
        // no amount of iterating moves it.
        must_be("OOP's share is exactly two rivers out of forty-four",
                S.solver()->root_ev(0), 100.0 * 2.0 / 44.0, 1e-9);

        // And the same board one card earlier, where the error would compound a
        // second time. Two cards to come, so the closed form is longer, but the
        // one thing that must hold is that suit collapsing does not change it
        // and that it is not the number the old divisor gave (12.0722).
        if (!truth("the same spot a street earlier", S.set_board("Ks7h2c", e), e)) return;
        S.solve(40, 0);
        const double flop_ev = S.solver()->root_ev(0);
        truth("a flop of it is nowhere near the old wrong number",
              std::fabs(flop_ev - 12.0722) > 1.0,
              "got " + std::to_string(flop_ev));
        must_be("and two chance streets still leave a zero-sum game",
                flop_ev + S.solver()->root_ev(1), 100.0, 1e-9);
    }

    void nodelock_promises(Session& S) {
        std::string e;
        if (!truth("lock spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(100, 0);

        const int root = S.tree().ctx[0].tree.root;
        const Node& rn = S.tree().ctx[0].tree.nodes[static_cast<size_t>(root)];
        const int ix = S.tree().ctx[0].tree.action_index(rn, AK_BET);
        const int cx = S.tree().ctx[0].tree.action_index(rn, AK_CHECK);
        if (!truth("root offers check and bet", ix >= 0 && cx >= 0)) return;

        // A mix, not just a pin to one action -- and given as weights that do
        // not already sum to one, so the normalisation is under test too.
        std::vector<std::pair<std::string, double>> mix;
        mix.push_back(std::make_pair(std::string("B"), 1.0));
        mix.push_back(std::make_pair(std::string("X"), 3.0));
        int matched = 0;
        if (!truth("locks QQ+ to a 25/75 mix",
                   S.add_lock(0, root, "QQ+", mix, matched, e), e)) return;
        same("the lock matched QQ+ on this board", matched, 12);
        S.solve(150, 0);

        DCFRSolver& sol = *S.solver();
        const int nh = sol.num_hands();
        std::vector<double> st(static_cast<size_t>(rn.num_actions) * nh);
        sol.avg_strategy_block(0, root, st.data());
        double worst_bet = 0.0, worst_chk = 0.0;
        int locked = 0;
        for (int h = 0; h < nh; ++h) {
            if (!sol.is_hand_locked(0, root, 0, h)) continue;
            ++locked;
            worst_bet = std::max(worst_bet,
                std::fabs(st[static_cast<size_t>(ix) * nh + h] - 0.25));
            worst_chk = std::max(worst_chk,
                std::fabs(st[static_cast<size_t>(cx) * nh + h] - 0.75));
        }
        same("every matched combo is locked", locked, 12);
        truth("the locked mix is exactly what was asked",
              worst_bet < 1e-9 && worst_chk < 1e-9,
              "bet off by " + std::to_string(worst_bet) +
              ", check off by " + std::to_string(worst_chk));

        // Not just at the runout being read: a lock lives on the template node,
        // so it has to hold at every stored instance of it. The turn root has
        // exactly one instance, which would make that claim vacuous -- so the
        // claim is made a street later, where the same node is reached through
        // every river card.
        int deep = -1;
        for (size_t c = 1; c < S.tree().ctx.size(); ++c) {
            const BetTree& bt = S.tree().ctx[c].tree;
            if (S.tree().ctx[c].instances > 1 &&
                bt.nodes[static_cast<size_t>(bt.root)].type == NT_DECISION &&
                bt.action_index(bt.nodes[static_cast<size_t>(bt.root)], AK_BET) >= 0) {
                deep = static_cast<int>(c);
                break;
            }
        }
        if (truth("a river node is reached through many cards", deep >= 0)) {
            const BetTree& bt  = S.tree().ctx[static_cast<size_t>(deep)].tree;
            const int      dn  = bt.root;
            const Node&    dnn = bt.nodes[static_cast<size_t>(dn)];
            const int      dix = bt.action_index(dnn, AK_BET);
            int m2 = 0;
            if (truth("locks QQ+ there too", S.add_lock(deep, dn, "QQ+", mix, m2, e), e)) {
                S.solve(120, 0);
                DCFRSolver& s2 = *S.solver();
                double worst_inst = 0.0;
                const long long insts = S.tree().ctx[static_cast<size_t>(deep)].instances;
                std::vector<double> si(static_cast<size_t>(dnn.num_actions) * nh);
                for (long long b = 0; b < insts; ++b) {
                    s2.avg_strategy_inst(deep, dn, b, si.data());
                    for (int h = 0; h < nh; ++h) {
                        if (!s2.is_hand_locked(deep, dn, 0, h)) continue;
                        worst_inst = std::max(worst_inst,
                            std::fabs(si[static_cast<size_t>(dix) * nh + h] - 0.25));
                    }
                }
                same("river cards reaching that node", insts, 35);
                truth("it holds on every one of them", worst_inst < 1e-9,
                      "worst " + std::to_string(worst_inst) + " over " +
                      std::to_string(insts) + " instances");
            }
            S.remove_lock(deep, dn);
        }

        // A rebuild re-resolves locks by context label and node path, so a
        // change that leaves the node in place must keep them.
        if (!truth("rebuild", S.rebuild(e), e)) return;
        same("the lock survived a rebuild", S.dropped_locks(), 0);
        same("and is still one lock", static_cast<long long>(S.locks().size()), 1);

        // Now make its node vanish. A lock on the root of the flop tree cannot
        // survive moving to a river board, and saying so is the point.
        if (!truth("moves to a different board", S.set_board("Ah9h4hKd2s", e), e)) return;
        same("the lock was dropped, not silently kept",
             static_cast<long long>(S.locks().size()), 0);
    }

    // A lock that does not change the strategy is not a lock.
    // Un lock es de UN runout, no de la calle entera.
    //
    // Es lo que hace la referencia y es lo unico que sirve para estudiar: bloquear
    // "que sobreapueste en el 3s" no es bloquear "que sobreapueste en cualquier
    // river". Antes esto vivia en la plantilla del contexto y salia en los 48
    // rivers a la vez -- y la interfaz te ensenaba la carta elegida antes de
    // ignorarla. Contra la referencia eso eran tres fichas sobre un bote de 100.
    //
    // Se comprueban las tres cosas que hacen falta, porque cualquiera de las
    // dos primeras sin la tercera dejaria pasar el fallo:
    //   1. el river bloqueado juega lo que se le dijo,
    //   2. otro river cualquiera NO,
    //   3. y el hermano de palo del bloqueado tampoco -- que es el que se
    //      colaria si el colapso de palos se llevara el 3s y el 3h juntos.
    void a_lock_belongs_to_one_runout(Session& S) {
        std::string e;
        if (!truth("per-runout lock spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        if (!truth("no raises", S.set_sizings(false, ST_RIVER, std::vector<Sizing>(), e), e)) return;

        // Al river, por una carta concreta. El codigo de la apuesta lleva la
        // cantidad dentro (B13, no B), y esa cantidad depende del bote y del
        // tamano, asi que se pregunta en vez de escribirla: escrita a mano, la
        // prueba se rompe cada vez que cambia un valor por defecto y no por lo
        // que viene a vigilar.
        S.go_root();
        const BetTree& bt0 = S.tree().ctx[0].tree;
        const Node& r0 = bt0.nodes[static_cast<size_t>(bt0.root)];
        std::string apuesta;
        for (int a = 0; a < r0.num_actions; ++a)
            if (bt0.act(r0, a).kind == AK_BET) apuesta = bt0.act(r0, a).code;
        if (!truth("the turn root has a bet", !apuesta.empty(), "no bet action")) return;
        const char* pasos[3] = { apuesta.c_str(), "C", "2s" };
        for (const char* paso : pasos)
            if (!truth(std::string("cd ") + paso, S.go(paso, e), e)) return;
        const int ci = S.cur_ctx(), nid = S.cur_node();

        std::vector<std::pair<std::string, double>> mix;
        mix.push_back(std::make_pair(std::string("B"), 0.30));
        mix.push_back(std::make_pair(std::string("X"), 0.70));
        int matched = 0;
        if (!truth("lock the 2s river", S.add_lock(ci, nid, "random", mix, matched, e), e)) return;
        truth("and it remembers which card", !S.locks().empty() &&
              S.locks().back().runout == "2s",
              S.locks().empty() ? "no lock" : ("runout '" + S.locks().back().runout + "'"));
        S.solve(400, 0);

        // La frecuencia de apostar en un river dado, promediada sobre las manos
        // que de verdad llegan.
        struct Leer {
            static double freq(Session& S, const std::string& apuesta,
                               const char* carta, std::string& e) {
                S.go_root();
                if (!S.go(apuesta, e)) return -1.0;
                if (!S.go("C", e)) return -1.0;
                if (!S.go(carta, e)) return -1.0;
                long long inst = 0;
                int perm = S.deal().identity();
                S.cur_addr(inst, perm);
                const NodeStats N = gather(*S.solver(), S.cur_ctx(), S.cur_node(), inst, perm);
                if (!N.ok) return -1.0;
                const int ib = 1;               // Check es 0, Bet es 1
                double w = 0.0, f = 0.0;
                for (int h = 0; h < N.nh; ++h) {
                    const double x = N.weight[static_cast<size_t>(h)];
                    if (x <= 1e-9) continue;
                    w += x;
                    f += x * N.freq(h, ib);
                }
                return w > 1e-9 ? f / w : -1.0;
            }
        };
        const double bloq = Leer::freq(S, apuesta, "2s", e);
        const double otro = Leer::freq(S, apuesta, "5d", e);
        const double palo = Leer::freq(S, apuesta, "2h", e);
        if (!truth("the three rivers read back", bloq >= 0 && otro >= 0 && palo >= 0, e)) return;

        close_to("the locked river plays the lock", bloq, 0.30, 1e-6);
        truth("another river does not", std::fabs(otro - 0.30) > 0.02,
              "it plays " + std::to_string(otro) + " too");
        truth("and neither does its suit twin", std::fabs(palo - 0.30) > 0.02,
              "the 2h plays " + std::to_string(palo) + ": suit collapsing dragged it along");
    }

    // La equity promedia los runouts que quedan, y aqui se cuenta a mano.
    //
    // Habia una comprobacion de esto y solo valia para el RIVER: con las cinco
    // cartas fuera no hay nada que promediar y el reparto es una vuelta sobre
    // las manos del rival. Lo que no estaba sujeto por nada es justo lo dificil:
    // en un flop la equity es la media sobre los 1.176 runouts, y el barrido que
    // la calcula es O(N) por runout con inclusion-exclusion, no una comparacion
    // mano contra mano. Un fallo ahi no se ve, porque el numero sigue siendo
    // verosimil.
    //
    // Se compara contra la definicion, por fuerza bruta: para cada runout, cada
    // mano contra cada mano del rival, evaluando las siete cartas. Es lento y da
    // igual -- son rangos de quince combos --, y sobre todo NO comparte camino
    // con el codigo que vigila: si compartiera, las dos se equivocarian juntas.
    //
    // Tres casos, que son los tres que existen:
    //
    //    flop sin carta dada     la media sobre los 1.176 runouts
    //    flop con el turn dado   la media sobre los que empiezan por esa carta
    //    turn sin carta dada     la media sobre los 48 rivers
    //
    // El del medio es el que tuvo el fallo de verdad: sin filtrar por la carta
    // que ya salio, la equity de un nodo de turn era equity de FLOP, y en
    // Ah9h4h/Kd un KsQs marcaba 26,80 -- lo que vale como carta alta antes del
    // rey -- cuando ya era pareja de reyes.
    //
    // El calculo que esto vigila se comparo ademas contra la referencia: 320 casos
    // de board por rango por rango, 186.508 equities, y la peor diferencia fue
    // de 0,0026 puntos, que es el redondeo con el que la referencia las imprime.
    // El rango de un nodo es el del padre por la frecuencia de la accion.
    //
    // Es la definicion de "alcance", y hasta ahora no la comprobaba nadie. La
    // columna `weight` del CSV lleva masa de PAREJAS -- alcance propio por los
    // combos del rival que no te bloquean --, que es el peso correcto para
    // promediar pero que no es el rango: en la raiz vale del orden de 500 para
    // una mano de peso 1. Quien abria la hoja y leia "weight" leia otra cosa.
    // Ahora hay una columna `reach` que si es el rango, y esto es lo que la
    // sujeta.
    //
    // Si OOP apuesta el 30% con una mano, detras de esa apuesta esa mano llega
    // con el 30% de lo que traia. Se mide en las dos ramas de la raiz -- la que
    // pasa y la que apuesta -- y combo a combo, no en el agregado: un agregado
    // cuadra tambien si dos manos se intercambian el peso.
    void the_range_of_a_node_is_the_parent_times_the_frequency(Session& S) {
        std::string e;
        if (!truth("reach spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(300, 0);
        S.go_root();
        long long inst = 0;
        int perm = S.deal().identity();
        S.cur_addr(inst, perm);
        const NodeStats R = gather(*S.solver(), S.cur_ctx(), S.cur_node(), inst, perm);
        if (!truth("the root reads", R.ok)) return;

        // La masa de parejas y el alcance no son el mismo numero, y conviene
        // que quede dicho con una cuenta: si alguien los iguala, esto salta.
        double razon = 1e18;
        for (int h = 0; h < R.nh; ++h) {
            const double rc = R.v.own_reach[static_cast<size_t>(h)];
            if (rc > 1e-9)
                razon = std::min(razon, R.weight[static_cast<size_t>(h)] / rc);
        }
        truth("pair mass is not the range", razon > 100.0,
              "la razon mas floja fue " + std::to_string(razon));

        int ramas = 0;
        for (int a = 0; a < R.A; ++a) {
            const std::string code = R.codes[static_cast<size_t>(a)];
            S.go_root();
            std::string ge;
            if (!S.go(code, ge)) continue;          // pagar o pasar cierran calle
            long long i2 = 0;
            int p2 = S.deal().identity();
            S.cur_addr(i2, p2);
            const NodeStats C = gather(*S.solver(), S.cur_ctx(), S.cur_node(), i2, p2);
            if (!C.ok || C.player == R.player || C.nh != R.nh) continue;
            ++ramas;

            double peor = 0.0;
            std::string donde;
            int mirados = 0;
            for (int h = 0; h < R.nh; ++h) {
                const double traia = R.v.own_reach[static_cast<size_t>(h)];
                if (traia <= 1e-9) continue;
                const double quiere = traia * R.freq(h, a);
                const double hay = C.v.opp_reach[static_cast<size_t>(h)];
                const double d = std::fabs(hay - quiere);
                if (d > peor) {
                    peor = d;
                    const Combo& k = S.deal().combos[static_cast<size_t>(h)];
                    donde = card_str(k.c1) + card_str(k.c2) + " traia " +
                            std::to_string(traia) + " x " +
                            std::to_string(R.freq(h, a)) + " y llega " +
                            std::to_string(hay);
                }
                ++mirados;
            }
            truth("there are hands behind " + code, mirados > 100,
                  std::to_string(mirados) + " manos");
            truth("and behind " + code + " every hand brings its own frequency",
                  peor < 1e-9, donde + " se separa por " + std::to_string(peor));
        }
        truth("both branches of the root were looked at", ramas == 2,
              std::to_string(ramas) + " ramas");
        S.go_root();
    }

    void the_runout_average_is_the_average_of_the_runouts(Session& S) {
        (void)S;
        struct Caso { const char* board; const char* dada; };
        static const Caso casos[] = {
            { "Ks7h2c",   ""   },
            { "Ah9h4h",   "Kd" },
            { "Ah9h4h",   "2h" },   // la carta dada es de un palo del board
            { "Ks7h2c9d", ""   },
            { "2c2d2h",   ""   },   // board de trio: casi todo empata
        };
        // Quince combos por lado, escritos a mano para que haya de todo: color,
        // escalera, parejas y aire.
        static const char* const MANOS[15] = {
            "AhKh", "AsKd", "QcQd", "JsTs", "9c9d", "8h7h", "7s6s", "5c4c",
            "AcQs", "KcJd", "Td9s", "6h5d", "3c2s", "AdTh", "QhJc"
        };
        for (const Caso& c : casos) {
            std::vector<int> b;
            std::string e;
            if (!truth(std::string("equity board ") + c.board,
                       parse_board(c.board, b, e), e)) continue;
            Deal d;
            if (!truth("the deal builds", d.build(b, e), e)) continue;

            std::vector<int> ya;
            if (c.dada[0]) {
                const int x = parse_card(c.dada);
                if (!truth(std::string("the dealt card ") + c.dada + " parses", x >= 0))
                    continue;
                bool choca = false;
                for (int y : b) if (y == x) choca = true;
                if (!truth("and it is not already on the board", !choca)) continue;
                ya.push_back(x);
            }

            // El rango del rival: los quince, con pesos distintos para que el
            // promedio pese de verdad y no sea una media simple.
            std::vector<double> vill(static_cast<size_t>(d.num()), 0.0);
            std::vector<int> suyas;
            for (int i = 0; i < 15; ++i) {
                const int c1 = parse_card(std::string(MANOS[i]).substr(0, 2));
                const int c2 = parse_card(std::string(MANOS[i]).substr(2, 2));
                if (c1 < 0 || c2 < 0) continue;
                bool choca = false;
                for (int y : b) if (y == c1 || y == c2) choca = true;
                for (int y : ya) if (y == c1 || y == c2) choca = true;
                if (choca) continue;
                const int h = d.combo_at[static_cast<size_t>(c1) * 52 + c2];
                if (h < 0) continue;
                vill[static_cast<size_t>(h)] = 0.2 + 0.05 * i;
                suyas.push_back(h);
            }
            if (!truth(std::string("there is a villain range on ") + c.board,
                       suyas.size() >= 10,
                       std::to_string(suyas.size()) + " combos")) continue;

            std::vector<double> eq;
            compute_equity(d, vill, eq, ya);

            // Y ahora la definicion, contada mano a mano.
            int mirados = 0;
            double peor = 0.0;
            std::string donde;
            for (int i = 0; i < 15; ++i) {
                const int c1 = parse_card(std::string(MANOS[i]).substr(0, 2));
                const int c2 = parse_card(std::string(MANOS[i]).substr(2, 2));
                if (c1 < 0 || c2 < 0) continue;
                bool choca = false;
                for (int y : b) if (y == c1 || y == c2) choca = true;
                for (int y : ya) if (y == c1 || y == c2) choca = true;
                if (choca) continue;
                const int h = d.combo_at[static_cast<size_t>(c1) * 52 + c2];
                if (h < 0) continue;

                double num = 0.0, den = 0.0;
                for (int r = 0; r < d.num_runouts; ++r) {
                    int t, rv;
                    d.runout_cards(r, t, rv);
                    if (!ya.empty()) {
                        bool vale = true;
                        for (int y : ya) if (y != t && y != rv) { vale = false; break; }
                        if (!vale) continue;
                    }
                    if (t >= 0 && (t == c1 || t == c2)) continue;
                    if (rv >= 0 && (rv == c1 || rv == c2)) continue;

                    std::vector<int> mesa(b);
                    if (t  >= 0) mesa.push_back(t);
                    if (rv >= 0) mesa.push_back(rv);
                    int siete[7];
                    siete[0] = c1;
                    siete[1] = c2;
                    for (size_t k = 0; k < mesa.size(); ++k) siete[2 + k] = mesa[k];
                    const int mia = eval_best(siete, static_cast<int>(2 + mesa.size()));

                    for (int g : suyas) {
                        const Combo& j = d.combos[static_cast<size_t>(g)];
                        if (j.c1 == c1 || j.c1 == c2 || j.c2 == c1 || j.c2 == c2) continue;
                        if (t  >= 0 && (j.c1 == t  || j.c2 == t))  continue;
                        if (rv >= 0 && (j.c1 == rv || j.c2 == rv)) continue;
                        const double w = vill[static_cast<size_t>(g)];
                        siete[0] = j.c1;
                        siete[1] = j.c2;
                        const int suya = eval_best(siete, static_cast<int>(2 + mesa.size()));
                        siete[0] = c1;
                        siete[1] = c2;
                        den += w;
                        if (mia > suya)       num += w;
                        else if (mia == suya) num += 0.5 * w;
                    }
                }
                if (den <= 1e-12) continue;
                const double mano = 100.0 * num / den;
                const double barrido = eq[static_cast<size_t>(h)];
                const double dif = std::fabs(mano - barrido);
                if (dif > peor) {
                    peor = dif;
                    donde = std::string(MANOS[i]) + " mano " +
                            std::to_string(mano) + " barrido " +
                            std::to_string(barrido);
                }
                ++mirados;
            }
            const std::string ref = std::string(c.board) +
                                    (c.dada[0] ? std::string("/") + c.dada : "");
            truth("there are hands to average on " + ref, mirados >= 10,
                  std::to_string(mirados) + " manos");
            truth("and counting every runout by hand agrees on " + ref,
                  peor < 1e-9, donde + " se separan por " + std::to_string(peor));
        }
    }

    void the_equity_is_of_this_runout(Session& S) {
        std::string e;
        if (!truth("runout-equity spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(150, 0);
        S.go_root();
        std::string ge;
        const bool bajo = S.go("X", ge) && S.go("X", ge) && S.go("Kd", ge) &&
                          S.go("X", ge) && S.go("X", ge) && S.go("2c", ge);
        if (!truth("the session reaches a river node", bajo, ge)) { S.go_root(); return; }

        const std::vector<int> b = S.board_here();
        truth("and the board there is five cards", b.size() == 5,
              std::to_string(b.size()) + " cartas");
        long long inst = 0;
        int perm = S.deal().identity();
        S.cur_addr(inst, perm);
        const NodeStats N = gather(*S.solver(), S.cur_ctx(), S.cur_node(), inst, perm, true);
        if (!truth("the river node reads", N.ok && b.size() == 5)) { S.go_root(); return; }

        // Los cinco naipes de cada mano en ESTE board, de una vez.
        const Deal& D = S.deal();
        std::vector<long long> sc(static_cast<size_t>(N.nh), -1);
        for (int h = 0; h < N.nh; ++h) {
            const Combo& k = D.combos[static_cast<size_t>(h)];
            if (k.c1 < 0) continue;
            bool choca = false;
            for (int x : b) if (x == k.c1 || x == k.c2) choca = true;
            if (choca) continue;
            int siete[7];
            siete[0] = k.c1; siete[1] = k.c2;
            for (size_t i = 0; i < b.size(); ++i) siete[2 + i] = b[i];
            sc[static_cast<size_t>(h)] = eval_best(siete, 7);
        }

        const std::vector<double>& ro = N.v.opp_reach;
        int    mirados = 0;
        double peor = 0.0;
        std::string donde;
        for (int h = 0; h < N.nh; ++h) {
            if (N.weight[static_cast<size_t>(h)] <= 1e-9 || sc[static_cast<size_t>(h)] < 0) continue;
            const Combo& k = D.combos[static_cast<size_t>(h)];
            double gana = 0.0, empata = 0.0, total = 0.0;
            for (int g = 0; g < N.nh; ++g) {
                const double w = ro[static_cast<size_t>(g)];
                if (w <= 0.0 || sc[static_cast<size_t>(g)] < 0) continue;
                const Combo& j = D.combos[static_cast<size_t>(g)];
                if (j.c1 == k.c1 || j.c1 == k.c2 || j.c2 == k.c1 || j.c2 == k.c2) continue;
                total += w;
                if (sc[static_cast<size_t>(h)] > sc[static_cast<size_t>(g)]) gana += w;
                else if (sc[static_cast<size_t>(h)] == sc[static_cast<size_t>(g)]) empata += w;
            }
            if (total <= 1e-12) continue;
            const double mio = 100.0 * (gana + 0.5 * empata) / total;
            const double suyo = N.equity(*S.solver(), h);
            const double d = std::fabs(mio - suyo);
            if (d > peor) { peor = d; donde = card_str(k.c1) + card_str(k.c2); }
            ++mirados;
        }
        truth("there are hands to arbitrate", mirados > 20,
              std::to_string(mirados) + " manos");
        truth("and the showdown, counted by hand, says the same", peor < 1e-6,
              donde + " se separa por " + std::to_string(peor) + " puntos");
        S.go_root();
    }

    void equity_follows_the_node(Session& S) {
        std::string e;
        if (!truth("equity spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(200, 0);

        struct Ver {
            static double eq(Session& S, const char* clase, bool del_nodo) {
                long long inst = 0;
                int perm = S.deal().identity();
                S.cur_addr(inst, perm);
                const NodeStats N = gather(*S.solver(), S.cur_ctx(), S.cur_node(),
                                           inst, perm, del_nodo);
                if (!N.ok) return -1.0;
                double w = 0.0, q = 0.0;
                for (int h = 0; h < N.nh; ++h) {
                    if (class_name(S.deal().combos[static_cast<size_t>(h)].cls) != clase) continue;
                    const double x = N.weight[static_cast<size_t>(h)];
                    if (x <= 1e-12) continue;
                    w += x;
                    q += x * N.equity(*S.solver(), h);
                }
                return w > 1e-12 ? q / w : -1.0;
            }
        };

        // La raiz es de OOP; la primera decision de IP viene despues de un pass
        // o de una apuesta, y son dos rangos distintos enfrente.
        const BetTree& bt = S.tree().ctx[0].tree;
        const Node& r = bt.nodes[static_cast<size_t>(bt.root)];
        std::string apu;
        for (int a = 0; a < r.num_actions; ++a)
            if (bt.act(r, a).kind == AK_BET) apu = bt.act(r, a).code;
        if (!truth("the root has a bet", !apu.empty(), "no bet")) return;

        S.go_root();
        if (!truth("cd X", S.go("X", e), e)) return;
        const double tras_pass  = Ver::eq(S, "AA", true);
        const double vieja_pass = Ver::eq(S, "AA", false);
        S.go_root();
        if (!truth("cd to the bet", S.go(apu, e), e)) return;
        const double tras_apuesta = Ver::eq(S, "AA", true);
        const double de_partida   = Ver::eq(S, "AA", false);

        if (!truth("the four numbers come back",
                   tras_pass > 0 && tras_apuesta > 0 && de_partida > 0 && vieja_pass > 0,
                   "no")) return;

        // La vieja es la misma mires donde mires, y eso es exactamente lo que
        // la hacia inutil a partir del primer nodo.
        close_to("the starting equity is the same wherever you look",
                 de_partida, vieja_pass, 1e-9);
        truth("facing a bet the equity drops", tras_apuesta < tras_pass - 1.0,
              "bet " + std::to_string(tras_apuesta) + " vs check " + std::to_string(tras_pass));
        truth("and it is not the starting one any more",
              std::fabs(tras_apuesta - de_partida) > 1.0,
              "still " + std::to_string(tras_apuesta));
    }

    // El all-in como subida solo existe si hay lista de subidas.
    //
    // El interruptor de all-in anade el all-in A LA LISTA DE SUBIDAS de ese
    // jugador; sin lista no hay donde anadirlo y no se inventa una subida que
    // no existe. Es la regla de la referencia, leida contrastando dos nodos suyos
    // del mismo arbol: en el river OOP tiene subidas 3x y el interruptor
    // puesto, y ofrece b114 y b220; IP tiene el interruptor puesto y ninguna
    // subida, y solo ofrece pagar o tirarse.
    //
    // Sin esto teniamos ocho nodos de mas en un arbol de 611, y un all-in de
    // mas cambia el juego en toda la rama que cuelga de el. Y la suite entera
    // pasaba igual: ninguno de sus spots tenia el interruptor puesto con la
    // lista vacia, asi que no distinguia las dos cosas.
    void an_allin_raise_needs_a_raise_list(Session& S) {
        std::string e;
        if (!truth("allin-raise spot builds", spot(S, "Ah9h4h", e), e)) return;

        struct Ver {
            // Cuantas acciones tiene el que enfrenta la apuesta del flop.
            static int acciones(Session& S) {
                const BetTree& bt = S.tree().ctx[0].tree;
                const Node& r = bt.nodes[static_cast<size_t>(bt.root)];
                for (int a = 0; a < r.num_actions; ++a)
                    if (bt.act(r, a).kind == AK_BET)
                        return bt.nodes[static_cast<size_t>(bt.child(r, a))].num_actions;
                return -1;
            }
        };

        // Con el interruptor puesto y SIN subidas: tirarse y pagar, nada mas.
        if (!truth("no raise sizes", S.set_sizings(false, ST_FLOP,
                                                   std::vector<Sizing>(), e), e)) return;
        if (!truth("allin on for IP", S.set_allin(ST_FLOP, true, e), e)) return;
        same("with no raise list, facing a bet is fold or call", Ver::acciones(S), 2);

        // Y con una lista, el all-in aparece: tirarse, pagar, subir, all-in.
        std::vector<Sizing> tres;
        if (!truth("3x parses", parse_sizings("3x", tres, e), e)) return;
        if (!truth("3x is set", S.set_sizings(false, ST_FLOP, tres, e), e)) return;
        const int con = Ver::acciones(S);
        truth("with a raise list, the all-in shows up", con > 2,
              "solo " + std::to_string(con) + " acciones");
    }

    // La mejor respuesta, la que se ensena, es la misma que mide la
    // explotabilidad.
    //
    // Dos propiedades que no puede fingir:
    //
    //   1. NUNCA vale menos que seguir la solucion. Es una mejor respuesta: si
    //      saliera peor en alguna mano, estaria eligiendo mal, porque entre sus
    //      opciones esta la que juega la solucion.
    //   2. Lo que gana tiene que caber DENTRO de la explotabilidad de la raiz.
    //      La explotabilidad es lo que gana el explotador en todo el arbol; lo
    //      de un nodo suelto no puede pasarse de eso.
    //
    // Sin la primera, `br_at` podria estar devolviendo cualquier cosa con buena
    // pinta. Sin la segunda, podria estar midiendo un arbol distinto.
    void the_best_response_is_a_best_response(Session& S) {
        std::string e;
        if (!truth("best-response spot builds", spot(S, "Ah9h4hKd2s", e), e)) return;
        S.solve(600, 0);
        DCFRSolver& sol = *S.solver();
        const double expl = sol.exploitability();

        const int root = S.tree().ctx[0].tree.root;
        std::vector<double> vals;
        int A = 0;
        if (!truth("the root has a best response", sol.br_at(0, root, 0, vals, A),
                   "br_at said no")) return;
        const NodeStats N = gather(sol, 0, root, 0, S.deal().identity());
        if (!truth("and the node reads", N.ok)) return;
        same("one value per action", A, N.A);

        const int nh = S.deal().num();
        int peor = -1;
        double peor_d = 0.0, mayor = 0.0;
        double suma = 0.0, peso = 0.0;
        for (int h = 0; h < nh; ++h) {
            const double w = N.weight[static_cast<size_t>(h)];
            const double c = N.v.compat[static_cast<size_t>(h)];
            if (w <= 1e-9 || c <= 1e-12) continue;
            int mejor = 0;
            for (int a = 1; a < A; ++a)
                if (vals[static_cast<size_t>(a) * nh + h] >
                    vals[static_cast<size_t>(mejor) * nh + h]) mejor = a;
            const double br = vals[static_cast<size_t>(mejor) * nh + h] / c + cfg::POT0 * 0.5;
            const double gana = br - N.ev_node(h);
            if (gana < peor_d) { peor_d = gana; peor = h; }
            if (gana > mayor) mayor = gana;
            peso += w;
            suma += w * (gana > 0.0 ? gana : 0.0);
        }
        truth("it never does worse than the solution",
              peor_d > -1e-6,
              peor < 0 ? "" : (card_str(S.deal().combos[static_cast<size_t>(peor)].c1) +
                               card_str(S.deal().combos[static_cast<size_t>(peor)].c2) +
                               " loses " + std::to_string(-peor_d)));
        truth("and what it gains fits inside the exploitability",
              peso > 1e-9 && suma / peso <= expl + 1e-6,
              "gains " + std::to_string(peso > 1e-9 ? suma / peso : 0.0) +
              " but the whole tree is only worth " + std::to_string(expl));
        truth("and it is not trivially zero everywhere", mayor > 0.0,
              "every hand gained exactly nothing, which means it is not looking");
    }

    // Las categorias de mano hecha son un hecho de las cartas, no un corte.
    //
    // Aqui no hay umbrales que discutir -- eso es justo lo que se quito cuando
    // desaparecieron NUTS/VALUE/BC/AIR, que eran tres numeros inventados. Un
    // trio es un trio. Asi que se puede comprobar contra casos escritos a mano,
    // que es la unica forma de que la prueba no sea un espejo del codigo.
    //
    // Y se comprueba que el reparto NO PIERDE NADA: las categorias tienen que
    // sumar exactamente el rango del nodo. Una categoria que se coma combos, o
    // que los cuente dos veces, es un mapa que miente.
    // Los nombres de las familias son los que la referencia ENSENA, letra por letra.
    //
    // Y los ensena con guion bajo -- `2nd_pair`, `3rd_pair` -- aunque su propio
    // `show_category_names` los devuelva con guion normal, `2nd-pair`. Ni ella
    // se pone de acuerdo consigo misma, y aqui se elige lo que sale en su Range
    // Explorer, que es la pantalla contra la que uno compara.
    //
    // Se fija tambien el ORDEN, que es como se lee un rango de arriba abajo, y
    // que ningun nombre lleve " + ": eso seria una familia cruzada, y la referencia no
    // tiene ninguna. En su lenguaje de filtros "un set con proyecto de color" se
    // pide con `set & flush_draw`.
    void the_category_names_are_the_ones_everyone_shows(Session& S) {
        (void)S;
        static const char* const HECHA[] = {
            "straight_flush", "quads", "top_fullhouse", "fullhouse", "flush",
            "straight", "set", "trips", "two_pair", "overpair", "top_pair",
            "underpair", "2nd_pair", "3rd_pair", "low_pair", "ace_high",
            "king_high", "nothing"
        };
        static const char* const PROY[] = {
            "no_draw", "4out_straight_draw", "8out_straight_draw",
            "flush_draw", "combo_draw"
        };
        same("there are as many made-hand names as categories",
             static_cast<long long>(sizeof HECHA / sizeof HECHA[0]), MC_COUNT);
        same("and as many draw names",
             static_cast<long long>(sizeof PROY / sizeof PROY[0]), DR_COUNT);
        int mal = 0;
        std::string donde;
        for (int i = 0; i < MC_COUNT; ++i)
            if (std::string(MC_NAME[i]) != HECHA[i]) {
                ++mal;
                donde += std::string(" ") + MC_NAME[i] + "!=" + HECHA[i];
            }
        for (int i = 0; i < DR_COUNT; ++i)
            if (std::string(DR_NAME[i]) != PROY[i]) {
                ++mal;
                donde += std::string(" ") + DR_NAME[i] + "!=" + PROY[i];
            }
        truth("every category name is the standard one", mal == 0, donde);
        bool cruzado = false;
        for (int i = 0; i < MC_COUNT; ++i)
            if (std::string(MC_NAME[i]).find(" + ") != std::string::npos) cruzado = true;
        for (int i = 0; i < DR_COUNT; ++i)
            if (std::string(DR_NAME[i]).find(" + ") != std::string::npos) cruzado = true;
        truth("and none of them is a crossed name", !cruzado);
    }

    // Las filas salen del BOARD, no del rango que lleves.
    //
    // La referencia lista una familia si es posible en ese board, aunque no tengas
    // ninguna: en A-K-6 ensena "set 0.0 combos", que dice algo -- aqui hay sets y
    // tu no llevas ninguno -- y en cambio no ensena color ni escalera, que ahi no
    // existen. Nosotros escondiamos las vacias, y una fila que no esta no se
    // distingue de una que no puede estar.
    void the_rows_come_from_the_board_not_from_the_range(Session& S) {
        std::string e;
        // Arcoiris: con dos cartas tapadas se llega a tres del mismo palo como
        // mucho, asi que ni color ni proyecto de color existen en este board.
        if (!truth("rainbow spot builds", spot(S, "Ah9d4c", e), e)) return;
        // Y un rango SIN una sola pareja servida. El set sigue siendo posible en
        // ese board -- lo da cualquier AA, 99 o 44 -- pero aqui no hay ninguna.
        const char* sin_parejas = "AKo,AQo,AJo,KQo,KJo,QJo";
        if (!truth("a range with no pocket pairs", S.set_range(0, sin_parejas, e), e)) return;
        if (!truth("on both sides", S.set_range(1, sin_parejas, e), e)) return;
        if (!truth("and the tree rebuilds", S.rebuild(e), e)) return;
        S.solve(120, 0);
        const NodeStats N = gather(*S.solver(), 0, S.tree().ctx[0].tree.root, 0,
                                   S.deal().identity());
        if (!truth("the node reads", N.ok)) return;
        const std::vector<ClassAgg> filas =
            aggregate_by_made(*S.solver(), N, S.deal().board);

        bool hay_set = false, set_vacio = false;
        bool hay_color = false, hay_proy_color = false, hay_nada = false;
        for (const ClassAgg& g : filas) {
            const std::string nm = agg_name(g);
            if (nm == "set")        { hay_set = true; set_vacio = g.w <= 1e-9; }
            if (nm == "flush")      hay_color = true;
            if (nm == "flush_draw") hay_proy_color = true;
            if (nm == "nothing")    hay_nada = true;
        }
        truth("the set row is listed with no pocket pairs in the range", hay_set,
              "la fila de set no esta, y en este board el set es posible");
        truth("and it is empty, which is the point", set_vacio);
        truth("a rainbow board has no flush row", !hay_color);
        truth("nor a flush-draw row", !hay_proy_color);
        truth("and the rows that do have hands are still there", hay_nada);
    }

    void a_made_hand_is_a_fact(Session& S) {
        std::string e;

        // Board A-9-4 con dos corazones fuera: los casos se eligen para que
        // cada rama del clasificador tenga uno.
        std::vector<int> b;
        if (!truth("made-hand board parses", parse_board("Ah9h4h", b, e), e)) return;
        struct Caso { const char* mano; int esperado; };
        const Caso casos[] = {
            { "AsAd", MC_SET },          // pareja servida que liga: trio
            { "AsKd", MC_TOPPAIR },      // liga la carta mas alta del board
            { "9s8d", MC_SECONDPAIR },   // liga la segunda
            { "4s3d", MC_THIRDPAIR },    // liga la tercera
            // En A-9-4 el KK NO es overpair: el as esta por encima. Lo escribi
            // mal la primera vez y la comprobacion me corrigio. Y ahora ya no es
            // "segunda pareja" tampoco: con la escalera de la referencia, una servida
            // entre la media y la alta tiene escalon propio.
            { "KsKd", MC_UNDERPAIR },
            { "2s2d", MC_LOWPAIR },    // servida por debajo de todo
            { "KsQd", MC_KINGHIGH },     // el rey suelto va aparte, como en la referencia
            { "AsQd", MC_TOPPAIR },      // el as liga, no es "as alto"
            { "KsQs", MC_KINGHIGH },     // dos picas: no hay color con board de corazones
            { "KhQd", MC_KINGHIGH },     // un solo corazon: tampoco
            { "KhQh", MC_FLUSH },        // dos corazones con tres en el board: color
            { "9s4d", MC_TWOPAIR },      // liga dos del board
        };
        for (const Caso& c : casos) {
            const int c1 = parse_card(std::string(c.mano).substr(0, 2));
            const int c2 = parse_card(std::string(c.mano).substr(2, 2));
            if (!truth(std::string("parses ") + c.mano, c1 >= 0 && c2 >= 0)) continue;
            const int got = made_cat(c1, c2, b);
            truth(std::string(c.mano) + " on Ah9h4h is " + MC_NAME[c.esperado],
                  got == c.esperado, std::string("it says ") + MC_NAME[got]);
        }

        // El overpair necesita un board sin nada por encima, asi que va aparte.
        std::vector<int> b2;
        if (truth("second board parses", parse_board("9s4d2c", b2, e), e)) {
            const Caso mas[] = {
                { "KsKd", MC_OVERPAIR },     // por encima de todo el board
                { "AsKd", MC_ACEHIGH },      // nada hecho, pero lleva el as
                { "QsJd", MC_NOTHING },         // nada y sin as
                { "9c9d", MC_SET },
            };
            for (const Caso& c : mas) {
                const int c1 = parse_card(std::string(c.mano).substr(0, 2));
                const int c2 = parse_card(std::string(c.mano).substr(2, 2));
                if (c1 < 0 || c2 < 0) continue;
                const int got = made_cat(c1, c2, b2);
                truth(std::string(c.mano) + " on 9s4d2c is " + MC_NAME[c.esperado],
                      got == c.esperado, std::string("it says ") + MC_NAME[got]);
            }
        }

        // Y el reparto conserva el rango entero.
        if (!truth("made-hand spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(150, 0);
        const NodeStats N = gather(*S.solver(), 0, S.tree().ctx[0].tree.root, 0,
                                   S.deal().identity());
        if (!truth("the node reads", N.ok)) return;
        const std::vector<ClassAgg> filas =
            aggregate_by_made(*S.solver(), N, S.deal().board);
        // Dos bloques que no se cruzan, y CADA UNO reparte el rango entero.
        // Antes habia una sola suma porque habia una fila por cada par (mano
        // hecha, proyecto); eso convertia 17 familias en 54 filas y no es lo que
        // hace la referencia -- el suyo son dos listas independientes.
        double suma_hecha = 0.0, suma_proy = 0.0;
        int n_hecha = 0, n_proy = 0;
        for (const ClassAgg& g : filas) {
            if (g.kind == AG_DRAW) { suma_proy += g.w; ++n_proy; }
            else                   { suma_hecha += g.w; ++n_hecha; }
        }
        close_to("the made-hand block holds the whole range and no more",
                 suma_hecha, N.wtot, 1e-9);
        close_to("and the draw block holds it too, on its own",
                 suma_proy, N.wtot, 1e-9);
        truth("and there is more than one of each", n_hecha > 1 && n_proy > 1,
              std::to_string(n_hecha) + " de mano hecha, " +
              std::to_string(n_proy) + " de proyecto");

        // Y lo que se ensena con la palabra "combos" al lado tiene que ser
        // combos.
        //
        // La columna llevaba g.w, que es masa de PAREJAS -- mi alcance por el
        // del rival -- y en un rango de setecientos combos sacaba 88.889. Es el
        // mismo error que ya se arreglo en los botones de accion y que no se
        // propago aqui, asi que ahora lo sujeta una cuenta: el reparto por
        // categorias suma los combos del nodo, y los combos del nodo son los
        // que hay en su lista.
        {
            double suman = 0.0;
            for (const ClassAgg& g : filas)
                if (g.kind == AG_MADE)
                    suman += (N.wtot > 1e-12 ? g.w / N.wtot : 0.0) * N.combos();
            close_to("the made-hand categories add up to the combos at the node",
                     suman, N.combos(), 1e-6);
            // Y ese total es de verdad un recuento de manos: en la raiz, el
            // rango entero. Nunca la masa de parejas, que es mucho mayor.
            truth("and that is a hand count, not a pair mass",
                  N.combos() < N.wtot * 0.5,
                  "combos " + std::to_string(N.combos()) +
                  " frente a wtot " + std::to_string(N.wtot));
            double vivos = 0.0;
            for (int h = 0; h < N.nh; ++h)
                if (S.solver()->range(N.player)[static_cast<size_t>(h)] > 0.0)
                    vivos += S.solver()->range(N.player)[static_cast<size_t>(h)];
            close_to("and at the root it is the whole range", N.combos(), vivos, 1e-6);
        }

        // Y el board del NODO, no el de partida.
        //
        // Esto es lo que se me escapo la primera vez: made_cat estaba bien y la
        // agregacion estaba bien, pero quien las llamaba pasaba el board de tres
        // cartas en un nodo de turn. En Ah9h4h/Kd un AK liga el as y el rey, o
        // sea doble pareja, y seguia figurando como pareja top -- la columna
        // decia una cosa que no era, que es peor que no decir nada.
        {
            const int c1 = parse_card("As"), c2 = parse_card("Kd");
            std::vector<int> b4;
            if (truth("four-card board parses", parse_board("Ah9h4hKd", b4, e), e))
                truth("AsKd on Ah9h4hKd is two pair",
                      made_cat(c1, c2, b4) == MC_TWOPAIR,
                      std::string("it says ") + MC_NAME[made_cat(c1, c2, b4)]);
        }
        S.go_root();
        std::string ge;
        const bool bajo = S.go("X", ge) && S.go("X", ge) && S.go("Kd", ge);
        if (truth("the session reaches a turn node", bajo, ge)) {
            const std::vector<int> baq = S.board_here();
            truth("and the board there is four cards", baq.size() == 4,
                  std::to_string(baq.size()) + " cartas");
            truth("the fourth being the one that was dealt",
                  baq.size() == 4 && baq[3] == parse_card("Kd"),
                  baq.size() == 4 ? card_str(baq[3]) : "");
            long long tinst = 0;
            int tperm = S.deal().identity();
            S.cur_addr(tinst, tperm);
            const NodeStats TN = gather(*S.solver(), S.cur_ctx(), S.cur_node(),
                                        tinst, tperm);
            if (truth("the turn node reads", TN.ok)) {
                const std::vector<ClassAgg> tf =
                    aggregate_by_made(*S.solver(), TN, baq);
                bool hay_dos_pares = false;
                for (const ClassAgg& g : tf) if (g.cls == MC_TWOPAIR) hay_dos_pares = true;
                // Con el board de tres cartas no existe ninguna doble pareja en
                // este rango: si aparece, el turn se esta teniendo en cuenta.
                truth("and the king on the turn makes two pair exist", hay_dos_pares,
                      "ni una doble pareja, el turn no se esta contando");
            }
        }
        S.go_root();
    }

    // La frecuencia de una linea es una probabilidad, y eso obliga a dos cosas
    // que se pueden comprobar sin saber nada del juego.
    //
    // La primera es que se CONSERVA: la masa que llega a un nodo se reparte
    // entera entre sus hijos, y ni una ficha se queda por el camino ni aparece
    // de la nada. Eso vale igual en un nodo de decision, donde la estrategia
    // suma uno, que al cruzar un reparto de carta, donde las 45 cartas que
    // pueden salir suman uno entre todas. El reparto es donde se cae: lleva un
    // divisor (las cartas posibles) y un multiplicador (el tamano de la orbita)
    // y si falta cualquiera de los dos la suma se va por 44.
    //
    // La segunda es que el mismo numero se puede calcular por otro camino. El
    // recorrido de frecuencias baja multiplicando estrategias; gather() lo saca
    // de reach_at() y compat_counts(). En un nodo de flop las dos cosas tienen
    // que dar lo mismo, y no comparten una linea de codigo.
    void a_line_frequency_is_a_probability(Session& S) {
        std::string e;
        if (!truth("line-freq spot builds", spot(S, "Ah9h4h", e), e)) return;
        S.solve(200, 0);

        std::vector<std::pair<std::string, double>> fr;
        S.solver()->line_freqs(fr);
        if (!truth("there are lines with a frequency", fr.size() > 4)) return;

        std::map<std::string, double> m;
        for (const std::pair<std::string, double>& x : fr) m[x.first] = x.second;

        // La raiz existe y es de donde sale todo.
        std::map<std::string, double>::const_iterator raiz = m.find("r:0");
        if (!truth("the root is one of the lines", raiz != m.end())) return;
        const double R = raiz->second;
        if (!truth("and the root is reachable", R > 1e-9)) return;

        // Los nombres son los mismos que da `lines`, o las dos listas no se
        // pueden cruzar y la funcion no sirve para nada.
        const std::vector<std::string> ls = all_lines(S.tree());
        size_t sin_freq = 0, sin_linea = 0;
        for (const std::string& l : ls) if (!m.count(l)) ++sin_freq;
        std::set<std::string> conj(ls.begin(), ls.end());
        for (const std::pair<std::string, double>& x : fr)
            if (!conj.count(x.first)) ++sin_linea;
        truth("every line has a frequency", sin_freq == 0,
              std::to_string(sin_freq) + " lineas sin frecuencia");
        truth("and every frequency has a line", sin_linea == 0,
              std::to_string(sin_linea) + " frecuencias sin linea");

        // Conservacion: cada nodo con hijos reparte su masa entera.
        std::map<std::string, double> suma_hijos;
        std::map<std::string, int>    num_hijos;
        for (const std::pair<std::string, double>& x : fr) {
            const size_t i = x.first.rfind(':');
            if (i == std::string::npos || i == 0) continue;
            const std::string padre = x.first.substr(0, i);
            if (!m.count(padre)) continue;
            suma_hijos[padre] += x.second;
            num_hijos[padre] += 1;
        }
        int rotos = 0, cruzados = 0;
        double peor = 0.0;
        std::string donde;
        for (std::map<std::string, double>::const_iterator it = suma_hijos.begin();
             it != suma_hijos.end(); ++it) {
            const double p = m[it->first];
            const double d = std::fabs(it->second - p) / (p > 1e-9 ? p : 1.0);
            if (d > peor) { peor = d; donde = it->first; }
            if (d > 1e-9) ++rotos;
            ++cruzados;
        }
        truth("there are chance nodes to cross", cruzados > 6,
              std::to_string(cruzados) + " nodos con hijos");
        truth("the children hold all of the parent and no more", rotos == 0,
              std::to_string(rotos) + " nodos se descuadran, el peor " + donde +
              " por " + std::to_string(peor));

        // Y el mismo numero por el otro camino, en los nodos de flop.
        const std::map<std::pair<int, int>, std::string> nom = node_lines(S.tree());
        int comparados = 0;
        double peor_cruce = 0.0;
        std::string cruce_donde;
        for (size_t nid = 0; nid < S.tree().ctx[0].tree.nodes.size(); ++nid) {
            if (S.tree().ctx[0].tree.nodes[nid].type != NT_DECISION) continue;
            std::map<std::pair<int, int>, std::string>::const_iterator it =
                nom.find(std::make_pair(0, static_cast<int>(nid)));
            if (it == nom.end() || !m.count(it->second)) continue;
            const NodeStats N = gather(*S.solver(), 0, static_cast<int>(nid), 0,
                                      S.deal().identity());
            if (!N.ok) continue;
            const double suyo = node_reach_pct(*S.solver(), N);
            const double mio  = 100.0 * m[it->second] / R;
            const double d = std::fabs(mio - suyo);
            if (d > peor_cruce) { peor_cruce = d; cruce_donde = it->second; }
            ++comparados;
        }
        truth("there are flop nodes to cross-check", comparados > 2,
              std::to_string(comparados) + " nodos comparados");
        truth("and gather says the same at every one of them", peor_cruce < 1e-6,
              cruce_donde + " se separa por " + std::to_string(peor_cruce) +
              " puntos");

        // La pagina ya no las ensena -- el panel de frecuencia de linea se quito
        // porque no se usaba -- asi que aqui no hay cable que comprobar. El
        // numero sigue vivo en la consola, con `freqs`, y es el de arriba.
    }

    // El campo de subidas solo acepta multiplos, y eso arregla la coma.
    //
    // Antes admitia las dos cosas: "50" era medio bote encima de la igualada y
    // "3x" tres veces lo que tienes delante. Dos cuentas distintas en el mismo
    // sitio, con una letra por toda senal.
    //
    // Prohibir el porcentaje aqui desambigua la coma, que es lo que de verdad
    // se gana. "2,5x" tenia dos lecturas legales -- lista de "2" y "5x", o el
    // decimal 2.5x -- y ahora solo una, porque "2" a secas no es una subida.
    //
    // Y "min" no es un multiplo, que es por lo que tiene nombre propio: sobre
    // una primera apuesta la subida minima es 2x, pero sobre una subida ya no.
    void a_raise_is_a_multiple_or_the_minimum(Session& S) {
        std::string e;

        struct Ok { const char* txt; int cuantos; double primero; bool minimo; };
        const Ok bien[] = {
            { "3x",      1, 3.0, false },
            { "2x,3x",   2, 2.0, false },
            { "2,5x",    1, 2.5, false },   // la coma es decimal: ya no hay duda
            { "2.5x",    1, 2.5, false },   // y con punto, lo mismo
            { "min",     1, 1.0, true  },
            { "min,5x",  2, 1.0, true  },   // lo que en la referencia se escribe "2,5x"
        };
        for (const Ok& c : bien) {
            std::vector<Sizing> v;
            const std::string tag = std::string("subidas '") + c.txt + "' ";
            if (!truth(tag + "se aceptan", parse_raises(c.txt, v, e), e)) continue;
            same(tag + "y son las que pone", static_cast<long long>(v.size()), c.cuantos);
            if (v.empty()) continue;
            must_be(tag + "con el valor correcto", v[0].v, c.primero, 1e-12);
            truth(tag + "y el minimo marcado o no", v[0].minraise == c.minimo);
        }
        {
            std::vector<Sizing> v;
            truth("subidas 'none' las quita",
                  parse_raises("none", v, e) && v.empty(), e);
        }

        // Un porcentaje en el campo de subidas se rechaza, y dice como se
        // escribe. Es el cambio entero, y sin esto no hay nada que lo sujete.
        const char* mal[] = { "50", "300", "pot", "0.5", "33,66" };
        for (const char* c : mal) {
            std::vector<Sizing> v;
            std::string why;
            const std::string tag = std::string("subidas '") + c + "' ";
            if (!truth(tag + "se rechazan", !parse_raises(c, v, why), "se acepto")) continue;
            truth(tag + "y dicen que van en multiplos",
                  why.find("multiplos") != std::string::npos, "dijo '" + why + "'");
        }

        // Y lo que separa `min` de un multiplo: el minimo legal cambia con el
        // nivel. Sobre una apuesta de 12 la minima sube a 24, y sobre esa a 36
        // -- no a 48, que es lo que daria 2x. Si `min` fuera un multiplo, esto
        // saldria 48 y nadie se enteraria hasta comparar arboles con la referencia.
        if (!truth("raise-scale spot builds", spot(S, "Ah9h4h", e), e)) return;
        std::vector<Sizing> b;
        if (!truth("una apuesta de 12", parse_sizings("60", b, e) &&
                                        S.set_sizings(true, ST_FLOP, b, e), e)) return;
        std::vector<Sizing> r;
        if (!truth("y subidas minimas", parse_raises("min", r, e) &&
                                        S.set_sizings(false, ST_FLOP, r, e), e)) return;
        expect_actions("la primera minima sube a 24", S.tree(), "R", "R/B12", "F|C|R24");
        expect_actions("y la segunda a 36, no a 48", S.tree(), "R", "R/B12/R24", "F|C|R36");
        if (!truth("y ahora 2x", parse_raises("2x", r, e) &&
                                 S.set_sizings(false, ST_FLOP, r, e), e)) return;
        expect_actions("2x tambien sube a 24 la primera", S.tree(), "R", "R/B12", "F|C|R24");
        // Con la cuenta de la referencia, "2x" es subir POR dos veces la igualada, o sea
        // la minima legal, y eso vale en cualquier nivel y no solo sobre una
        // primera apuesta. Antes daba 48 aqui, porque se multiplicaba el TOTAL
        // del rival en vez de lo que hay que pagar.
        expect_actions("y a 36 la segunda, que es otra vez la minima",
                       S.tree(), "R", "R/B12/R24", "F|C|R36");

        // Y la cuenta entera, contra los numeros de la referencia.
        //
        // Salen de sus 611 lineas: los tres unicos sitios de aquel arbol donde
        // su "5x" no cae en la minima ni en el all-in. Los tres cuadran con
        // "lo que llevo + 5 veces lo que pago" y ninguno con "5 veces su total",
        // que es lo que haciamos. Se separaban solo del 3-bet en adelante,
        // porque en la primera subida uno no lleva nada puesto y las dos cuentas
        // dan lo mismo -- por eso 610 de 611 nodos coincidian.
        if (!truth("un stack largo para que la cadena quepa", S.set_stack(220.0, e), e)) return;
        if (!truth("y sin all-in que la corte", S.set_allin_thresh(1.0, e), e)) return;
        cfg::MERGE_PCT = 0.0;
        if (!truth("ni fusion de tamanos", S.rebuild(e), e)) return;
        if (!truth("minima y 5x a la vez", parse_raises("min,5x", r, e) &&
                                           S.set_sizings(false, ST_FLOP, r, e), e)) return;
        struct Nivel { const char* path; const char* espera; int mio, suyo, sale; };
        const Nivel cadena[] = {
            { "R/B12",                   "F|C|R24|R60",   0, 12,  60 },
            { "R/B12/R24",               "F|C|R36|R72",  12, 24,  72 },
            { "R/B12/R24/R36",           "F|C|R48|R84",  24, 36,  84 },   // medido
            { "R/B12/R24/R36/R48/R60",   "F|C|R72|R108", 48, 60, 108 },   // medido
            { "R/B12/R24/R36/R48/R60/R72/R84", "F|C|R96|R132", 72, 84, 132 },  // medido
        };
        for (const Nivel& n : cadena) {
            char q[160];
            std::snprintf(q, sizeof q, "llevando %d contra %d, el 5x sube a %d",
                          n.mio, n.suyo, n.sale);
            expect_actions(q, S.tree(), "R", n.path, n.espera);
        }
        cfg::MERGE_PCT = 0.10;
    }

    // Un proyecto es de la MANO, y solo hasta el river.
    //
    // Sin esto, en un board de dos del mismo palo un proyecto de color nut
    // figuraba como "carta alta" al lado del 72o, y la fila decia que un tercio
    // del rango era aire cuando ese tercio apostaba el 78% de las veces. Eso no
    // es informacion, es ruido con etiqueta.
    //
    // Las dos reglas que lo sujetan todo: que la mano aporte -- si el board
    // lleva cuatro a color o cuatro a escalera, lo tienen todos y no separa a
    // nadie -- y que en el river no haya proyecto, porque ya no sale carta.
    // Las categorias son las de la referencia, y esta tabla sale de ella.
    //
    // Las nuestras estaban inventadas por mi: "pareja floja", "segunda pareja"
    // contada por cuantas cartas del board superaba una servida... una escalera
    // razonable y que no es la de nadie. Trae tres comandos que su manual no
    // menciona: `show_category_names` suelta las dos tablas de nombres entera,
    // `show_cats_pp <board>` las reparte con nombre por los 1326 combos, y
    // `show_categories <c1> <c2> <c3>` -- con las cartas SEPARADAS, que con el
    // board pegado contesta que el argumento esta mal -- devuelve dos filas de
    // 1326 indices, mano hecha y proyecto.
    //
    // Su tabla de mano hecha, de mas floja a mas fuerte:
    //
    //    nothing king_high ace_high low_pair 3rd-pair 2nd-pair underpair
    //    top_pair top_pair_tp overpair two_pair trips set straight flush
    //    fullhouse top_fullhouse quads straight_flush
    //
    // y la de proyectos:
    //
    //    no_draw 4out_straight_draw 8out_straight_draw flush_draw combo_draw
    //
    // De los diecinueve de mano hecha se reparten dieciocho. `top_fullhouse`
    // costo encontrarlo: 652 boards al azar y ni uno, porque hace falta una FORMA
    // que el azar no da -- trio con la pareja mas alta que el trio, 2-2-2-3-3 --.
    // Y `top_pair_tp` no ha salido todavia: 3592 boards barridos por forma, mas
    // 1692 al azar, mas 40 hechos a mano para el -- board emparejado por debajo de
    // la carta alta, que es donde "pareja top" y "dos parejas" coinciden --, y en
    // ninguno. Lo tiene en la tabla y no lo reparte. Queda dicho como lo que es:
    // no visto, no que no exista.
    //
    // Los casos de aqui se GENERARON de su respuesta sobre 34 boards elegidos
    // para que salga todo -- flop monocolor, arcoiris, emparejado y de trio, turn
    // a cuatro palos y a cuatro cartas de escalera, river con color, escalera,
    // full, poker y trio-con-pareja-encima en la mesa --, con dos muestreos que
    // se suman:
    //
    //    por cajon   tres manos de cada pareja (mano hecha, proyecto) que la referencia
    //                reparta en ese board, repartidas a lo largo del grupo
    //    por forma   las manos que estan en las fronteras de su escalera: las que
    //                ligan dos rangos del board (ahi se decide two_pair), las 13
    //                servidas, una por cada rango del board, los conectores y los
    //                de un hueco (ahi se decide la escalera), y una y dos cartas
    //                del palo del board (ahi el color)
    //
    // El segundo hace falta: sin el, una mutacion que reparta two_pair en board
    // emparejado no la caza nadie, porque en 9-7-2-2 la mano que lo destapa es el
    // 9-7 y por abecedario no entra.
    //
    // Y contra el mismo la referencia, fuera de esta tabla, combo a combo y en las dos
    // mitades:
    //
    //    1.692 boards al azar                    1.911.872 combos   0 diferencias
    //      296 boards por forma de rango y palo    328.301 combos   0 diferencias
    //      459 boards de escalera y de color       508.368 combos   0 diferencias
    //    -------------------------------------------------------------------------
    //    2.447 boards                            2.748.541 combos   0 diferencias
    void the_categories_are_the_ones_everyone_uses(Session& S) {
        (void)S;
        struct Caso { const char* board; const char* mano; int hecho; int proy; };
        static const Caso casos[] = {
            // Ah9h4h
            { "Ah9h4h", "6c2c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "6c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "6c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "7c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "7c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "7c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "7c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "8c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "8c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "8c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "8c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "JcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "JcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "JdTd", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "QcJc", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "QcJd", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "QcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "QcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "Ts8s", MC_NOTHING, DR_NONE },
            { "Ah9h4h", "3c2c", MC_NOTHING, DR_GUT },
            { "Ah9h4h", "3c2d", MC_NOTHING, DR_GUT },
            { "Ah9h4h", "5c3c", MC_NOTHING, DR_GUT },
            { "Ah9h4h", "5c3d", MC_NOTHING, DR_GUT },
            { "Ah9h4h", "5s3s", MC_NOTHING, DR_GUT },
            { "Ah9h4h", "6c2h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "8s2h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "8s3h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "Jh7d", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "Qh2c", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "Qh8s", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "Ts8h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4h", "3c2h", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4h", "3h2c", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4h", "5d3h", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4h", "5s3h", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4h", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "Kd7d", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "KsTs", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h", "Kc2h", MC_KINGHIGH, DR_FLUSH },
            { "Ah9h4h", "Kh2c", MC_KINGHIGH, DR_FLUSH },
            { "Ah9h4h", "Kh6c", MC_KINGHIGH, DR_FLUSH },
            { "Ah9h4h", "Kh8s", MC_KINGHIGH, DR_FLUSH },
            { "Ah9h4h", "KsTh", MC_KINGHIGH, DR_FLUSH },
            { "Ah9h4h", "2d2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h", "3d3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h", "3s3d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h", "2h2c", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4h", "3h3c", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4h", "3s3h", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4h", "4c2c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "4c2d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "4c3c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "4c3d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "5c4c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "5c4d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "6c4c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "6c4d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "7d7c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "7s4s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "8d8c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "Ts4s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h", "4c2h", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4h", "7h7c", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4h", "Th4s", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4h", "9c2c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9c7c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9c7d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9c8d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9s6c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "Jc9c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "Jc9d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "Tc9c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "Tc9d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "Ts9s", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h", "9c2h", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4h", "9s6h", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4h", "Th9s", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4h", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h", "KdKc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h", "TsTd", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h", "JhJc", MC_UNDERPAIR, DR_FLUSH },
            { "Ah9h4h", "QhQc", MC_UNDERPAIR, DR_FLUSH },
            { "Ah9h4h", "TsTh", MC_UNDERPAIR, DR_FLUSH },
            { "Ah9h4h", "Ac2c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "AcKc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "AcKd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "AcQc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "AcQd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "Ad8c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "AsTs", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h", "Ac2h", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4h", "Ad8h", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4h", "AsTh", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4h", "9c4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "9c4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "Ac4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "Ac4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "Ac9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "Ac9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "As9s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h", "4d4c", MC_SET, DR_NONE },
            { "Ah9h4h", "9d9c", MC_SET, DR_NONE },
            { "Ah9h4h", "9s9c", MC_SET, DR_NONE },
            { "Ah9h4h", "AdAc", MC_SET, DR_NONE },
            { "Ah9h4h", "AsAd", MC_SET, DR_NONE },
            { "Ah9h4h", "3h2h", MC_FLUSH, DR_NONE },
            { "Ah9h4h", "Jh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4h", "Kh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4h", "Kh5h", MC_FLUSH, DR_NONE },
            { "Ah9h4h", "Qh3h", MC_FLUSH, DR_NONE },
            { "Ah9h4h", "Th8h", MC_FLUSH, DR_NONE },
            // 9h7h2c
            { "9h7h2c", "4c3c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "4c3d", MC_NOTHING, DR_NONE },
            { "9h7h2c", "5c3c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "5c3d", MC_NOTHING, DR_NONE },
            { "9h7h2c", "5c4c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "5c4d", MC_NOTHING, DR_NONE },
            { "9h7h2c", "6c4c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "6c4d", MC_NOTHING, DR_NONE },
            { "9h7h2c", "Js5c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "Qc3c", MC_NOTHING, DR_NONE },
            { "9h7h2c", "QcJc", MC_NOTHING, DR_NONE },
            { "9h7h2c", "QcJd", MC_NOTHING, DR_NONE },
            { "9h7h2c", "QcTc", MC_NOTHING, DR_NONE },
            { "9h7h2c", "QcTd", MC_NOTHING, DR_NONE },
            { "9h7h2c", "Ts5s", MC_NOTHING, DR_NONE },
            { "9h7h2c", "6c5c", MC_NOTHING, DR_GUT },
            { "9h7h2c", "6c5d", MC_NOTHING, DR_GUT },
            { "9h7h2c", "JcTc", MC_NOTHING, DR_GUT },
            { "9h7h2c", "JcTd", MC_NOTHING, DR_GUT },
            { "9h7h2c", "JcTs", MC_NOTHING, DR_GUT },
            { "9h7h2c", "Ts6s", MC_NOTHING, DR_GUT },
            { "9h7h2c", "8c6c", MC_NOTHING, DR_OESD },
            { "9h7h2c", "8c6d", MC_NOTHING, DR_OESD },
            { "9h7h2c", "Tc8c", MC_NOTHING, DR_OESD },
            { "9h7h2c", "Tc8d", MC_NOTHING, DR_OESD },
            { "9h7h2c", "Ts8s", MC_NOTHING, DR_OESD },
            { "9h7h2c", "4h3h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c", "Jh6h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c", "Th5h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c", "6h5h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c", "Jh8h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c", "Th8h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "Kc4c", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KcJc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KcQc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KdTc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "9h7h2c", "Kh3h", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c", "Kh8h", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c", "KhTh", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "Ac5c", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AcKc", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AcKd", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AcQc", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AdQs", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "9h7h2c", "Ah3h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c", "Ah4h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c", "Ah8h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c", "AhTh", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c", "3c2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "3c2h", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "3h2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "4c2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "4c2h", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "6s6h", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "Ac2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "Ah2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "Kc2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "Kh2d", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "Ts2s", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c", "3h2h", MC_THIRDPAIR, DR_FLUSH },
            { "9h7h2c", "Ah2h", MC_THIRDPAIR, DR_FLUSH },
            { "9h7h2c", "Qh2h", MC_THIRDPAIR, DR_FLUSH },
            { "9h7h2c", "Th2h", MC_THIRDPAIR, DR_FLUSH },
            { "9h7h2c", "7c3c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "7c5c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "7c5d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "7c6c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "7c6d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "8c7c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "8c7d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "Ac7c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "Ts7s", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c", "8s8c", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c", "8s8h", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c", "9c3c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "9c3h", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "9c8c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "9c8d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "9d3c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "9d4c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Ac9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Ac9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Ah9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Jc9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Jc9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Kc9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Kh9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Tc9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Tc9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "Ts9s", MC_TOPPAIR, DR_NONE },
            { "9h7h2c", "AdAc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "JdJc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "KdKc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "KsKc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "QdQc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "TdTc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "TsTh", MC_OVERPAIR, DR_NONE },
            { "9h7h2c", "7c2d", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "7c2h", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "9c2d", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "9c2h", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "9c7c", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "9c7d", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "9s7s", MC_TWOPAIR, DR_NONE },
            { "9h7h2c", "2h2d", MC_SET, DR_NONE },
            { "9h7h2c", "7d7c", MC_SET, DR_NONE },
            { "9h7h2c", "7s7c", MC_SET, DR_NONE },
            { "9h7h2c", "9d9c", MC_SET, DR_NONE },
            { "9h7h2c", "9s9d", MC_SET, DR_NONE },
            // Ks8d3c
            { "Ks8d3c", "4c2c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "4c2d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "5c4c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "5c4d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "6c4c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "6c4d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "6c5c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "6c5d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "7c5c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "7c5d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "7c6c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "7c6d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "9c2d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "9c7c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "9c7d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "9d2s", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Jc9c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Jc9d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "JcTc", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "JcTd", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Jd9c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Js2s", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "QcJc", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "QcJd", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "QcTc", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "QcTd", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Qd2d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Qs2c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Qs9d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Tc9c", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Tc9d", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Ts9s", MC_NOTHING, DR_NONE },
            { "Ks8d3c", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "AcQc", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "Ad9c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "As2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "As4s", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "As9d", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "Ks8d3c", "2d2c", MC_LOWPAIR, DR_NONE },
            { "Ks8d3c", "2s2c", MC_LOWPAIR, DR_NONE },
            { "Ks8d3c", "2s2h", MC_LOWPAIR, DR_NONE },
            { "Ks8d3c", "3d2c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "3d2d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "3s2c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "4c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "4c3h", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "5c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "5c3h", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "7d7c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "9c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "9d3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "Qs3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "Ts3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3c", "8c2c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "8c6c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "8c6d", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "8c7c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "8c7d", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "9c8h", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "Tc8c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "Tc8h", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "Ts8s", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3c", "9d9c", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3c", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3c", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3c", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3c", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3c", "AcKc", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "AcKd", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "Kc2c", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "KcJc", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "KcJd", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "KcQc", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "KcQd", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "Kd2c", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "Kd6c", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "Kd9c", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "KhTs", MC_TOPPAIR, DR_NONE },
            { "Ks8d3c", "AdAc", MC_OVERPAIR, DR_NONE },
            { "Ks8d3c", "AsAc", MC_OVERPAIR, DR_NONE },
            { "Ks8d3c", "AsAh", MC_OVERPAIR, DR_NONE },
            { "Ks8d3c", "8c3d", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "8c3h", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kc3d", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kc3h", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kc8c", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kc8h", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kd3d", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "Kh8s", MC_TWOPAIR, DR_NONE },
            { "Ks8d3c", "3h3d", MC_SET, DR_NONE },
            { "Ks8d3c", "8h8c", MC_SET, DR_NONE },
            { "Ks8d3c", "8s8c", MC_SET, DR_NONE },
            { "Ks8d3c", "KdKc", MC_SET, DR_NONE },
            { "Ks8d3c", "KhKd", MC_SET, DR_NONE },
            // QsJhTd
            { "QsJhTd", "3c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "3c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "3h2c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "3s2c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "4c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "4c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "4c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "4c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "5c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "5c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "5c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "5c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "6c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "6c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "6c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "6c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "6d4c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "7c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "7c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "7c6c", MC_NOTHING, DR_NONE },
            { "QsJhTd", "7c6d", MC_NOTHING, DR_NONE },
            { "QsJhTd", "7s6s", MC_NOTHING, DR_NONE },
            { "QsJhTd", "8c2c", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8c6c", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8c6d", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8c7c", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8c7d", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8h2c", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8h2s", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8h3s", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8s2h", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8s3h", MC_NOTHING, DR_GUT },
            { "QsJhTd", "8s7s", MC_NOTHING, DR_GUT },
            { "QsJhTd", "9c2c", MC_NOTHING, DR_OESD },
            { "QsJhTd", "9c7c", MC_NOTHING, DR_OESD },
            { "QsJhTd", "9c7d", MC_NOTHING, DR_OESD },
            { "QsJhTd", "9h2c", MC_NOTHING, DR_OESD },
            { "QsJhTd", "9s7s", MC_NOTHING, DR_OESD },
            { "QsJhTd", "Kc2c", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Kh2c", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Kh3h", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Kh8s", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Ks2c", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Ks3s", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Ks8h", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Ks8s", MC_KINGHIGH, DR_OESD },
            { "QsJhTd", "Ac2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "Ah2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "Ah4h", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "As2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "As4s", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "As7s", MC_ACEHIGH, DR_GUT },
            { "QsJhTd", "Ac8c", MC_ACEHIGH, DR_OESD },
            { "QsJhTd", "Ah8c", MC_ACEHIGH, DR_OESD },
            { "QsJhTd", "Ah8s", MC_ACEHIGH, DR_OESD },
            { "QsJhTd", "As8h", MC_ACEHIGH, DR_OESD },
            { "QsJhTd", "As9s", MC_ACEHIGH, DR_OESD },
            { "QsJhTd", "2d2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "2h2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "2s2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "3d3c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "4d4c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "5d5c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "6d6c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "7d7c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "7s7h", MC_LOWPAIR, DR_NONE },
            { "QsJhTd", "8d8c", MC_LOWPAIR, DR_GUT },
            { "QsJhTd", "8s8c", MC_LOWPAIR, DR_GUT },
            { "QsJhTd", "8s8h", MC_LOWPAIR, DR_GUT },
            { "QsJhTd", "9d9c", MC_LOWPAIR, DR_OESD },
            { "QsJhTd", "9s9c", MC_LOWPAIR, DR_OESD },
            { "QsJhTd", "9s9h", MC_LOWPAIR, DR_OESD },
            { "QsJhTd", "Tc2c", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd", "Th5c", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd", "Ts7s", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd", "AcTc", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd", "Tc8c", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd", "Tc8d", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd", "Ts8s", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd", "KcTc", MC_THIRDPAIR, DR_OESD },
            { "QsJhTd", "Tc9c", MC_THIRDPAIR, DR_OESD },
            { "QsJhTd", "Tc9d", MC_THIRDPAIR, DR_OESD },
            { "QsJhTd", "Ts9s", MC_THIRDPAIR, DR_OESD },
            { "QsJhTd", "Jc2c", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd", "Jd5c", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd", "Js2s", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd", "Js7s", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd", "AcJc", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd", "Jc8c", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd", "Js8s", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd", "Jc9c", MC_SECONDPAIR, DR_OESD },
            { "QsJhTd", "Jc9d", MC_SECONDPAIR, DR_OESD },
            { "QsJhTd", "KcJc", MC_SECONDPAIR, DR_OESD },
            { "QsJhTd", "KcJd", MC_SECONDPAIR, DR_OESD },
            { "QsJhTd", "KsJs", MC_SECONDPAIR, DR_OESD },
            { "QsJhTd", "Qc2c", MC_TOPPAIR, DR_NONE },
            { "QsJhTd", "Qd5c", MC_TOPPAIR, DR_NONE },
            { "QsJhTd", "Qh2h", MC_TOPPAIR, DR_NONE },
            { "QsJhTd", "Qh7s", MC_TOPPAIR, DR_NONE },
            { "QsJhTd", "AcQc", MC_TOPPAIR, DR_GUT },
            { "QsJhTd", "AcQd", MC_TOPPAIR, DR_GUT },
            { "QsJhTd", "Qc8c", MC_TOPPAIR, DR_GUT },
            { "QsJhTd", "Qh8s", MC_TOPPAIR, DR_GUT },
            { "QsJhTd", "KcQc", MC_TOPPAIR, DR_OESD },
            { "QsJhTd", "KcQd", MC_TOPPAIR, DR_OESD },
            { "QsJhTd", "Qc9c", MC_TOPPAIR, DR_OESD },
            { "QsJhTd", "Qh9s", MC_TOPPAIR, DR_OESD },
            { "QsJhTd", "AdAc", MC_OVERPAIR, DR_GUT },
            { "QsJhTd", "AsAc", MC_OVERPAIR, DR_GUT },
            { "QsJhTd", "AsAh", MC_OVERPAIR, DR_GUT },
            { "QsJhTd", "KdKc", MC_OVERPAIR, DR_OESD },
            { "QsJhTd", "KsKc", MC_OVERPAIR, DR_OESD },
            { "QsJhTd", "KsKh", MC_OVERPAIR, DR_OESD },
            { "QsJhTd", "JcTc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "JcTh", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "QcJc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "QcJd", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "QcTc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "QcTh", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "QhTs", MC_TWOPAIR, DR_NONE },
            { "QsJhTd", "JdJc", MC_SET, DR_NONE },
            { "QsJhTd", "QdQc", MC_SET, DR_NONE },
            { "QsJhTd", "QhQc", MC_SET, DR_NONE },
            { "QsJhTd", "ThTc", MC_SET, DR_NONE },
            { "QsJhTd", "TsTh", MC_SET, DR_NONE },
            { "QsJhTd", "9c8c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd", "9c8d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd", "AcKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd", "AcKd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd", "AhKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd", "Ks9s", MC_STRAIGHT, DR_NONE },
            // JhTh9h
            { "JhTh9h", "3c2c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "3c2d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "4c2c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "4c2d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "4c3c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "4c3d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "5c3c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "5c3d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "5c4c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "5c4d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "5s2c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "6c4c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "6c4d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "6c5c", MC_NOTHING, DR_NONE },
            { "JhTh9h", "6c5d", MC_NOTHING, DR_NONE },
            { "JhTh9h", "6s5s", MC_NOTHING, DR_NONE },
            { "JhTh9h", "7c2c", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7c5c", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7c5d", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7c6c", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7c6d", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7d4d", MC_NOTHING, DR_GUT },
            { "JhTh9h", "7s6s", MC_NOTHING, DR_GUT },
            { "JhTh9h", "8c2c", MC_NOTHING, DR_OESD },
            { "JhTh9h", "8c6c", MC_NOTHING, DR_OESD },
            { "JhTh9h", "8c6d", MC_NOTHING, DR_OESD },
            { "JhTh9h", "Qc3d", MC_NOTHING, DR_OESD },
            { "JhTh9h", "Qs7s", MC_NOTHING, DR_OESD },
            { "JhTh9h", "3c2h", MC_NOTHING, DR_FLUSH },
            { "JhTh9h", "3h2c", MC_NOTHING, DR_FLUSH },
            { "JhTh9h", "5h4c", MC_NOTHING, DR_FLUSH },
            { "JhTh9h", "6s5h", MC_NOTHING, DR_FLUSH },
            { "JhTh9h", "7c2h", MC_NOTHING, DR_FLUSH_OESD },
            { "JhTh9h", "8h4s", MC_NOTHING, DR_FLUSH_OESD },
            { "JhTh9h", "8s2h", MC_NOTHING, DR_FLUSH_OESD },
            { "JhTh9h", "8s3h", MC_NOTHING, DR_FLUSH_OESD },
            { "JhTh9h", "Qs7h", MC_NOTHING, DR_FLUSH_OESD },
            { "JhTh9h", "Kc2c", MC_KINGHIGH, DR_GUT },
            { "JhTh9h", "Kd4d", MC_KINGHIGH, DR_GUT },
            { "JhTh9h", "Ks6s", MC_KINGHIGH, DR_GUT },
            { "JhTh9h", "Kc7c", MC_KINGHIGH, DR_OESD },
            { "JhTh9h", "Kd8c", MC_KINGHIGH, DR_OESD },
            { "JhTh9h", "Ks8s", MC_KINGHIGH, DR_OESD },
            { "JhTh9h", "Kc2h", MC_KINGHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Kh2c", MC_KINGHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Kh4d", MC_KINGHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Kh8s", MC_KINGHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Ks8h", MC_KINGHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "JhTh9h", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "JhTh9h", "As6s", MC_ACEHIGH, DR_NONE },
            { "JhTh9h", "Ac7c", MC_ACEHIGH, DR_GUT },
            { "JhTh9h", "AcKc", MC_ACEHIGH, DR_GUT },
            { "JhTh9h", "AcKd", MC_ACEHIGH, DR_GUT },
            { "JhTh9h", "AdKc", MC_ACEHIGH, DR_GUT },
            { "JhTh9h", "AsKs", MC_ACEHIGH, DR_GUT },
            { "JhTh9h", "Ac8c", MC_ACEHIGH, DR_OESD },
            { "JhTh9h", "AcQc", MC_ACEHIGH, DR_OESD },
            { "JhTh9h", "AcQd", MC_ACEHIGH, DR_OESD },
            { "JhTh9h", "AdQc", MC_ACEHIGH, DR_OESD },
            { "JhTh9h", "AsQs", MC_ACEHIGH, DR_OESD },
            { "JhTh9h", "Ac2h", MC_ACEHIGH, DR_FLUSH },
            { "JhTh9h", "Ah2c", MC_ACEHIGH, DR_FLUSH },
            { "JhTh9h", "Ah3s", MC_ACEHIGH, DR_FLUSH },
            { "JhTh9h", "As6h", MC_ACEHIGH, DR_FLUSH },
            { "JhTh9h", "Ac7h", MC_ACEHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Ah8d", MC_ACEHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "Ah8s", MC_ACEHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "AsQh", MC_ACEHIGH, DR_FLUSH_OESD },
            { "JhTh9h", "2d2c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "3d3c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "4d4c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "4s4c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "5d5c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "6d6c", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "6s6d", MC_LOWPAIR, DR_NONE },
            { "JhTh9h", "7d7c", MC_LOWPAIR, DR_GUT },
            { "JhTh9h", "7s7c", MC_LOWPAIR, DR_GUT },
            { "JhTh9h", "7s7d", MC_LOWPAIR, DR_GUT },
            { "JhTh9h", "8d8c", MC_LOWPAIR, DR_OESD },
            { "JhTh9h", "8s8c", MC_LOWPAIR, DR_OESD },
            { "JhTh9h", "8s8d", MC_LOWPAIR, DR_OESD },
            { "JhTh9h", "2h2c", MC_LOWPAIR, DR_FLUSH },
            { "JhTh9h", "4h4d", MC_LOWPAIR, DR_FLUSH },
            { "JhTh9h", "6s6h", MC_LOWPAIR, DR_FLUSH },
            { "JhTh9h", "7h7c", MC_LOWPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "8h8c", MC_LOWPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "8s8h", MC_LOWPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "9c2c", MC_THIRDPAIR, DR_NONE },
            { "JhTh9h", "9d6c", MC_THIRDPAIR, DR_NONE },
            { "JhTh9h", "As9s", MC_THIRDPAIR, DR_NONE },
            { "JhTh9h", "9c7c", MC_THIRDPAIR, DR_GUT },
            { "JhTh9h", "9c7d", MC_THIRDPAIR, DR_GUT },
            { "JhTh9h", "Kc9c", MC_THIRDPAIR, DR_GUT },
            { "JhTh9h", "Ks9s", MC_THIRDPAIR, DR_GUT },
            { "JhTh9h", "9c8c", MC_THIRDPAIR, DR_OESD },
            { "JhTh9h", "9c8d", MC_THIRDPAIR, DR_OESD },
            { "JhTh9h", "Qc9c", MC_THIRDPAIR, DR_OESD },
            { "JhTh9h", "Qs9s", MC_THIRDPAIR, DR_OESD },
            { "JhTh9h", "9c2h", MC_THIRDPAIR, DR_FLUSH },
            { "JhTh9h", "9d6h", MC_THIRDPAIR, DR_FLUSH },
            { "JhTh9h", "Ah9s", MC_THIRDPAIR, DR_FLUSH },
            { "JhTh9h", "9c7h", MC_THIRDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "Kh9c", MC_THIRDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "Qh9s", MC_THIRDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "AcTc", MC_SECONDPAIR, DR_NONE },
            { "JhTh9h", "Tc2c", MC_SECONDPAIR, DR_NONE },
            { "JhTh9h", "Td3c", MC_SECONDPAIR, DR_NONE },
            { "JhTh9h", "Ts6s", MC_SECONDPAIR, DR_NONE },
            { "JhTh9h", "KcTc", MC_SECONDPAIR, DR_GUT },
            { "JhTh9h", "Tc7c", MC_SECONDPAIR, DR_GUT },
            { "JhTh9h", "Ts7s", MC_SECONDPAIR, DR_GUT },
            { "JhTh9h", "QcTc", MC_SECONDPAIR, DR_OESD },
            { "JhTh9h", "QcTd", MC_SECONDPAIR, DR_OESD },
            { "JhTh9h", "Tc8c", MC_SECONDPAIR, DR_OESD },
            { "JhTh9h", "Tc8d", MC_SECONDPAIR, DR_OESD },
            { "JhTh9h", "Ts8s", MC_SECONDPAIR, DR_OESD },
            { "JhTh9h", "AhTc", MC_SECONDPAIR, DR_FLUSH },
            { "JhTh9h", "Td3h", MC_SECONDPAIR, DR_FLUSH },
            { "JhTh9h", "Ts6h", MC_SECONDPAIR, DR_FLUSH },
            { "JhTh9h", "KhTc", MC_SECONDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "Tc7h", MC_SECONDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "Ts8h", MC_SECONDPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "AcJc", MC_TOPPAIR, DR_NONE },
            { "JhTh9h", "Jc2c", MC_TOPPAIR, DR_NONE },
            { "JhTh9h", "Jd3c", MC_TOPPAIR, DR_NONE },
            { "JhTh9h", "Js6s", MC_TOPPAIR, DR_NONE },
            { "JhTh9h", "Jc7c", MC_TOPPAIR, DR_GUT },
            { "JhTh9h", "KcJc", MC_TOPPAIR, DR_GUT },
            { "JhTh9h", "KcJd", MC_TOPPAIR, DR_GUT },
            { "JhTh9h", "KsJs", MC_TOPPAIR, DR_GUT },
            { "JhTh9h", "Jc8c", MC_TOPPAIR, DR_OESD },
            { "JhTh9h", "QcJc", MC_TOPPAIR, DR_OESD },
            { "JhTh9h", "QcJd", MC_TOPPAIR, DR_OESD },
            { "JhTh9h", "QsJs", MC_TOPPAIR, DR_OESD },
            { "JhTh9h", "AhJc", MC_TOPPAIR, DR_FLUSH },
            { "JhTh9h", "Jd3h", MC_TOPPAIR, DR_FLUSH },
            { "JhTh9h", "Js6h", MC_TOPPAIR, DR_FLUSH },
            { "JhTh9h", "Jc7h", MC_TOPPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "KhJc", MC_TOPPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "QhJs", MC_TOPPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "AdAc", MC_OVERPAIR, DR_NONE },
            { "JhTh9h", "AsAc", MC_OVERPAIR, DR_NONE },
            { "JhTh9h", "AsAd", MC_OVERPAIR, DR_NONE },
            { "JhTh9h", "KdKc", MC_OVERPAIR, DR_GUT },
            { "JhTh9h", "KsKc", MC_OVERPAIR, DR_GUT },
            { "JhTh9h", "KsKd", MC_OVERPAIR, DR_GUT },
            { "JhTh9h", "QdQc", MC_OVERPAIR, DR_OESD },
            { "JhTh9h", "QsQc", MC_OVERPAIR, DR_OESD },
            { "JhTh9h", "QsQd", MC_OVERPAIR, DR_OESD },
            { "JhTh9h", "AhAc", MC_OVERPAIR, DR_FLUSH },
            { "JhTh9h", "AhAd", MC_OVERPAIR, DR_FLUSH },
            { "JhTh9h", "AsAh", MC_OVERPAIR, DR_FLUSH },
            { "JhTh9h", "KhKc", MC_OVERPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "QhQc", MC_OVERPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "QsQh", MC_OVERPAIR, DR_FLUSH_OESD },
            { "JhTh9h", "Jc9c", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "Jc9d", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "JcTc", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "JcTd", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "Js9d", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "Tc9c", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "Tc9d", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "Ts9s", MC_TWOPAIR, DR_NONE },
            { "JhTh9h", "9d9c", MC_SET, DR_NONE },
            { "JhTh9h", "JdJc", MC_SET, DR_NONE },
            { "JhTh9h", "JsJc", MC_SET, DR_NONE },
            { "JhTh9h", "TdTc", MC_SET, DR_NONE },
            { "JhTh9h", "TsTd", MC_SET, DR_NONE },
            { "JhTh9h", "8c7c", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "8c7d", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "KcQc", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "KcQd", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "KdQd", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "Qs8s", MC_STRAIGHT, DR_NONE },
            { "JhTh9h", "8c7h", MC_STRAIGHT, DR_FLUSH },
            { "JhTh9h", "KhQd", MC_STRAIGHT, DR_FLUSH },
            { "JhTh9h", "Qs8h", MC_STRAIGHT, DR_FLUSH },
            { "JhTh9h", "3h2h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "Ah3h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "Ah4h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "Kh3h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "Qh2h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "Qh7h", MC_FLUSH, DR_NONE },
            { "JhTh9h", "8h7h", MC_STRFLUSH, DR_NONE },
            { "JhTh9h", "KhQh", MC_STRFLUSH, DR_NONE },
            { "JhTh9h", "Qh8h", MC_STRFLUSH, DR_NONE },
            // 7s5d2c
            { "7s5d2c", "8c3c", MC_NOTHING, DR_NONE },
            { "7s5d2c", "9d3s", MC_NOTHING, DR_NONE },
            { "7s5d2c", "9h3d", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Jc9c", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Jc9d", MC_NOTHING, DR_NONE },
            { "7s5d2c", "JcTc", MC_NOTHING, DR_NONE },
            { "7s5d2c", "JcTd", MC_NOTHING, DR_NONE },
            { "7s5d2c", "QcJc", MC_NOTHING, DR_NONE },
            { "7s5d2c", "QcJd", MC_NOTHING, DR_NONE },
            { "7s5d2c", "QcTc", MC_NOTHING, DR_NONE },
            { "7s5d2c", "QcTd", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Tc8c", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Tc8d", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Tc9c", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Tc9d", MC_NOTHING, DR_NONE },
            { "7s5d2c", "Ts9s", MC_NOTHING, DR_NONE },
            { "7s5d2c", "6c3c", MC_NOTHING, DR_GUT },
            { "7s5d2c", "9c6c", MC_NOTHING, DR_GUT },
            { "7s5d2c", "9c8c", MC_NOTHING, DR_GUT },
            { "7s5d2c", "9c8d", MC_NOTHING, DR_GUT },
            { "7s5d2c", "9s8s", MC_NOTHING, DR_GUT },
            { "7s5d2c", "4c3c", MC_NOTHING, DR_OESD },
            { "7s5d2c", "4c3d", MC_NOTHING, DR_OESD },
            { "7s5d2c", "6c4c", MC_NOTHING, DR_OESD },
            { "7s5d2c", "6c4d", MC_NOTHING, DR_OESD },
            { "7s5d2c", "6h4c", MC_NOTHING, DR_OESD },
            { "7s5d2c", "8c6c", MC_NOTHING, DR_OESD },
            { "7s5d2c", "8c6d", MC_NOTHING, DR_OESD },
            { "7s5d2c", "8s6s", MC_NOTHING, DR_OESD },
            { "7s5d2c", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "KcJc", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "KcQc", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Kd3d", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Kd9h", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Kh3c", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Ks3s", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Ks9d", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "7s5d2c", "Ac6c", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "AcKc", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "AcKd", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "AcQc", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "Ad9h", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "Ah6c", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "As9d", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "7s5d2c", "Ac3c", MC_ACEHIGH, DR_GUT },
            { "7s5d2c", "Ad4d", MC_ACEHIGH, DR_GUT },
            { "7s5d2c", "Ah3c", MC_ACEHIGH, DR_GUT },
            { "7s5d2c", "As4s", MC_ACEHIGH, DR_GUT },
            { "7s5d2c", "3c2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "3c2h", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "3d2h", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "3s2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "4c2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "4c2h", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "9d2s", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "9h2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Ad2h", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "As2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Kd2h", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Ks2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Qd2d", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Qs2s", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "Ts2s", MC_THIRDPAIR, DR_NONE },
            { "7s5d2c", "5c3c", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "5c3d", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "5c4c", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "5c4d", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "6c5c", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "6c5h", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "Ac5c", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "Ts5s", MC_SECONDPAIR, DR_NONE },
            { "7s5d2c", "6d6c", MC_UNDERPAIR, DR_NONE },
            { "7s5d2c", "6s6c", MC_UNDERPAIR, DR_NONE },
            { "7s5d2c", "6s6h", MC_UNDERPAIR, DR_NONE },
            { "7s5d2c", "7c3c", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "7c6c", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "7c6d", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "8c7c", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "8c7d", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "9c7c", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "9c7d", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "Ac7c", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "Ts7h", MC_TOPPAIR, DR_NONE },
            { "7s5d2c", "8d8c", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "9d9c", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "AdAc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "JdJc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "JsJc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "KdKc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "QdQc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "TdTc", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "TsTh", MC_OVERPAIR, DR_NONE },
            { "7s5d2c", "5c2d", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "5c2h", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "7c2d", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "7c2h", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "7c5c", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "7c5h", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "7h5s", MC_TWOPAIR, DR_NONE },
            { "7s5d2c", "2h2d", MC_SET, DR_NONE },
            { "7s5d2c", "2s2d", MC_SET, DR_NONE },
            { "7s5d2c", "5h5c", MC_SET, DR_NONE },
            { "7s5d2c", "5s5c", MC_SET, DR_NONE },
            { "7s5d2c", "7d7c", MC_SET, DR_NONE },
            { "7s5d2c", "7h7d", MC_SET, DR_NONE },
            // Th6h2h
            { "Th6h2h", "7c3c", MC_NOTHING, DR_NONE },
            { "Th6h2h", "7c5c", MC_NOTHING, DR_NONE },
            { "Th6h2h", "7c5d", MC_NOTHING, DR_NONE },
            { "Th6h2h", "Jc9c", MC_NOTHING, DR_NONE },
            { "Th6h2h", "Jc9d", MC_NOTHING, DR_NONE },
            { "Th6h2h", "Jd3c", MC_NOTHING, DR_NONE },
            { "Th6h2h", "QcJc", MC_NOTHING, DR_NONE },
            { "Th6h2h", "QcJd", MC_NOTHING, DR_NONE },
            { "Th6h2h", "QsJs", MC_NOTHING, DR_NONE },
            { "Th6h2h", "4c3c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "4c3d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "5c3c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "5c3d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "5c4c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "5c4d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "8c7c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "8c7d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "9c7c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "9c7d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "9c8c", MC_NOTHING, DR_GUT },
            { "Th6h2h", "9c8d", MC_NOTHING, DR_GUT },
            { "Th6h2h", "9s8s", MC_NOTHING, DR_GUT },
            { "Th6h2h", "7c3h", MC_NOTHING, DR_FLUSH },
            { "Th6h2h", "8s3h", MC_NOTHING, DR_FLUSH },
            { "Th6h2h", "8s4h", MC_NOTHING, DR_FLUSH },
            { "Th6h2h", "Jh3c", MC_NOTHING, DR_FLUSH },
            { "Th6h2h", "QsJh", MC_NOTHING, DR_FLUSH },
            { "Th6h2h", "4c3h", MC_NOTHING, DR_FLUSH_OESD },
            { "Th6h2h", "8c7h", MC_NOTHING, DR_FLUSH_OESD },
            { "Th6h2h", "9s8h", MC_NOTHING, DR_FLUSH_OESD },
            { "Th6h2h", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "Kd8c", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "KsQs", MC_KINGHIGH, DR_NONE },
            { "Th6h2h", "Kc3h", MC_KINGHIGH, DR_FLUSH },
            { "Th6h2h", "Kh5s", MC_KINGHIGH, DR_FLUSH },
            { "Th6h2h", "Kh8s", MC_KINGHIGH, DR_FLUSH },
            { "Th6h2h", "KsQh", MC_KINGHIGH, DR_FLUSH },
            { "Th6h2h", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "AcKc", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "AcKd", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "AcQc", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "AcQd", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "Ad8d", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "AsQs", MC_ACEHIGH, DR_NONE },
            { "Th6h2h", "Ac3h", MC_ACEHIGH, DR_FLUSH },
            { "Th6h2h", "Ah7c", MC_ACEHIGH, DR_FLUSH },
            { "Th6h2h", "Ah8s", MC_ACEHIGH, DR_FLUSH },
            { "Th6h2h", "AsQh", MC_ACEHIGH, DR_FLUSH },
            { "Th6h2h", "3c2c", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "3c2d", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "4c2c", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "4c2d", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "8d2d", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "Qs2s", MC_THIRDPAIR, DR_NONE },
            { "Th6h2h", "3h2c", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "4h2c", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "7h2d", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "Ah2c", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "Kh2c", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "Qh2s", MC_THIRDPAIR, DR_FLUSH },
            { "Th6h2h", "6c3c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "6c4c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "6c4d", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "6c5c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "6c5d", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "7c6c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "7c6d", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "8c6c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "8c6d", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "9c6c", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "Qs6s", MC_SECONDPAIR, DR_NONE },
            { "Th6h2h", "6c3h", MC_SECONDPAIR, DR_FLUSH },
            { "Th6h2h", "9h6c", MC_SECONDPAIR, DR_FLUSH },
            { "Th6h2h", "Qh6s", MC_SECONDPAIR, DR_FLUSH },
            { "Th6h2h", "7d7c", MC_UNDERPAIR, DR_NONE },
            { "Th6h2h", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "Th6h2h", "8s8c", MC_UNDERPAIR, DR_NONE },
            { "Th6h2h", "9d9c", MC_UNDERPAIR, DR_NONE },
            { "Th6h2h", "9s9d", MC_UNDERPAIR, DR_NONE },
            { "Th6h2h", "7h7c", MC_UNDERPAIR, DR_FLUSH },
            { "Th6h2h", "8h8d", MC_UNDERPAIR, DR_FLUSH },
            { "Th6h2h", "9s9h", MC_UNDERPAIR, DR_FLUSH },
            { "Th6h2h", "AcTc", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "JcTc", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "JcTd", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "QcTc", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "QcTd", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Tc7c", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Tc8c", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Tc8d", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Tc9c", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Tc9d", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "Ts9s", MC_TOPPAIR, DR_NONE },
            { "Th6h2h", "AhTc", MC_TOPPAIR, DR_FLUSH },
            { "Th6h2h", "Tc7h", MC_TOPPAIR, DR_FLUSH },
            { "Th6h2h", "Ts9h", MC_TOPPAIR, DR_FLUSH },
            { "Th6h2h", "AdAc", MC_OVERPAIR, DR_NONE },
            { "Th6h2h", "JdJc", MC_OVERPAIR, DR_NONE },
            { "Th6h2h", "KdKc", MC_OVERPAIR, DR_NONE },
            { "Th6h2h", "QdQc", MC_OVERPAIR, DR_NONE },
            { "Th6h2h", "QsQd", MC_OVERPAIR, DR_NONE },
            { "Th6h2h", "AhAc", MC_OVERPAIR, DR_FLUSH },
            { "Th6h2h", "KhKc", MC_OVERPAIR, DR_FLUSH },
            { "Th6h2h", "QsQh", MC_OVERPAIR, DR_FLUSH },
            { "Th6h2h", "6c2c", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "6c2d", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "Tc2c", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "Tc2d", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "Tc6c", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "Tc6d", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "Ts6s", MC_TWOPAIR, DR_NONE },
            { "Th6h2h", "2d2c", MC_SET, DR_NONE },
            { "Th6h2h", "6d6c", MC_SET, DR_NONE },
            { "Th6h2h", "6s6c", MC_SET, DR_NONE },
            { "Th6h2h", "TdTc", MC_SET, DR_NONE },
            { "Th6h2h", "TsTd", MC_SET, DR_NONE },
            { "Th6h2h", "4h3h", MC_FLUSH, DR_NONE },
            { "Th6h2h", "Ah5h", MC_FLUSH, DR_NONE },
            { "Th6h2h", "AhKh", MC_FLUSH, DR_NONE },
            { "Th6h2h", "Kh4h", MC_FLUSH, DR_NONE },
            { "Th6h2h", "Qh3h", MC_FLUSH, DR_NONE },
            { "Th6h2h", "QhJh", MC_FLUSH, DR_NONE },
            // AsAd7c
            { "AsAd7c", "3c2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "3c2d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "3d2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "3s2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "4c2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "4c2d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "4c3c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "4c3d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "5c3c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "5c3d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "5c4c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "5c4d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "6c4c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "6c4d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "6c5c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "6c5d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "8c6c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "8c6d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c2d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c2s", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c3d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c3s", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c8c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "9c8d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Jc9c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Jc9d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "JcTc", MC_NOTHING, DR_NONE },
            { "AsAd7c", "JcTd", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Jd2d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Js2s", MC_NOTHING, DR_NONE },
            { "AsAd7c", "QcJc", MC_NOTHING, DR_NONE },
            { "AsAd7c", "QcJd", MC_NOTHING, DR_NONE },
            { "AsAd7c", "QcTc", MC_NOTHING, DR_NONE },
            { "AsAd7c", "QcTd", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qd2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qd3d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qd9c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qs2c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qs3s", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Qs9c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Tc8c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Tc8d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Tc9c", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Tc9d", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Ts9s", MC_NOTHING, DR_NONE },
            { "AsAd7c", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "KcJc", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "KcQc", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Kd2c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Kd4d", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Kd9c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Kh2c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Ks2c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Ks4s", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "Ks9c", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "AsAd7c", "2d2c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "2s2c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "4s4c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "6s6h", MC_THIRDPAIR, DR_NONE },
            { "AsAd7c", "7d2c", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "7d5c", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "7d5d", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "7d6c", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "7d6d", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "8c7d", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "8c7h", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "8h7d", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "9c7d", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "9c7h", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "Ts7s", MC_SECONDPAIR, DR_NONE },
            { "AsAd7c", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "9d9c", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "KdKc", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "AsAd7c", "Ac2c", MC_TRIPS, DR_NONE },
            { "AsAd7c", "AcKc", MC_TRIPS, DR_NONE },
            { "AsAd7c", "AcKd", MC_TRIPS, DR_NONE },
            { "AsAd7c", "AcQc", MC_TRIPS, DR_NONE },
            { "AsAd7c", "AcQd", MC_TRIPS, DR_NONE },
            { "AsAd7c", "Ah2c", MC_TRIPS, DR_NONE },
            { "AsAd7c", "AhTs", MC_TRIPS, DR_NONE },
            { "AsAd7c", "7h7d", MC_FULL, DR_NONE },
            { "AsAd7c", "Ac7d", MC_FULL, DR_NONE },
            { "AsAd7c", "Ac7h", MC_FULL, DR_NONE },
            { "AsAd7c", "Ah7s", MC_FULL, DR_NONE },
            { "AsAd7c", "AhAc", MC_QUADS, DR_NONE },
            // 9s9d4h
            { "9s9d4h", "3c2c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "3c2d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "3d2c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "3s2c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "5c3c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "5c3d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "6c5c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "6c5d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "7c5c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "7c5d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "7c6c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "7c6d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "8c6c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "8c6d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "8c7c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "8c7d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "JcTc", MC_NOTHING, DR_NONE },
            { "9s9d4h", "JcTd", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Jd8c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "QcJc", MC_NOTHING, DR_NONE },
            { "9s9d4h", "QcJd", MC_NOTHING, DR_NONE },
            { "9s9d4h", "QcTc", MC_NOTHING, DR_NONE },
            { "9s9d4h", "QcTd", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Qd2d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Qs2s", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Tc8c", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Tc8d", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Ts8s", MC_NOTHING, DR_NONE },
            { "9s9d4h", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Kd2c", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Kd3d", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Kh2c", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Ks2c", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Ks3s", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "KsTs", MC_KINGHIGH, DR_NONE },
            { "9s9d4h", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "AcKc", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "AcKd", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "AcQc", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "AcQd", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "As2c", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "AsTs", MC_ACEHIGH, DR_NONE },
            { "9s9d4h", "2d2c", MC_THIRDPAIR, DR_NONE },
            { "9s9d4h", "2s2c", MC_THIRDPAIR, DR_NONE },
            { "9s9d4h", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "9s9d4h", "3s3h", MC_THIRDPAIR, DR_NONE },
            { "9s9d4h", "4c2c", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "4c2d", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "4c3c", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "4c3d", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "5c4c", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "5c4d", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "6c4c", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "6c4d", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "8h4c", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "Ad4d", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "As4s", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "Ts4s", MC_SECONDPAIR, DR_NONE },
            { "9s9d4h", "5d5c", MC_UNDERPAIR, DR_NONE },
            { "9s9d4h", "6d6c", MC_UNDERPAIR, DR_NONE },
            { "9s9d4h", "7d7c", MC_UNDERPAIR, DR_NONE },
            { "9s9d4h", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "9s9d4h", "8s8h", MC_UNDERPAIR, DR_NONE },
            { "9s9d4h", "AdAc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "JdJc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "KdKc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "KsKc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "QdQc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "TdTc", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "TsTh", MC_OVERPAIR, DR_NONE },
            { "9s9d4h", "9c2c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c2d", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c2s", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c3d", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c3s", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c7c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c7d", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c8c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9c8d", MC_TRIPS, DR_NONE },
            { "9s9d4h", "9h8c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Ad9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "As9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Jc9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Jc9h", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Kd9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Ks9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Tc9c", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Tc9h", MC_TRIPS, DR_NONE },
            { "9s9d4h", "Ts9h", MC_TRIPS, DR_NONE },
            { "9s9d4h", "4d4c", MC_FULL, DR_NONE },
            { "9s9d4h", "9c4c", MC_FULL, DR_NONE },
            { "9s9d4h", "9c4d", MC_FULL, DR_NONE },
            { "9s9d4h", "9h4s", MC_FULL, DR_NONE },
            { "9s9d4h", "9h9c", MC_QUADS, DR_NONE },
            // 5h5s9c
            { "5h5s9c", "3c2c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "3c2d", MC_NOTHING, DR_NONE },
            { "5h5s9c", "3h2c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "3s2c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "4c2c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "4c2d", MC_NOTHING, DR_NONE },
            { "5h5s9c", "4c3c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "4c3d", MC_NOTHING, DR_NONE },
            { "5h5s9c", "6c4c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "6c4d", MC_NOTHING, DR_NONE },
            { "5h5s9c", "JcTc", MC_NOTHING, DR_NONE },
            { "5h5s9c", "JcTd", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Jh7c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "QcJc", MC_NOTHING, DR_NONE },
            { "5h5s9c", "QcJd", MC_NOTHING, DR_NONE },
            { "5h5s9c", "QcTc", MC_NOTHING, DR_NONE },
            { "5h5s9c", "QcTd", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Qh2h", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Qs2s", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Tc8c", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Tc8d", MC_NOTHING, DR_NONE },
            { "5h5s9c", "Ts8s", MC_NOTHING, DR_NONE },
            { "5h5s9c", "7c6c", MC_NOTHING, DR_GUT },
            { "5h5s9c", "7c6d", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8c6c", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8c6d", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8c7c", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8c7d", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8d6c", MC_NOTHING, DR_GUT },
            { "5h5s9c", "8s7s", MC_NOTHING, DR_GUT },
            { "5h5s9c", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "KcJc", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "KcQc", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "Kh2c", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "Kh3h", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "Ks2c", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "Ks3s", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "5h5s9c", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "AcKc", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "AcKd", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "AcQc", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "Ah4h", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "As2c", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "As4s", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "5h5s9c", "2d2c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "2h2c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "2s2c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "3s3c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "4s4h", MC_THIRDPAIR, DR_NONE },
            { "5h5s9c", "6d6c", MC_UNDERPAIR, DR_NONE },
            { "5h5s9c", "7d7c", MC_UNDERPAIR, DR_NONE },
            { "5h5s9c", "7s7c", MC_UNDERPAIR, DR_NONE },
            { "5h5s9c", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "5h5s9c", "8s8h", MC_UNDERPAIR, DR_NONE },
            { "5h5s9c", "9d2c", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d2h", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d2s", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d3h", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d3s", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d7c", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d7d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d8c", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9d8d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "9s7h", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Ah9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "As9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Jc9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Jc9h", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Kh9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Ks9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Tc9d", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Tc9h", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "Ts9s", MC_TOPPAIR, DR_NONE },
            { "5h5s9c", "AdAc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "JdJc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "KdKc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "KsKc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "QdQc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "TdTc", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "TsTh", MC_OVERPAIR, DR_NONE },
            { "5h5s9c", "5c2c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "5c3c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "5c3d", MC_TRIPS, DR_NONE },
            { "5h5s9c", "5c4c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "5c4d", MC_TRIPS, DR_NONE },
            { "5h5s9c", "6c5c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "6c5d", MC_TRIPS, DR_NONE },
            { "5h5s9c", "7c5c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "7c5d", MC_TRIPS, DR_NONE },
            { "5h5s9c", "8h5c", MC_TRIPS, DR_NONE },
            { "5h5s9c", "Ts5d", MC_TRIPS, DR_NONE },
            { "5h5s9c", "9d5c", MC_FULL, DR_NONE },
            { "5h5s9c", "9d5d", MC_FULL, DR_NONE },
            { "5h5s9c", "9h9d", MC_FULL, DR_NONE },
            { "5h5s9c", "9s9h", MC_FULL, DR_NONE },
            { "5h5s9c", "5d5c", MC_QUADS, DR_NONE },
            // 2c2d2h
            { "2c2d2h", "4c3c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "4c3d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "5c3c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "5c3d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "5c4c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "5c4d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "6c4c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "6c4d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "6c5c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "6c5d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "7c5c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "7c5d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "7c6c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "7c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "8c6c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "8c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "8c7c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "8c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9c7c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9c8c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9c8d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9h3c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9h3d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9h4c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "9h4d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Jc9c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Jc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "JcTc", MC_NOTHING, DR_NONE },
            { "2c2d2h", "JcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Qc3c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "QcJc", MC_NOTHING, DR_NONE },
            { "2c2d2h", "QcJd", MC_NOTHING, DR_NONE },
            { "2c2d2h", "QcTc", MC_NOTHING, DR_NONE },
            { "2c2d2h", "QcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Qd3d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Tc8c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Tc8d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Tc9c", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Tc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Ts9s", MC_NOTHING, DR_NONE },
            { "2c2d2h", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Kc4c", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Kc9h", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Kd4d", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Kd9h", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Kh3c", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "KsTs", MC_KINGHIGH, DR_NONE },
            { "2c2d2h", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "Ac5c", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "Ac9h", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "AcKc", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "AcKd", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "AcQc", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "AcQd", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "Ad5d", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "Ad9h", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "Ah3c", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "AsTs", MC_ACEHIGH, DR_NONE },
            { "2c2d2h", "3d3c", MC_FULL, DR_NONE },
            { "2c2d2h", "4d4c", MC_FULL, DR_NONE },
            { "2c2d2h", "5d5c", MC_FULL, DR_NONE },
            { "2c2d2h", "6d6c", MC_FULL, DR_NONE },
            { "2c2d2h", "7d7c", MC_FULL, DR_NONE },
            { "2c2d2h", "8d8c", MC_FULL, DR_NONE },
            { "2c2d2h", "9d9c", MC_FULL, DR_NONE },
            { "2c2d2h", "AdAc", MC_FULL, DR_NONE },
            { "2c2d2h", "JdJc", MC_FULL, DR_NONE },
            { "2c2d2h", "KdKc", MC_FULL, DR_NONE },
            { "2c2d2h", "QdQc", MC_FULL, DR_NONE },
            { "2c2d2h", "TdTc", MC_FULL, DR_NONE },
            { "2c2d2h", "TsTh", MC_FULL, DR_NONE },
            { "2c2d2h", "3c2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "3d2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "4c2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "4d2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "9c2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "Ac2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "Ad2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "Kc2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "Kd2s", MC_QUADS, DR_NONE },
            { "2c2d2h", "Ts2s", MC_QUADS, DR_NONE },
            // 7c7d7h
            { "7c7d7h", "3c2c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "3c2d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "3d2c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "4c2c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "4c2d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "4c3c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "4c3d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "5c3c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "5c3d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "5c4c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "5c4d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "6c4c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "6c4d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "6c5c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "6c5d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "8c6c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "8c6d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9c8c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9c8d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9h2c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9h2d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9h3c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "9h3d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Jc9c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Jc9d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "JcTc", MC_NOTHING, DR_NONE },
            { "7c7d7h", "JcTd", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Qc2c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "QcJc", MC_NOTHING, DR_NONE },
            { "7c7d7h", "QcJd", MC_NOTHING, DR_NONE },
            { "7c7d7h", "QcTc", MC_NOTHING, DR_NONE },
            { "7c7d7h", "QcTd", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Qd2d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Tc8c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Tc8d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Tc9c", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Tc9d", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Ts9s", MC_NOTHING, DR_NONE },
            { "7c7d7h", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kc2d", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kc9h", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kd2c", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kd3d", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kd9h", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Kh2c", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "KsTs", MC_KINGHIGH, DR_NONE },
            { "7c7d7h", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ac2d", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ac4c", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ac9h", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "AcKc", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "AcKd", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "AcQc", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "AcQd", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ad9h", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "AsTs", MC_ACEHIGH, DR_NONE },
            { "7c7d7h", "2d2c", MC_FULL, DR_NONE },
            { "7c7d7h", "3d3c", MC_FULL, DR_NONE },
            { "7c7d7h", "4d4c", MC_FULL, DR_NONE },
            { "7c7d7h", "5d5c", MC_FULL, DR_NONE },
            { "7c7d7h", "6d6c", MC_FULL, DR_NONE },
            { "7c7d7h", "8d8c", MC_FULL, DR_NONE },
            { "7c7d7h", "9d9c", MC_FULL, DR_NONE },
            { "7c7d7h", "AdAc", MC_FULL, DR_NONE },
            { "7c7d7h", "JdJc", MC_FULL, DR_NONE },
            { "7c7d7h", "KdKc", MC_FULL, DR_NONE },
            { "7c7d7h", "QdQc", MC_FULL, DR_NONE },
            { "7c7d7h", "TdTc", MC_FULL, DR_NONE },
            { "7c7d7h", "TsTh", MC_FULL, DR_NONE },
            { "7c7d7h", "7s2c", MC_QUADS, DR_NONE },
            { "7c7d7h", "7s5c", MC_QUADS, DR_NONE },
            { "7c7d7h", "7s5d", MC_QUADS, DR_NONE },
            { "7c7d7h", "7s6c", MC_QUADS, DR_NONE },
            { "7c7d7h", "7s6d", MC_QUADS, DR_NONE },
            { "7c7d7h", "8c7s", MC_QUADS, DR_NONE },
            { "7c7d7h", "9c7s", MC_QUADS, DR_NONE },
            { "7c7d7h", "Ts7s", MC_QUADS, DR_NONE },
            // Ah9h4hKs
            { "Ah9h4hKs", "6c2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "6c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "6c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "7c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "7c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "7c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "7c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "8c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "8c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "8c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "8c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Jd6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Js2s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Qs2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Qs3s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "Ts8s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs", "3c2c", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "3c2d", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "3s2c", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "5c3c", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "5c3d", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "JcTc", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "JcTd", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "QcJc", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "QcJd", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "QcTc", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "QcTd", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "QsTs", MC_NOTHING, DR_GUT },
            { "Ah9h4hKs", "6c2h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "8s2h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "8s3h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "Jh5c", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "Qh2c", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "Qh8s", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "Ts8h", MC_NOTHING, DR_FLUSH },
            { "Ah9h4hKs", "3c2h", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4hKs", "3h2c", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4hKs", "JcTh", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4hKs", "QsTh", MC_NOTHING, DR_FLUSH_OESD },
            { "Ah9h4hKs", "2d2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "2s2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "3d3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "4c2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "4c2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "4c3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "4c3d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "5c4c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "5c4d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "5d5c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "6c4c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "6c4d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "6d6c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "7c4d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "7d7c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "8d8c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "Ts4s", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs", "2h2c", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4hKs", "6h6d", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Th4s", MC_LOWPAIR, DR_FLUSH },
            { "Ah9h4hKs", "9c2c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c2s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c3s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c7c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c7d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c8c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c8d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9s6c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "Jc9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "Jc9d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "JdJc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "QdQc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "Qs9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "Tc9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "Tc9d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "TdTc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "TsTd", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs", "9c2h", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Jh9c", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "TsTh", MC_THIRDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kc2c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "KcJc", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "KcJd", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "KcQc", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "KcQd", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "Kd2c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "KdTs", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs", "Kc2h", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kh2c", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kh3d", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kh8s", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "KhTs", MC_SECONDPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Ac2c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "AcQc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "AcQd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "Ad7d", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "As2c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "AsTs", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs", "Ac2h", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Ad7h", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4hKs", "AsTh", MC_TOPPAIR, DR_FLUSH },
            { "Ah9h4hKs", "9c4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "9c4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Ac4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Ac4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Ac9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Ac9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "AcKc", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "AcKd", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Ad9s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "As4s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "As9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Kc4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Kc4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Kc9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Kc9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "Kd9s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs", "AcKh", MC_TWOPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kh4d", MC_TWOPAIR, DR_FLUSH },
            { "Ah9h4hKs", "Kh9s", MC_TWOPAIR, DR_FLUSH },
            { "Ah9h4hKs", "4d4c", MC_SET, DR_NONE },
            { "Ah9h4hKs", "9d9c", MC_SET, DR_NONE },
            { "Ah9h4hKs", "9s9d", MC_SET, DR_NONE },
            { "Ah9h4hKs", "AdAc", MC_SET, DR_NONE },
            { "Ah9h4hKs", "KdKc", MC_SET, DR_NONE },
            { "Ah9h4hKs", "KhKc", MC_SET, DR_FLUSH },
            { "Ah9h4hKs", "KhKd", MC_SET, DR_FLUSH },
            { "Ah9h4hKs", "3h2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs", "Jh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs", "Kh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs", "Kh5h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs", "Qh3h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs", "Th8h", MC_FLUSH, DR_NONE },
            // 9h7h2c2s
            { "9h7h2c2s", "4c3c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "4c3d", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "5c3c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "5c3d", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "5c4c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "5c4d", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "6c4c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "6c4d", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "Js5c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "Qc3c", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "QcJc", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "QcJd", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "QcTc", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "QcTd", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "Ts5s", MC_NOTHING, DR_NONE },
            { "9h7h2c2s", "6c5c", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "6c5d", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "JcTc", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "JcTd", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "JcTs", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "Ts6s", MC_NOTHING, DR_GUT },
            { "9h7h2c2s", "8c6c", MC_NOTHING, DR_OESD },
            { "9h7h2c2s", "8c6d", MC_NOTHING, DR_OESD },
            { "9h7h2c2s", "Tc8c", MC_NOTHING, DR_OESD },
            { "9h7h2c2s", "Tc8d", MC_NOTHING, DR_OESD },
            { "9h7h2c2s", "Ts8s", MC_NOTHING, DR_OESD },
            { "9h7h2c2s", "4h3h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c2s", "Jh6h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c2s", "Th5h", MC_NOTHING, DR_FLUSH },
            { "9h7h2c2s", "6h5h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c2s", "Jh8h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c2s", "Th8h", MC_NOTHING, DR_FLUSH_OESD },
            { "9h7h2c2s", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "Kc4c", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KcJc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KcJd", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KcQc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KcQd", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KdTc", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "KsTs", MC_KINGHIGH, DR_NONE },
            { "9h7h2c2s", "Kh3h", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c2s", "Kh8h", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c2s", "KhTh", MC_KINGHIGH, DR_FLUSH },
            { "9h7h2c2s", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "Ac5c", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AcKc", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AcKd", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AcQc", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AcQd", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AdQs", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "AsTs", MC_ACEHIGH, DR_NONE },
            { "9h7h2c2s", "Ah3h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c2s", "Ah4h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c2s", "Ah8h", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c2s", "AhTh", MC_ACEHIGH, DR_FLUSH },
            { "9h7h2c2s", "3d3c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c2s", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c2s", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c2s", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c2s", "6s6h", MC_THIRDPAIR, DR_NONE },
            { "9h7h2c2s", "7c3c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "7c5c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "7c5d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "7c6c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "7c6d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "8c7c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "8c7d", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "Ac7c", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "Ts7s", MC_SECONDPAIR, DR_NONE },
            { "9h7h2c2s", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c2s", "8s8c", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c2s", "8s8h", MC_UNDERPAIR, DR_NONE },
            { "9h7h2c2s", "9c3c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9c7c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9c7d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9c8c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9c8d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9d3h", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9s3c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9s4c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "9s7s", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Ac9s", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Ah9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Jc9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Jc9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Kc9s", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Kh9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Tc9c", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Tc9d", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "Ts9s", MC_TOPPAIR, DR_NONE },
            { "9h7h2c2s", "AdAc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "JdJc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "KdKc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "KsKc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "QdQc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "TdTc", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "TsTh", MC_OVERPAIR, DR_NONE },
            { "9h7h2c2s", "3c2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "3c2h", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "3h2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "4c2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "4c2h", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "Ac2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "Ah2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "Kc2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "Kh2d", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "Ts2h", MC_TRIPS, DR_NONE },
            { "9h7h2c2s", "3h2h", MC_TRIPS, DR_FLUSH },
            { "9h7h2c2s", "Ah2h", MC_TRIPS, DR_FLUSH },
            { "9h7h2c2s", "Qh2h", MC_TRIPS, DR_FLUSH },
            { "9h7h2c2s", "Th2h", MC_TRIPS, DR_FLUSH },
            { "9h7h2c2s", "7c2d", MC_FULL, DR_NONE },
            { "9h7h2c2s", "7c2h", MC_FULL, DR_NONE },
            { "9h7h2c2s", "7d7c", MC_FULL, DR_NONE },
            { "9h7h2c2s", "9c2d", MC_FULL, DR_NONE },
            { "9h7h2c2s", "9c2h", MC_FULL, DR_NONE },
            { "9h7h2c2s", "9d2h", MC_FULL, DR_NONE },
            { "9h7h2c2s", "9d9c", MC_FULL, DR_NONE },
            { "9h7h2c2s", "9s9d", MC_FULL, DR_NONE },
            { "9h7h2c2s", "2h2d", MC_QUADS, DR_NONE },
            // Ks8d3cKh
            { "Ks8d3cKh", "4c2c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "4c2d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "5c4c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "5c4d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "6c4c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "6c4d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "6c5c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "6c5d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "7c5c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "7c5d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "7c6c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "7c6d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "9c2d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "9c7c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "9c7d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "9d2s", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Jc9c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Jc9d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "JcTc", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "JcTd", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Jd9c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Js2s", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "QcJc", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "QcJd", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "QcTc", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "QcTd", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Qd2d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Qs2c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Qs9d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Tc9c", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Tc9d", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Ts9s", MC_NOTHING, DR_NONE },
            { "Ks8d3cKh", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "AcQc", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "AcQd", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "Ad9c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "As2c", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "As4s", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "As9d", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "AsTs", MC_ACEHIGH, DR_NONE },
            { "Ks8d3cKh", "2d2c", MC_LOWPAIR, DR_NONE },
            { "Ks8d3cKh", "2s2c", MC_LOWPAIR, DR_NONE },
            { "Ks8d3cKh", "2s2h", MC_LOWPAIR, DR_NONE },
            { "Ks8d3cKh", "3d2c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "3d2d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "3s2c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "4c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "4c3h", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "4d4c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "5c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "5c3h", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "7d7c", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "9c3d", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "9d3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "Qs3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "Ts3s", MC_THIRDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c2c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c3d", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c3h", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c6c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c6d", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c7c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8c7d", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "8s6s", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "9c8h", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "Tc8c", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "Tc8h", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "Ts8s", MC_SECONDPAIR, DR_NONE },
            { "Ks8d3cKh", "9d9c", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3cKh", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3cKh", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3cKh", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3cKh", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "Ks8d3cKh", "AdAc", MC_OVERPAIR, DR_NONE },
            { "Ks8d3cKh", "AsAc", MC_OVERPAIR, DR_NONE },
            { "Ks8d3cKh", "AsAh", MC_OVERPAIR, DR_NONE },
            { "Ks8d3cKh", "AcKc", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "AcKd", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "Kc2c", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KcJc", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KcJd", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KcQc", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KcQd", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KcTc", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "Kd2c", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "Kd9c", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "KdTs", MC_TRIPS, DR_NONE },
            { "Ks8d3cKh", "3h3d", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "8h8c", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kc3d", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kc3h", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kc8c", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kc8h", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kd3d", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "Kd8s", MC_FULL, DR_NONE },
            { "Ks8d3cKh", "KdKc", MC_QUADS, DR_NONE },
            // QsJhTd9c
            { "QsJhTd9c", "3c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "3c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "3h2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "3s2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "4c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "4c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "4c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "4c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "5c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "5c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "5c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "5c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "6c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "6c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "6c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "6c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "6d4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "7c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "7c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "7c6c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "7c6d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "7s6s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c", "Ac2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "Ah2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "Ah4h", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "As2c", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "As4s", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "As7s", MC_ACEHIGH, DR_GUT },
            { "QsJhTd9c", "2d2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "2h2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "2s2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "3d3c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "4d4c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "5d5c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "6d6c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "7d7c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "9d2c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "9d6h", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "9d7c", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "9d7d", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "9s7s", MC_LOWPAIR, DR_NONE },
            { "QsJhTd9c", "Ac9d", MC_LOWPAIR, DR_GUT },
            { "QsJhTd9c", "Ah9d", MC_LOWPAIR, DR_GUT },
            { "QsJhTd9c", "As9s", MC_LOWPAIR, DR_GUT },
            { "QsJhTd9c", "Tc2c", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd9c", "Th5c", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd9c", "Ts7s", MC_THIRDPAIR, DR_NONE },
            { "QsJhTd9c", "AcTc", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd9c", "AhTc", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd9c", "AsTs", MC_THIRDPAIR, DR_GUT },
            { "QsJhTd9c", "Jc2c", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd9c", "Jd5c", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd9c", "Js2s", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd9c", "Js7s", MC_SECONDPAIR, DR_NONE },
            { "QsJhTd9c", "AcJc", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd9c", "AhJc", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd9c", "AsJs", MC_SECONDPAIR, DR_GUT },
            { "QsJhTd9c", "Qc2c", MC_TOPPAIR, DR_NONE },
            { "QsJhTd9c", "Qd5c", MC_TOPPAIR, DR_NONE },
            { "QsJhTd9c", "Qh2h", MC_TOPPAIR, DR_NONE },
            { "QsJhTd9c", "Qh7s", MC_TOPPAIR, DR_NONE },
            { "QsJhTd9c", "AcQc", MC_TOPPAIR, DR_GUT },
            { "QsJhTd9c", "AcQd", MC_TOPPAIR, DR_GUT },
            { "QsJhTd9c", "AhQc", MC_TOPPAIR, DR_GUT },
            { "QsJhTd9c", "AsQh", MC_TOPPAIR, DR_GUT },
            { "QsJhTd9c", "AdAc", MC_OVERPAIR, DR_GUT },
            { "QsJhTd9c", "AsAc", MC_OVERPAIR, DR_GUT },
            { "QsJhTd9c", "AsAh", MC_OVERPAIR, DR_GUT },
            { "QsJhTd9c", "Jc9d", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Jc9h", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "JcTc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "JcTh", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Qc9d", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Qc9h", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "QcJc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "QcJd", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "QcTc", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "QcTh", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Qd9d", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Tc9d", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Tc9h", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "Ts9s", MC_TWOPAIR, DR_NONE },
            { "QsJhTd9c", "9h9d", MC_SET, DR_NONE },
            { "QsJhTd9c", "JdJc", MC_SET, DR_NONE },
            { "QsJhTd9c", "QdQc", MC_SET, DR_NONE },
            { "QsJhTd9c", "ThTc", MC_SET, DR_NONE },
            { "QsJhTd9c", "TsTh", MC_SET, DR_NONE },
            { "QsJhTd9c", "8c2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8c6c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8c6d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8c7c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8c7d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8d8c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8h2s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8h3s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8s2h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "8s3h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "9d8c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "9d8d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "AcKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "AcKd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Ah8s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "As8h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Kc2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Kc6c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "KcJc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "KcJd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "KcQc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "KcQd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "KdKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Kh2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Kh3h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Kh8s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Ks2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Ks3s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Ks8h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Tc8c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Tc8d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c", "Ts8s", MC_STRAIGHT, DR_NONE },
            // Ah9h4h2h
            { "Ah9h4h2h", "6c3c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "6c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "6c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "7c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "7c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "7c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "7c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "8c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "8c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "8c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "8c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "JcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "JcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "Js5d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "QcJc", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "QcJd", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "QcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "QcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "Ts8s", MC_NOTHING, DR_NONE },
            { "Ah9h4h2h", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "KcJc", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "KcJd", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "Kd8c", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "KsTs", MC_KINGHIGH, DR_NONE },
            { "Ah9h4h2h", "3c2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h2h", "3c2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h2h", "3d3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h2h", "8d2c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h2h", "Ts2s", MC_LOWPAIR, DR_NONE },
            { "Ah9h4h2h", "4c3c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "4c3d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "5c4c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "5c4d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "5d5c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "6c4c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "6c4d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "6d6c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "7d7c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "8c4d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "8d8c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "Ts4s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4h2h", "9c3c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "9c7c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "9c7d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "9c8d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "9s7d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "Jc9c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "Jc9d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "Tc9c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "Tc9d", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "Ts9s", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4h2h", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h2h", "KdKc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h2h", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h2h", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h2h", "TsTd", MC_UNDERPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac3c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "AcKc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "AcKd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "AcQc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "AcQd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "Ad8d", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "AsTs", MC_TOPPAIR, DR_NONE },
            { "Ah9h4h2h", "4c2c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "4c2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "9c2c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "9c2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "9c4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "9c4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac2c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "Ac9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "As9s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4h2h", "2d2c", MC_SET, DR_NONE },
            { "Ah9h4h2h", "4d4c", MC_SET, DR_NONE },
            { "Ah9h4h2h", "9d9c", MC_SET, DR_NONE },
            { "Ah9h4h2h", "AdAc", MC_SET, DR_NONE },
            { "Ah9h4h2h", "AsAd", MC_SET, DR_NONE },
            { "Ah9h4h2h", "5c3c", MC_STRAIGHT, DR_NONE },
            { "Ah9h4h2h", "5c3d", MC_STRAIGHT, DR_NONE },
            { "Ah9h4h2h", "5d3d", MC_STRAIGHT, DR_NONE },
            { "Ah9h4h2h", "5s3s", MC_STRAIGHT, DR_NONE },
            { "Ah9h4h2h", "3h2c", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "5h2c", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "8s3h", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "8s5h", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Jh3h", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Jh8s", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Kh2c", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Kh6h", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Kh8s", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Qh2c", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Qh5h", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "Qh8s", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "TsTh", MC_FLUSH, DR_NONE },
            { "Ah9h4h2h", "5h3h", MC_STRFLUSH, DR_NONE },
            // 9s8d7c6h
            { "9s8d7c6h", "3c2c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "3c2d", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "3d2c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "3s2c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "4c2c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "4c2d", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "4c3c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "4c3d", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "Qc2c", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "Qd2d", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "Qs2s", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "Qs4s", MC_NOTHING, DR_NONE },
            { "9s8d7c6h", "Jc2c", MC_NOTHING, DR_GUT },
            { "9s8d7c6h", "Jh4c", MC_NOTHING, DR_GUT },
            { "9s8d7c6h", "QcJc", MC_NOTHING, DR_GUT },
            { "9s8d7c6h", "QcJd", MC_NOTHING, DR_GUT },
            { "9s8d7c6h", "QsJs", MC_NOTHING, DR_GUT },
            { "9s8d7c6h", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "KcQc", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "KcQd", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "Kd2c", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "Kd3d", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "Kh2c", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "Ks2c", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "Ks3s", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "KsQs", MC_KINGHIGH, DR_NONE },
            { "9s8d7c6h", "KcJc", MC_KINGHIGH, DR_GUT },
            { "9s8d7c6h", "KcJd", MC_KINGHIGH, DR_GUT },
            { "9s8d7c6h", "KhJc", MC_KINGHIGH, DR_GUT },
            { "9s8d7c6h", "KsJs", MC_KINGHIGH, DR_GUT },
            { "9s8d7c6h", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AcKc", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AcKd", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AcQc", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AcQd", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "As2c", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "As4s", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AsQs", MC_ACEHIGH, DR_NONE },
            { "9s8d7c6h", "AcJc", MC_ACEHIGH, DR_GUT },
            { "9s8d7c6h", "AhJc", MC_ACEHIGH, DR_GUT },
            { "9s8d7c6h", "AsJs", MC_ACEHIGH, DR_GUT },
            { "9s8d7c6h", "2d2c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "2s2c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "3d3c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "4d4c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "6c2c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "6c4c", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "6c4d", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "6s2s", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "Qs6s", MC_LOWPAIR, DR_NONE },
            { "9s8d7c6h", "Jc6c", MC_LOWPAIR, DR_GUT },
            { "9s8d7c6h", "Jh6c", MC_LOWPAIR, DR_GUT },
            { "9s8d7c6h", "Js6s", MC_LOWPAIR, DR_GUT },
            { "9s8d7c6h", "7d2c", MC_THIRDPAIR, DR_NONE },
            { "9s8d7c6h", "Ac7d", MC_THIRDPAIR, DR_NONE },
            { "9s8d7c6h", "Qs7s", MC_THIRDPAIR, DR_NONE },
            { "9s8d7c6h", "Jc7d", MC_THIRDPAIR, DR_GUT },
            { "9s8d7c6h", "Jh7d", MC_THIRDPAIR, DR_GUT },
            { "9s8d7c6h", "Js7s", MC_THIRDPAIR, DR_GUT },
            { "9s8d7c6h", "8c2c", MC_SECONDPAIR, DR_NONE },
            { "9s8d7c6h", "Ac8c", MC_SECONDPAIR, DR_NONE },
            { "9s8d7c6h", "Qs8s", MC_SECONDPAIR, DR_NONE },
            { "9s8d7c6h", "Jc8c", MC_SECONDPAIR, DR_GUT },
            { "9s8d7c6h", "Jh8c", MC_SECONDPAIR, DR_GUT },
            { "9s8d7c6h", "Js8s", MC_SECONDPAIR, DR_GUT },
            { "9s8d7c6h", "9c2c", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "9h2d", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "9h2s", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "9h3d", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "9h3s", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Ac9c", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Ad9h", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "As9h", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Kd9h", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Ks9h", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Qs9h", MC_TOPPAIR, DR_NONE },
            { "9s8d7c6h", "Jc9c", MC_TOPPAIR, DR_GUT },
            { "9s8d7c6h", "Jc9d", MC_TOPPAIR, DR_GUT },
            { "9s8d7c6h", "Jh9c", MC_TOPPAIR, DR_GUT },
            { "9s8d7c6h", "Js9h", MC_TOPPAIR, DR_GUT },
            { "9s8d7c6h", "AdAc", MC_OVERPAIR, DR_NONE },
            { "9s8d7c6h", "KdKc", MC_OVERPAIR, DR_NONE },
            { "9s8d7c6h", "KsKc", MC_OVERPAIR, DR_NONE },
            { "9s8d7c6h", "QdQc", MC_OVERPAIR, DR_NONE },
            { "9s8d7c6h", "QsQh", MC_OVERPAIR, DR_NONE },
            { "9s8d7c6h", "JdJc", MC_OVERPAIR, DR_GUT },
            { "9s8d7c6h", "JsJc", MC_OVERPAIR, DR_GUT },
            { "9s8d7c6h", "JsJh", MC_OVERPAIR, DR_GUT },
            { "9s8d7c6h", "7d6c", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "7d6d", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "8c6c", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "8c6d", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "8c7d", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "8c7h", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c6c", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c6d", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c7d", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c7h", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c8c", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9c8h", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "9h8s", MC_TWOPAIR, DR_NONE },
            { "9s8d7c6h", "6d6c", MC_SET, DR_NONE },
            { "9s8d7c6h", "7h7d", MC_SET, DR_NONE },
            { "9s8d7c6h", "8h8c", MC_SET, DR_NONE },
            { "9s8d7c6h", "9d9c", MC_SET, DR_NONE },
            { "9s8d7c6h", "9h9d", MC_SET, DR_NONE },
            { "9s8d7c6h", "5c2c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "5c3c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "5c3d", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "5c4c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "5c4d", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "5d5c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "6c5c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "6c5d", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "7d5c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "7d5d", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "JcTc", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "JcTd", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "Kd5c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "QcTc", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "QcTd", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "Tc8c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "Tc8h", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "Tc9c", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "Tc9d", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "TdTc", MC_STRAIGHT, DR_NONE },
            { "9s8d7c6h", "TsTh", MC_STRAIGHT, DR_NONE },
            // Qd9d6d2c
            { "Qd9d6d2c", "7c3c", MC_NOTHING, DR_NONE },
            { "Qd9d6d2c", "Jh3h", MC_NOTHING, DR_NONE },
            { "Qd9d6d2c", "Ts5s", MC_NOTHING, DR_NONE },
            { "Qd9d6d2c", "4c3c", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "5c3c", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "5c4c", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "7c5c", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "7h5h", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "Ts7s", MC_NOTHING, DR_GUT },
            { "Qd9d6d2c", "8c7c", MC_NOTHING, DR_OESD },
            { "Qd9d6d2c", "JcTc", MC_NOTHING, DR_OESD },
            { "Qd9d6d2c", "JhTh", MC_NOTHING, DR_OESD },
            { "Qd9d6d2c", "Tc8c", MC_NOTHING, DR_OESD },
            { "Qd9d6d2c", "Ts8s", MC_NOTHING, DR_OESD },
            { "Qd9d6d2c", "7c3d", MC_NOTHING, DR_FLUSH },
            { "Qd9d6d2c", "Jd4s", MC_NOTHING, DR_FLUSH },
            { "Qd9d6d2c", "Ts5d", MC_NOTHING, DR_FLUSH },
            { "Qd9d6d2c", "4c3d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "5c3d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "5c4d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "7c5d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "8c7d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "8d7h", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "JcTd", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "Tc8d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "Ts8d", MC_NOTHING, DR_FLUSH_OESD },
            { "Qd9d6d2c", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "Qd9d6d2c", "Kc4c", MC_KINGHIGH, DR_NONE },
            { "Qd9d6d2c", "Kh5h", MC_KINGHIGH, DR_NONE },
            { "Qd9d6d2c", "Ks8s", MC_KINGHIGH, DR_NONE },
            { "Qd9d6d2c", "KcJc", MC_KINGHIGH, DR_GUT },
            { "Qd9d6d2c", "KhTc", MC_KINGHIGH, DR_GUT },
            { "Qd9d6d2c", "KsTs", MC_KINGHIGH, DR_GUT },
            { "Qd9d6d2c", "Kc3d", MC_KINGHIGH, DR_FLUSH },
            { "Qd9d6d2c", "Kd7h", MC_KINGHIGH, DR_FLUSH },
            { "Qd9d6d2c", "Ks8d", MC_KINGHIGH, DR_FLUSH },
            { "Qd9d6d2c", "KcJd", MC_KINGHIGH, DR_FLUSH_OESD },
            { "Qd9d6d2c", "KdTh", MC_KINGHIGH, DR_FLUSH_OESD },
            { "Qd9d6d2c", "KsTd", MC_KINGHIGH, DR_FLUSH_OESD },
            { "Qd9d6d2c", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "Qd9d6d2c", "Ac5c", MC_ACEHIGH, DR_NONE },
            { "Qd9d6d2c", "AcKc", MC_ACEHIGH, DR_NONE },
            { "Qd9d6d2c", "Ah8c", MC_ACEHIGH, DR_NONE },
            { "Qd9d6d2c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "Qd9d6d2c", "Ac3d", MC_ACEHIGH, DR_FLUSH },
            { "Qd9d6d2c", "AcKd", MC_ACEHIGH, DR_FLUSH },
            { "Qd9d6d2c", "AdJh", MC_ACEHIGH, DR_FLUSH },
            { "Qd9d6d2c", "AsTd", MC_ACEHIGH, DR_FLUSH },
            { "Qd9d6d2c", "3c2h", MC_LOWPAIR, DR_NONE },
            { "Qd9d6d2c", "4c2h", MC_LOWPAIR, DR_NONE },
            { "Qd9d6d2c", "7s2h", MC_LOWPAIR, DR_NONE },
            { "Qd9d6d2c", "Ts2s", MC_LOWPAIR, DR_NONE },
            { "Qd9d6d2c", "3c2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "3d2h", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "3d3c", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "4c2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "4d4c", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "5d5c", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "7h2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Ac2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Ad2h", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Kc2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Kd2h", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Ts2d", MC_LOWPAIR, DR_FLUSH },
            { "Qd9d6d2c", "6c3c", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "6c4c", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "6c5c", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "7c6c", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "7c6h", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "8c6c", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "8c6h", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "8h6h", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "Ts6s", MC_THIRDPAIR, DR_NONE },
            { "Qd9d6d2c", "6c3d", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "6c4d", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "6c5d", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "7d7c", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "8d6h", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "8d8c", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Td6s", MC_THIRDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9c3c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9c7c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9h3c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9h4c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9s7h", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Ac9h", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Jc9c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Jc9h", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Kc9h", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Tc9c", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Tc9h", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "Ts9s", MC_SECONDPAIR, DR_NONE },
            { "Qd9d6d2c", "9c3d", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9c7d", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9c8d", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9s7d", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Ad9c", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Kd9c", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Td9s", MC_SECONDPAIR, DR_FLUSH },
            { "Qd9d6d2c", "JhJc", MC_UNDERPAIR, DR_NONE },
            { "Qd9d6d2c", "ThTc", MC_UNDERPAIR, DR_NONE },
            { "Qd9d6d2c", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "Qd9d6d2c", "JdJc", MC_UNDERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "TdTc", MC_UNDERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "TsTd", MC_UNDERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "AcQc", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "AcQh", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "KcQc", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "KcQh", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc3c", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "QcJc", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "QcTc", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "Qh3h", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "QsTs", MC_TOPPAIR, DR_NONE },
            { "Qd9d6d2c", "AdQc", MC_TOPPAIR, DR_FLUSH },
            { "Qd9d6d2c", "QcJd", MC_TOPPAIR, DR_FLUSH },
            { "Qd9d6d2c", "QcTd", MC_TOPPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Qh3d", MC_TOPPAIR, DR_FLUSH },
            { "Qd9d6d2c", "QsTd", MC_TOPPAIR, DR_FLUSH },
            { "Qd9d6d2c", "AhAc", MC_OVERPAIR, DR_NONE },
            { "Qd9d6d2c", "KhKc", MC_OVERPAIR, DR_NONE },
            { "Qd9d6d2c", "KsKh", MC_OVERPAIR, DR_NONE },
            { "Qd9d6d2c", "AdAc", MC_OVERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "KdKc", MC_OVERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "KsKd", MC_OVERPAIR, DR_FLUSH },
            { "Qd9d6d2c", "6c2h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "9c2h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "9c6c", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "9c6h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc2h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc2s", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc6c", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc6h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc9c", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qc9h", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "Qs9s", MC_TWOPAIR, DR_NONE },
            { "Qd9d6d2c", "6c2d", MC_TWOPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9c2d", MC_TWOPAIR, DR_FLUSH },
            { "Qd9d6d2c", "9h2d", MC_TWOPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Qc2d", MC_TWOPAIR, DR_FLUSH },
            { "Qd9d6d2c", "Qs2d", MC_TWOPAIR, DR_FLUSH },
            { "Qd9d6d2c", "2s2h", MC_SET, DR_NONE },
            { "Qd9d6d2c", "6h6c", MC_SET, DR_NONE },
            { "Qd9d6d2c", "9h9c", MC_SET, DR_NONE },
            { "Qd9d6d2c", "9s9c", MC_SET, DR_NONE },
            { "Qd9d6d2c", "QhQc", MC_SET, DR_NONE },
            { "Qd9d6d2c", "QsQh", MC_SET, DR_NONE },
            { "Qd9d6d2c", "2h2d", MC_SET, DR_FLUSH },
            { "Qd9d6d2c", "2s2d", MC_SET, DR_FLUSH },
            { "Qd9d6d2c", "3d2d", MC_FLUSH, DR_NONE },
            { "Qd9d6d2c", "Ad4d", MC_FLUSH, DR_NONE },
            { "Qd9d6d2c", "AdKd", MC_FLUSH, DR_NONE },
            { "Qd9d6d2c", "Jd2d", MC_FLUSH, DR_NONE },
            { "Qd9d6d2c", "Kd3d", MC_FLUSH, DR_NONE },
            { "Qd9d6d2c", "Td8d", MC_FLUSH, DR_NONE },
            // 9dQd2d6d
            { "9dQd2d6d", "4c3c", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "5c3c", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "5c4c", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "7c5c", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "Jc3c", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "Ts7s", MC_NOTHING, DR_NONE },
            { "9dQd2d6d", "8c7c", MC_NOTHING, DR_GUT },
            { "9dQd2d6d", "JcTc", MC_NOTHING, DR_GUT },
            { "9dQd2d6d", "JhTh", MC_NOTHING, DR_GUT },
            { "9dQd2d6d", "Tc8c", MC_NOTHING, DR_GUT },
            { "9dQd2d6d", "Ts8s", MC_NOTHING, DR_GUT },
            { "9dQd2d6d", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "9dQd2d6d", "KcJc", MC_KINGHIGH, DR_NONE },
            { "9dQd2d6d", "Kh7h", MC_KINGHIGH, DR_NONE },
            { "9dQd2d6d", "KsTs", MC_KINGHIGH, DR_NONE },
            { "9dQd2d6d", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "9dQd2d6d", "AcKc", MC_ACEHIGH, DR_NONE },
            { "9dQd2d6d", "Ah8c", MC_ACEHIGH, DR_NONE },
            { "9dQd2d6d", "AsTs", MC_ACEHIGH, DR_NONE },
            { "9dQd2d6d", "3c2c", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "3c2h", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "4c2c", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "4c2h", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "8c2c", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "Ts2s", MC_LOWPAIR, DR_NONE },
            { "9dQd2d6d", "6c3c", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "6c4c", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "6c5c", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "7c6c", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "7c6h", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "8c6c", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "8c6h", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "8h6h", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "Ts6s", MC_THIRDPAIR, DR_NONE },
            { "9dQd2d6d", "9c3c", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "9c7c", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "9c8c", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "9s7h", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "Jc9c", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "Jc9h", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "Tc9c", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "Tc9h", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "Ts9s", MC_SECONDPAIR, DR_NONE },
            { "9dQd2d6d", "JhJc", MC_UNDERPAIR, DR_NONE },
            { "9dQd2d6d", "ThTc", MC_UNDERPAIR, DR_NONE },
            { "9dQd2d6d", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "9dQd2d6d", "AcQc", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "AcQh", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "KcQc", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "KcQh", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "QcJc", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "QcTc", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "Qh3h", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "QsTs", MC_TOPPAIR, DR_NONE },
            { "9dQd2d6d", "AhAc", MC_OVERPAIR, DR_NONE },
            { "9dQd2d6d", "KhKc", MC_OVERPAIR, DR_NONE },
            { "9dQd2d6d", "KsKh", MC_OVERPAIR, DR_NONE },
            { "9dQd2d6d", "6c2c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "6c2h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "9c2c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "9c2h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "9c6c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "9c6h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc2c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc2h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc6c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc6h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc9c", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qc9h", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "Qs9s", MC_TWOPAIR, DR_NONE },
            { "9dQd2d6d", "2h2c", MC_SET, DR_NONE },
            { "9dQd2d6d", "6h6c", MC_SET, DR_NONE },
            { "9dQd2d6d", "9h9c", MC_SET, DR_NONE },
            { "9dQd2d6d", "QhQc", MC_SET, DR_NONE },
            { "9dQd2d6d", "QsQh", MC_SET, DR_NONE },
            { "9dQd2d6d", "3d2c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "3d3c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "4c3d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "4d2c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "4d4c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "5c3d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "5c4d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "5d5c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "6c4d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "6c5d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "7c5d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "7d7c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "8c7d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "8d8c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "8s3d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "8s4d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "9c7d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "9c8d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "AcKd", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Ad2c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Ad5d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Ad8s", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "AdAc", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "As4d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "JcTd", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Jd3d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "JdJc", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "KcJd", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Kd2c", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Kd4d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Kd8s", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "KdKc", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "QcJd", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "QcTd", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "Tc8d", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "TdTc", MC_FLUSH, DR_NONE },
            { "9dQd2d6d", "TsTd", MC_FLUSH, DR_NONE },
            // Ah9h4hKs2c
            { "Ah9h4hKs2c", "6c3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "6c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "6c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "7c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "7c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "7c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "7c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "8c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "8c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "8c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "8c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "JcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "JcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "Js3d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "QcJc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "QcJd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "QcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "QcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "Qs3s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "Ts8s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKs2c", "3c2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "3c2h", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "3d3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "3h2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "3s2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "4c3c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "4c3d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "5c4c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "5c4d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "5d5c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "6c4c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "6c4d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "6d6c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "7d7c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "7s2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "8d8c", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Js2s", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Qh2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Qs2d", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ts4s", MC_LOWPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c3c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c3h", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c7c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c7d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c8c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c8d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9d3s", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9s8d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Jc9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Jc9d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "JdJc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "QdQc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Qh9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Qs9d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Tc9c", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Tc9d", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "TdTc", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "TsTh", MC_THIRDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc3c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "KcJc", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "KcJd", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "KcQc", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "KcQd", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kd7c", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "KhTs", MC_SECONDPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac3c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs2c", "AcQc", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs2c", "AcQd", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ad8c", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs2c", "AsTs", MC_TOPPAIR, DR_NONE },
            { "Ah9h4hKs2c", "4c2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "4c2h", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c2h", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9c4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "9d2s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac2h", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ac9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "AcKc", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "AcKd", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Ad4s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "As2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "As4s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "As9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc2h", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc4c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc4d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kc9d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kh2d", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kh9c", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "Kh9s", MC_TWOPAIR, DR_NONE },
            { "Ah9h4hKs2c", "2h2d", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "2s2d", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "4d4c", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "9d9c", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "9s9c", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "AdAc", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "KdKc", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "KhKd", MC_SET, DR_NONE },
            { "Ah9h4hKs2c", "5c3c", MC_STRAIGHT, DR_NONE },
            { "Ah9h4hKs2c", "5c3d", MC_STRAIGHT, DR_NONE },
            { "Ah9h4hKs2c", "5d3s", MC_STRAIGHT, DR_NONE },
            { "Ah9h4hKs2c", "5s3s", MC_STRAIGHT, DR_NONE },
            { "Ah9h4hKs2c", "3h2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs2c", "Jh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs2c", "Kh2h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs2c", "Kh5h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs2c", "Qh3h", MC_FLUSH, DR_NONE },
            { "Ah9h4hKs2c", "Th8h", MC_FLUSH, DR_NONE },
            // KdKc7h7d2s
            { "KdKc7h7d2s", "3c2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "3c2d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "3d2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "3d3c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "4c2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "4c2d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "4c3c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "4c3d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "4d4c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "5c3c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "5c3d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "5c4c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "5c4d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "5d5c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "6c4c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "6c4d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "6c5c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "6c5d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "6d6c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "8c6c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "8c6d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9c8c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9c8d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9h2d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9h3d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9s2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "9s3c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Jc2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Jc6s", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Jc9c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Jc9d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "JcTc", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "JcTd", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Jd2d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qc2d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qc3c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qc9s", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "QcJc", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "QcJd", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "QcTc", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "QcTd", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qd2c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qd3d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Qd9h", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Tc8c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Tc8d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Tc9c", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Tc9d", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Ts9s", MC_NOTHING, DR_NONE },
            { "KdKc7h7d2s", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ac2d", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ac4c", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ac9s", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "AcQc", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "AcQd", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ad4d", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ad9h", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "Ah2c", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "AsTs", MC_ACEHIGH, DR_NONE },
            { "KdKc7h7d2s", "8d8c", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "9d9c", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "JdJc", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "JsJc", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "QdQc", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "TdTc", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "TsTh", MC_UNDERPAIR, DR_NONE },
            { "KdKc7h7d2s", "AdAc", MC_OVERPAIR, DR_NONE },
            { "KdKc7h7d2s", "AsAc", MC_OVERPAIR, DR_NONE },
            { "KdKc7h7d2s", "AsAh", MC_OVERPAIR, DR_NONE },
            { "KdKc7h7d2s", "2d2c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c2c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c2d", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c5c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c5d", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c6c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7c6d", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "8c7c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "8c7s", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "9c7c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "9c7s", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "AcKh", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "AcKs", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh2c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh2d", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh3c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh4d", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh7c", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Kh7s", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "KhJc", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "KhJd", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "KhQc", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "KhQd", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "Ts7s", MC_FULL, DR_NONE },
            { "KdKc7h7d2s", "7s7c", MC_QUADS, DR_NONE },
            { "KdKc7h7d2s", "KsKh", MC_QUADS, DR_NONE },
            // QsJhTd9c8h
            { "QsJhTd9c8h", "2d2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "2h2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "2s2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "3c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "3c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "3d3c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "3h2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "3s2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "4c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "4c2d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "4c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "4c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "4d4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "5c3c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "5c3d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "5c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "5c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "5d5c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "6c4c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "6c4d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "6c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "6c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "6d6c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "7c5c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "7c5d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "7c6c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "7c6d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "7d7c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8c2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8c6c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8c6d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8c7c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8c7d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8d8c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8s2h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "8s3h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d2s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d3s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d7c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d7d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d8c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9d8d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "9h9d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Ac2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Ac6c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "AcQc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "AcQd", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "AdAc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Ah2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Ah4h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Ah8s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "As2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "As4s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "As9d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Jc2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Jc8c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Jc8d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Jc9d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Jc9h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "JcTc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "JcTh", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "JdJc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Js2s", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qc2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qc8c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qc8d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qc9d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qc9h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "QcJc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "QcJd", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "QcTc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "QcTh", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "QdQc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Qh2h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Tc2c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Tc8c", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Tc8d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Tc9d", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "Tc9h", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "ThTc", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "TsTh", MC_NOTHING, DR_NONE },
            { "QsJhTd9c8h", "AcKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "AcKd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Kc2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KcJc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KcJd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KcQc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KcQd", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KdKc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KdQc", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Kh2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Kh3h", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Kh8s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Ks2c", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Ks3s", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "Ks9d", MC_STRAIGHT, DR_NONE },
            { "QsJhTd9c8h", "KsTs", MC_STRAIGHT, DR_NONE },
            // Ah9h4hKh2h
            { "Ah9h4hKh2h", "2d2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "3c2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "3c2d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "3d3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "3h2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "4c2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "4c2d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "4c3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "4c3d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "4d4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5c3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5c3d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5c4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5c4d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5d5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5h2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "6c4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "6c4d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "6c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "6c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "6d6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "7c5c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "7c5d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "7c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "7c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "7d7c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8c6c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8c6d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8d8c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8s3h", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "8s5h", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c2d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c4d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c7c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c7d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c8c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9c8d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "9d9c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac2d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac4d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac9c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Ac9d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "AcKc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "AcKd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "AcQc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "AcQd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "AdAc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jc2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jc9c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jc9d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "JcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "JcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "JdJc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jh2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jh5h", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Jh8s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc2d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc3c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc4c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc4d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc9c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Kc9d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "KcJc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "KcJd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "KcQc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "KcQd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "KdKc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "QcJc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "QcJd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "QcTc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "QcTd", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "QdQc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Qh2c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Qh6h", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Qh8s", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Tc9c", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Tc9d", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "TdTc", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "Th3h", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "TsTh", MC_NOTHING, DR_NONE },
            { "Ah9h4hKh2h", "5h3h", MC_STRFLUSH, DR_NONE },
            // TdTh2c2dTs
            { "TdTh2c2dTs", "3c2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "3c2s", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "3d2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "3h2s", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "4c2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "4c2s", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "4c3c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "4c3d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "4d2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "5c3c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "5c3d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "5c4c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "5c4d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "6c4c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "6c4d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "6c5c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "6c5d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "7c5c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "7c5d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "7c6c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "7c6d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "8c6c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "8c6d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "8c7c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "8c7d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c3d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c4d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c7c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c7d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c8c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9c8d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9d2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "9d3h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ac3c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "AcKc", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "AcKd", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "AcQc", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "AcQd", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ad2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ad5d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ad9c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ah2s", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ah4h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Ah9d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Jc9c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Jc9d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kc3c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "KcJc", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "KcJd", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "KcQc", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "KcQd", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kd2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kd4d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kd9c", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kh2s", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kh3h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Kh9d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "QcJc", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "QcJd", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Qd3d", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "Qh2h", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "QsJs", MC_NOTHING, DR_NONE },
            { "TdTh2c2dTs", "3d3c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "4d4c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "5d5c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "6d6c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "7d7c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "8d8c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "8s8c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "9d9c", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "AdAc", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "JdJc", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "KdKc", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "QdQc", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "QsQh", MC_FULL, DR_NONE },
            { "TdTh2c2dTs", "2s2h", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "JcTc", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "QcTc", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc2h", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc2s", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc4c", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc8c", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc8d", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc9c", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc9d", MC_QUADS, DR_NONE },
            { "TdTh2c2dTs", "Tc9s", MC_QUADS, DR_NONE },
            // 6s6d6h6c2d
            { "6s6d6h6c2d", "2h2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "2s2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "3c2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "3c2h", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "3d2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "3d3c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "3s2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4c2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4c2h", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4c3c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4c3d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4d2c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "4d4c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "5c3c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "5c3d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "5c4c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "5c4d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "5d5c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "7c5c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "7c5d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "7d7c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "8c7c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "8c7d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "8d8c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9c7c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9c7d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9c8c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9c8d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9d9c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9s3d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "9s4d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Jc8c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Jc9c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Jc9d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "JcTc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "JcTd", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "JdJc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "QcJc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "QcJd", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "QcTc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "QcTd", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Qd3d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "QdQc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Qs2s", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc2s", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc3s", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc8c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc8d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc9c", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Tc9d", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "TdTc", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "TsTh", MC_NOTHING, DR_NONE },
            { "6s6d6h6c2d", "Kc2c", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Kc3c", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KcJc", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KcJd", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KcQc", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KcQd", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Kd2c", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Kd4d", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Kd9s", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KdKc", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Kh2s", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ks2c", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ks3s", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KsTc", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "KsTs", MC_KINGHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ac2c", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ac3c", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AcKc", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AcKd", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AcQc", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AcQd", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ad2c", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ad5d", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ad9s", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AdAc", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "Ah2s", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "As2c", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "As4s", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AsTc", MC_ACEHIGH, DR_NONE },
            { "6s6d6h6c2d", "AsTs", MC_ACEHIGH, DR_NONE },
            // AhAdAc2h2d
            { "AhAdAc2h2d", "3c2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "3c2s", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "3d2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "3h2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4c2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4c2s", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4c3c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4c3d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4d2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "4h2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "5c3c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "5c3d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "5c4c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "5c4d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "6c4c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "6c4d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "6c5c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "6c5d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "7c5c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "7c5d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "7c6c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "7c6d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "8c6c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "8c6d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "8c7c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "8c7d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c3d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c3h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c4d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c4h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c7c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c7d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c8c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "9c8d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Jc9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Jc9d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "JcTc", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "JcTd", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Jd3d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Jh3h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Jh8h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kc3c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "KcJc", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "KcJd", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "KcQc", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "KcQd", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kd2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kd5d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kd9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kh2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kh5h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Kh9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "QcJc", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "QcJd", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "QcTc", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "QcTd", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qd2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qd4d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qd9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qh2c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qh4h", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Qh9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Tc8c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Tc8d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Tc9c", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Tc9d", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "Ts9s", MC_NOTHING, DR_NONE },
            { "AhAdAc2h2d", "3d3c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "4d4c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "5d5c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "6d6c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "7d7c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "8d8c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "8s8c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "9d9c", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "JdJc", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "KdKc", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "QdQc", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "TdTc", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "TsTh", MC_FULL, DR_NONE },
            { "AhAdAc2h2d", "2s2c", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "As2c", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "As2s", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "As3c", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "As8c", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "AsKc", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "AsKd", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "AsQc", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "AsQd", MC_QUADS, DR_NONE },
            { "AhAdAc2h2d", "AsTs", MC_QUADS, DR_NONE },
            // Ac2c3c4c5c
            { "Ac2c3c4c5c", "2h2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "3d2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "3d2h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "3h3d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "4d2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "4d2h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "4d3d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "4d3h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "4h4d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d2h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d3d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d3h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d4d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5d4h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "5h5d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "7c2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "7c5d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "7c5h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "7c6d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "7d7c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "8c6d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "8c7c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "8c7d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "8d8c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "8s7c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "9c7c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "9c7d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "9c8c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "9c8d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "9d9c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad2h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad3d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad3h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad4d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad4h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad5d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Ad5h", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "AdKc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "AdKd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "AdQc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "AdQd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "AhAd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Jc9c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Jc9d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "JcTc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "JcTd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Jd3d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "JdJc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Kc2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Kc8c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Kc8s", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "KcJc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "KcJd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "KcQc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "KcQd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "KdKc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Qc2d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Qc7c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Qc8s", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "QcJc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "QcJd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "QcTc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "QcTd", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "QdQc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Tc8c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Tc8d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Tc9c", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "Tc9d", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "TdTc", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "TsTh", MC_NOTHING, DR_NONE },
            { "Ac2c3c4c5c", "6c2d", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "6c4d", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "6c4h", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "6c5d", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "6c5h", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "6d6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "7c6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "8c6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "8s6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "9c6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "Ad6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "Jc6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "Kc6c", MC_STRFLUSH, DR_NONE },
            { "Ac2c3c4c5c", "Ts6c", MC_STRFLUSH, DR_NONE },
            // 2c2d2h3c3d
            { "2c2d2h3c3d", "5c4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "5c4d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "6c4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "6c4d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "6c5c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "6c5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "7c5c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "7c5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "7c6c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "7c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "8c6c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "8c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "8c7c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "8c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9c7c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9c8c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9c8d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9s4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9s4d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9s5c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "9s5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ac4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ac6c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ac9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "AcKc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "AcKd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "AcQc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "AcQd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ad6d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ad9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Jc9c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Jc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "JcTc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "JcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Jd7c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Kc4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Kc5c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Kc9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "KcJc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "KcJd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "KcQc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "KcQd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Kd5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Kd9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Qc4c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "QcJc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "QcJd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "QcTc", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "QcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Qd4d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Tc8c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Tc8d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Tc9c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Tc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "Ts9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c3d", "4d4c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "5d5c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "6d6c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "7d7c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "8d8c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "9d9c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "9s9c", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "AdAc", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "JdJc", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "KdKc", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "QdQc", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "TdTc", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "TsTh", MC_FULL, DR_NONE },
            { "2c2d2h3c3d", "4c3h", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "4c3s", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "5c3h", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "5c3s", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "9h3h", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "Ts3s", MC_TOPFULL, DR_NONE },
            { "2c2d2h3c3d", "3h2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "3s2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "3s3h", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "4c2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "4d2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "5c2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "5d2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "9c2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "Ac2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "Ad2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "Kc2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "Kd2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c3d", "Ts2s", MC_QUADS, DR_NONE },
            // 7c7d7h9c9d
            { "7c7d7h9c9d", "2d2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "3c2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "3c2d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "3d2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "3d3c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "4c2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "4c2d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "4c3c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "4c3d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "4d4c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "5c3c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "5c3d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "5c4c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "5c4d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "5d5c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "6c4c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "6c4d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "6c5c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "6c5d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "6d6c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "8c6c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "8c6d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "8d8c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ac2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ac2d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ac4c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "AcKc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "AcKd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "AcQc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "AcQd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ad2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ad4d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Jc8h", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "JcTc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "JcTd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Kc2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Kc2d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Kc3c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "KcJc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "KcJd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "KcQc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "KcQd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Kd2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Kd3d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Qc2c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "QcJc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "QcJd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "QcTc", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "QcTd", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Qd2d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Tc8c", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Tc8d", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "Ts8s", MC_NOTHING, DR_NONE },
            { "7c7d7h9c9d", "AdAc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "JdJc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "KdKc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "KsKc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "QdQc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "TdTc", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "TsTh", MC_FULL, DR_NONE },
            { "7c7d7h9c9d", "9h2c", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9h8c", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9h8d", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9s2c", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9s2d", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9s3c", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9s3d", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "9s8c", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Ac9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Ad9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Jc9h", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Jc9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Kc9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Kd9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Tc9h", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Tc9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "Ts9s", MC_TOPFULL, DR_NONE },
            { "7c7d7h9c9d", "7s2c", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "7s5c", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "7s5d", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "7s6c", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "7s6d", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "8c7s", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "8s7s", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "9h7s", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "9s7s", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "9s9h", MC_QUADS, DR_NONE },
            { "7c7d7h9c9d", "Ts7s", MC_QUADS, DR_NONE },
            // KcKdKhAcAd
            { "KcKdKhAcAd", "2d2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "3c2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "3c2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "3d2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "3d3c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "4c2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "4c2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "4c3c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "4c3d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "4d4c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "5c3c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "5c3d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "5c4c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "5c4d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "5d5c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "6c4c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "6c4d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "6c5c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "6c5d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "6d6c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "7c5c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "7c5d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "7c6c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "7c6d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "7d7c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8c6c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8c6d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8c7c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8c7d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8d8c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8s2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8s2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8s3c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "8s3d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9c7c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9c7d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9c8c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9c8d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9d9c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "9s4c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jc2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jc3c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jc8s", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jc9c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jc9d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "JcTc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "JcTd", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jd2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jd3d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Jd8s", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "JdJc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qc2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qc4c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qc8s", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "QcJc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "QcJd", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "QcTc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "QcTd", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qd2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qd4d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Qd8s", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "QdQc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Tc2c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Tc8c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Tc8d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Tc9c", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Tc9d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Td2d", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "TdTc", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "TsTh", MC_NOTHING, DR_NONE },
            { "KcKdKhAcAd", "Ah2c", MC_TOPFULL, DR_NONE },
            { "KcKdKhAcAd", "AhQc", MC_TOPFULL, DR_NONE },
            { "KcKdKhAcAd", "AhQd", MC_TOPFULL, DR_NONE },
            { "KcKdKhAcAd", "As2c", MC_TOPFULL, DR_NONE },
            { "KcKdKhAcAd", "AsTs", MC_TOPFULL, DR_NONE },
            { "KcKdKhAcAd", "AhKs", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "AsAh", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "AsKs", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "Ks2c", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "Ks7c", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "KsJc", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "KsJd", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "KsQc", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "KsQd", MC_QUADS, DR_NONE },
            { "KcKdKhAcAd", "KsTs", MC_QUADS, DR_NONE },
            // 3c3d3h2c2d
            { "3c3d3h2c2d", "4c2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "4c2s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "4d2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "5c2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "5c4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "5c4d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "5d2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "6c4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "6c4d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "6c5c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "6c5d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "7c5c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "7c5d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "7c6c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "7c6d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "8c6c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "8c6d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "8c7c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "8c7d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9c7c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9c7d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9c8c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9c8d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9s4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9s4d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9s5c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "9s5d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ac2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ac4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ac6c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ac9s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "AcKc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "AcKd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "AcQc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "AcQd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ad2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ad6d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ad9s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Jc9c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Jc9d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "JcTc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "JcTd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "JcTh", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kc2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kc4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kc5c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kc9s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "KcJc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "KcJd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "KcQc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "KcQd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kd2h", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kd5d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Kd9s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Qc4c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "QcJc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "QcJd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "QcTc", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "QcTd", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Qd4d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Tc8c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Tc8d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Tc9c", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Tc9d", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "Ts9s", MC_NOTHING, DR_NONE },
            { "3c3d3h2c2d", "4d4c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "5d5c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "6d6c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "7d7c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "8d8c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "9d9c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "9s9c", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "AdAc", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "JdJc", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "KdKc", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "QdQc", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "TdTc", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "TsTh", MC_FULL, DR_NONE },
            { "3c3d3h2c2d", "2s2h", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "3s2h", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "3s2s", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "4c3s", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "5c3s", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "9c3s", MC_QUADS, DR_NONE },
            { "3c3d3h2c2d", "Ts3s", MC_QUADS, DR_NONE },
            // 2c2d2h3c4c
            { "2c2d2h3c4c", "7c5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "7c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "8c6d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "8c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "9c7d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "9c8d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "9h5c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "9h6c", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Jc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "JcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Js5d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "QcJd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "QcTd", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Tc8d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Tc9d", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Ts9s", MC_NOTHING, DR_NONE },
            { "2c2d2h3c4c", "Kc5d", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "Kc9h", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "KdTc", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "Kh6c", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "2c2d2h3c4c", "Ac6d", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "Ac9h", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "AcKd", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "AdTc", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "Ah7c", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "2c2d2h3c4c", "6c5d", MC_STRAIGHT, DR_NONE },
            { "2c2d2h3c4c", "Ac5d", MC_STRAIGHT, DR_NONE },
            { "2c2d2h3c4c", "Ad5d", MC_STRAIGHT, DR_NONE },
            { "2c2d2h3c4c", "As5s", MC_STRAIGHT, DR_NONE },
            { "2c2d2h3c4c", "7c5c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "7c6c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "8c6c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "8c7c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "9c7c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "9c8c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Ac7c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "AcKc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "AcQc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Jc9c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "JcTc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Kc5c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Kc6c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "KcJc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "KcQc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Qc5c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "QcJc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "QcTc", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Tc8c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "Tc9c", MC_FLUSH, DR_NONE },
            { "2c2d2h3c4c", "3h3d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "4d3d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "4d3h", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "4h4d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "5c3d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "5c3h", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "5c4d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "5c4h", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "5d5c", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "6c4d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "6c4h", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "6d6c", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "7d7c", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "8d8c", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "9d9c", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "9s3h", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "AdAc", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "JdJc", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "Kd4d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "KdKc", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "Qd3d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "QdQc", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "Tc3d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "Tc4d", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "TdTc", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "TsTh", MC_FULL, DR_NONE },
            { "2c2d2h3c4c", "3d2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "3h2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "4d2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "4h2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "5c2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "6c2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "9d2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "Ac2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "Ad2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "Kc2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "Kd2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "Ts2s", MC_QUADS, DR_NONE },
            { "2c2d2h3c4c", "6c5c", MC_STRFLUSH, DR_NONE },
            { "2c2d2h3c4c", "Ac5c", MC_STRFLUSH, DR_NONE },
            // 2c2d3c3d4c
            { "2c2d3c3d4c", "7c5d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "7c6d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "8c6d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "8c7d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "9c7d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "9c8d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "9h5c", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "9h6c", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "9s5d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Jc9d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "JcTd", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Js5d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "QcJd", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "QcTd", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Tc8d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Tc9d", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Ts9s", MC_NOTHING, DR_NONE },
            { "2c2d3c3d4c", "Kc5d", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "Kc9h", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "KcJd", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "KcQd", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "Kd5d", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "Kd9s", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "Kh6c", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "KsTs", MC_KINGHIGH, DR_NONE },
            { "2c2d3c3d4c", "Ac6d", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "Ac9h", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "AcKd", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "AcQd", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "Ad6d", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "Ad9s", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "Ah7c", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "AsTs", MC_ACEHIGH, DR_NONE },
            { "2c2d3c3d4c", "5c4d", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "5c4h", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "6c4d", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "6c4h", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "9s4d", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "Ac4d", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "Qd4d", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "Ts4s", MC_TOPPAIR, DR_NONE },
            { "2c2d3c3d4c", "5d5c", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "6d6c", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "7d7c", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "8d8c", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "9d9c", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "AdAc", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "JdJc", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "KdKc", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "QdQc", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "TdTc", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "TsTh", MC_OVERPAIR, DR_NONE },
            { "2c2d3c3d4c", "6c5d", MC_STRAIGHT, DR_NONE },
            { "2c2d3c3d4c", "Ac5d", MC_STRAIGHT, DR_NONE },
            { "2c2d3c3d4c", "As5s", MC_STRAIGHT, DR_NONE },
            { "2c2d3c3d4c", "7c5c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "7c6c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "8c6c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "8c7c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "9c7c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "9c8c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Ac7c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "AcKc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "AcQc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Jc9c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "JcTc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Kc5c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Kc6c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "KcJc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "KcQc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Qc5c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "QcJc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "QcTc", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Tc8c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "Tc9c", MC_FLUSH, DR_NONE },
            { "2c2d3c3d4c", "3h2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "3h2s", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "4d2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "4d2s", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "4d3h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "4d3s", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "4h4d", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "5c2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "5c3h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "5c3s", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "5d2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "6c2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "9d3h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "Ac2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "Ad2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "Kc2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "Kd2h", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "Ts3s", MC_FULL, DR_NONE },
            { "2c2d3c3d4c", "2s2h", MC_QUADS, DR_NONE },
            { "2c2d3c3d4c", "3s3h", MC_QUADS, DR_NONE },
            { "2c2d3c3d4c", "6c5c", MC_STRFLUSH, DR_NONE },
            { "2c2d3c3d4c", "Ac5c", MC_STRFLUSH, DR_NONE },
            // 9c9d9h8c8d
            { "9c9d9h8c8d", "2d2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "3c2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "3c2d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "3d2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "3d3c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "4c2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "4c2d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "4c3c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "4c3d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "4d4c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "5c3c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "5c3d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "5c4c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "5c4d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "5d5c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "6c4c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "6c4d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "6c5c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "6c5d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "6d6c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "7c5c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "7c5d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "7c6c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "7c6d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "7d7c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "8h2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "8h6c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "8h6d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "8h7c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "8h7d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ac2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ac2d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ac4c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "AcKc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "AcKd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "AcQc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "AcQd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ad2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ad4d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Jc4h", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "JcTc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "JcTd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Kc2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Kc2d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Kc3c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "KcJc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "KcJd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "KcQc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "KcQd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Kd2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Kd3d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Qc2c", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "QcJc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "QcJd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "QcTc", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "QcTd", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Qd2d", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Tc8h", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Tc8s", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "Ts8s", MC_NOTHING, DR_NONE },
            { "9c9d9h8c8d", "AdAc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "JdJc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "KdKc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "KsKc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "QdQc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "TdTc", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "TsTh", MC_FULL, DR_NONE },
            { "9c9d9h8c8d", "8s8h", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s2c", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s2d", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s3c", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s3d", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s7c", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s7d", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s7h", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s8h", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "9s8s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Ac9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Ad9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Jc9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Kc9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Kd9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Tc9s", MC_QUADS, DR_NONE },
            { "9c9d9h8c8d", "Ts9s", MC_QUADS, DR_NONE },
        };
        std::string e;
        int mal = 0, vistos = 0;
        std::string primero;
        bool hechas[MC_COUNT] = { false }, proyectos[DR_COUNT] = { false };
        for (const Caso& c : casos) {
            std::vector<int> b;
            if (!parse_board(c.board, b, e)) { truth(std::string("board ") + c.board, false, e); continue; }
            const int c1 = parse_card(std::string(c.mano).substr(0, 2));
            const int c2 = parse_card(std::string(c.mano).substr(2, 2));
            if (c1 < 0 || c2 < 0) { truth(std::string("hand ") + c.mano, false, "no parsea"); continue; }
            ++vistos;
            hechas[c.hecho] = true;
            proyectos[c.proy] = true;
            const int got = made_cat(c1, c2, b);
            const int gotd = draw_cat(c1, c2, b);
            if (got != c.hecho || gotd != c.proy) {
                ++mal;
                if (mal <= 8)
                    primero += std::string(mal > 1 ? " | " : "") + c.mano + "@" +
                               c.board + " nuestro:" + MC_NAME[got] + "/" + DR_NAME[gotd] +
                               " esperado:" + MC_NAME[c.hecho] + "/" + DR_NAME[c.proy];
            }
        }
        truth("there are reference cases to check against", vistos > 3000,
              std::to_string(vistos) + " casos");
        int faltan = 0;
        for (int i = 0; i < MC_COUNT; ++i) if (!hechas[i]) ++faltan;
        for (int i = 0; i < DR_COUNT; ++i) if (!proyectos[i]) ++faltan;
        truth("and they cover every made-hand and every draw", faltan == 0,
              std::to_string(faltan) + " cajones sin un solo caso");
        truth("and every one of them agrees with the reference", mal == 0,
              std::to_string(mal) + " de " + std::to_string(vistos) +
              " no cuadran; el primero: " + primero);
    }

    void a_draw_belongs_to_the_hand(Session& S) {
        std::string e;
        struct Caso { const char* board; const char* mano; int esperado; };
        const Caso casos[] = {
            // 9h7h2c: dos corazones fuera, y el 9-7-2 no conecta nada.
            { "9h7h2c", "AhKh", DR_FLUSH },        // dos corazones mas: cuatro
            { "9h7h2c", "5h4h", DR_FLUSH },
            { "9h7h2c", "AhKd", DR_NONE  },        // un solo corazon: tres
            { "9h7h2c", "Ts8s", DR_OESD  },        // el J y el 6 la ligan
            { "9h7h2c", "6s5s", DR_GUT   },        // solo el 8
            { "9h7h2c", "Th8h", DR_FLUSH_OESD },   // las dos cosas
            { "9h7h2c", "AsKd", DR_NONE  },        // nada de nada
            // El board ya lleva cuatro a escalera: la tiene todo el mundo, asi
            // que no es proyecto de nadie.
            { "9s8d7c6h", "AsKd", DR_NONE },
            // Y con cuatro corazones fuera, una mano sin corazones tampoco.
            { "9h7h2h5h", "AsKd", DR_NONE },
            // Y con un corazon en la mano sobre esos cuatro ya son CINCO: eso no
            // es proyecto, es color hecho. Este caso lo escribi al reves y me
            // corrigio la comprobacion, que es exactamente para lo que esta.
            { "9h7h2h5h", "AsKh", DR_NONE },
            // Tres fuera y uno en la mano si es proyecto.
            { "9h7h2h5c", "AsKh", DR_FLUSH },
            { "9h7h2h5c", "AsKd", DR_NONE },
            // En el river no queda carta: lo que no es, ya no sera.
            { "9h7h2c3d", "AhKh", DR_FLUSH },
            { "9h7h2c3d4s", "AhKh", DR_NONE },
        };
        for (const Caso& c : casos) {
            std::vector<int> b;
            if (!truth(std::string("board ") + c.board + " parses",
                       parse_board(c.board, b, e), e)) continue;
            const int c1 = parse_card(std::string(c.mano).substr(0, 2));
            const int c2 = parse_card(std::string(c.mano).substr(2, 2));
            if (!truth(std::string("hand ") + c.mano + " parses", c1 >= 0 && c2 >= 0)) continue;
            const int got = draw_cat(c1, c2, b);
            const char* q = (c.esperado == DR_NONE) ? "nothing" : DR_NAME[c.esperado];
            truth(std::string(c.mano) + " on " + c.board + " draws to " + q,
                  got == c.esperado,
                  std::string("it says ") + (got == DR_NONE ? "nothing" : DR_NAME[got]));
        }

        // Y entero: en un board de dos corazones tiene que aparecer una fila de
        // proyecto de color.
        if (!truth("draw spot builds", spot(S, "9h7h2c", e), e)) return;
        S.solve(150, 0);
        const NodeStats N = gather(*S.solver(), 0, S.tree().ctx[0].tree.root, 0,
                                   S.deal().identity());
        if (!truth("the node reads", N.ok)) return;
        const std::vector<ClassAgg> filas =
            aggregate_by_made(*S.solver(), N, S.deal().board);
        // Y las filas son DOS BLOQUES que no se cruzan, como en la referencia: el de mano
        // hecha y el de proyecto, y cada uno reparte el rango entero por su
        // cuenta. Antes habia una fila por cada par y de ahi salia "set +
        // flush_draw", que es una familia que la referencia no tiene: en su lenguaje de
        // filtros eso se pide con `set & flush_draw`.
        bool hay_color = false, hay_hecha = false;
        double suma_hecha = 0.0, suma_proy = 0.0;
        for (const ClassAgg& g : filas) {
            if (g.kind == AG_DRAW) {
                suma_proy += g.w;
                if (g.cls == DR_FLUSH || g.cls == DR_FLUSH_OESD) hay_color = true;
            } else {
                suma_hecha += g.w;
                hay_hecha = true;
            }
        }
        truth("two hearts out there means someone is drawing", hay_color,
              "ni una fila de proyecto de color");
        truth("and there are made-hand rows too", hay_hecha);
        close_to("the made-hand rows hold the whole range", suma_hecha, N.wtot, 1e-9);
        close_to("and the draw rows hold it too, on their own",
                 suma_proy, N.wtot, 1e-9);
        // Y ni una fila cruzada: los nombres son de una lista o de la otra,
        // nunca "esto + aquello".
        bool cruzada = false;
        for (const ClassAgg& g : filas)
            if (std::string(agg_name(g)).find(" + ") != std::string::npos) cruzada = true;
        truth("and no row is a crossed name", !cruzada);
    }

    void nodelock_bites(Session& S) {
        std::string e;
        if (!truth("lock spot builds", spot(S, "Ah9h4hKd", e), e)) return;
        S.solve(100, 0);

        const int root = S.tree().ctx[0].tree.root;
        const Node& n = S.tree().ctx[0].tree.nodes[static_cast<size_t>(root)];
        const int bet = S.tree().ctx[0].tree.action_index(n, AK_BET);
        if (!truth("root has a bet", bet >= 0)) return;

        std::vector<std::pair<std::string, double>> mix;
        mix.push_back(std::make_pair(std::string("B"), 1.0));
        int matched = 0;
        if (!truth("locks a wide range to always bet",
                   S.add_lock(0, root, "22+,A2s+", mix, matched, e), e)) return;
        truth("the lock matched some combos", matched > 0,
              "matched " + std::to_string(matched));

        S.solve(100, 0);
        std::vector<double> st(static_cast<size_t>(n.num_actions) * S.solver()->num_hands());
        S.solver()->avg_strategy_block(0, root, st.data());

        // Every locked combo must be betting outright.
        double worst = 1.0;
        for (int h = 0; h < S.solver()->num_hands(); ++h) {
            if (S.range(0)[static_cast<size_t>(h)] <= 0.0) continue;
            if (!S.solver()->is_hand_locked(0, root, 0, h)) continue;
            const double p = st[static_cast<size_t>(bet) * S.solver()->num_hands() + h];
            if (p < worst) worst = p;
        }
        if (record_)
            std::printf("  rec   %-46s lowest bet frequency %.10f\n", "nodelock", worst);
        else
            truth("locked combos bet 100%", worst > 0.999,
                  "lowest bet frequency among locked combos was " + std::to_string(worst));
    }
};
