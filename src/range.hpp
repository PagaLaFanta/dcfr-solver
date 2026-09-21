#pragma once
// =============================================================================
//  Ranges.
//
//  A range is a weight vector over the Deal's combo list (see deal.hpp), where
//  zero simply means "not in my range". One shared indexing keeps the CFR core
//  free of per-player combo bookkeeping.
//
//  Range syntax:  AA, KK, AKs, AKo, AK, QQ+, AJs+, KTo+, 55-88, T9s-76s,
//                 A5s-A2s, AsKd (a specific combo), any token followed by
//                 :weight, or `random` / `all` for everything.
// =============================================================================

#include "msg.hpp"
#include "cards.hpp"
#include "config.hpp"
#include "deal.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
//  Range parsing
// -----------------------------------------------------------------------------
namespace rangedet {

inline std::string strip(const std::string& s) {
    std::string o;
    for (char c : s) if (!std::isspace(static_cast<unsigned char>(c))) o += c;
    return o;
}

// One class token without '+' or '-': "AA", "AKs", "AKo", "AK".
// Fills hi/lo rank and suitedness (-1 both, 0 offsuit, 1 suited).
inline bool one_class(const std::string& t, int& hi, int& lo, int& suited) {
    if (t.size() < 2 || t.size() > 3) return false;
    const int r1 = parse_rank(t[0]);
    const int r2 = parse_rank(t[1]);
    if (r1 < 0 || r2 < 0) return false;
    hi = std::max(r1, r2);
    lo = std::min(r1, r2);
    suited = -1;
    if (t.size() == 3) {
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(t[2])));
        if (c == 's') suited = 1;
        else if (c == 'o') suited = 0;
        else return false;
        if (hi == lo) return false;   // "AAs" is meaningless
    }
    if (hi == lo) suited = -1;
    return true;
}

inline void push_classes(int hi, int lo, int suited, std::vector<int>& out) {
    if (hi == lo) {
        out.push_back(class_index(card_of(hi, 0), card_of(lo, 1)));
        return;
    }
    if (suited != 0) out.push_back(class_index(card_of(hi, 0), card_of(lo, 0)));
    if (suited != 1) out.push_back(class_index(card_of(hi, 0), card_of(lo, 1)));
}

// Expands one range token into class indices.
inline bool expand(const std::string& tokIn, std::vector<int>& out, std::string& err) {
    const std::string tok = tokIn;

    // ---- interval: "55-88", "T9s-76s", "A5s-A2s" ---------------------------
    const size_t dash = tok.find('-');
    if (dash != std::string::npos && dash > 0 && dash + 1 < tok.size()) {
        int h1, l1, s1, h2, l2, s2;
        if (!one_class(tok.substr(0, dash), h1, l1, s1) ||
            !one_class(tok.substr(dash + 1), h2, l2, s2)) {
            err = M("intervalo de rango mal escrito: '", "bad range interval '") + tok + "'";
            return false;
        }
        if (s1 != s2) { err = M("los dos extremos de '", "both ends of '") + tok +
              M("' tienen que ser los dos suited o los dos offsuit",
                "' need the same suitedness"); return false; }

        if (h1 == l1 && h2 == l2) {                       // pairs
            for (int r = std::min(h1, h2); r <= std::max(h1, h2); ++r)
                push_classes(r, r, -1, out);
            return true;
        }
        if (h1 == l1 || h2 == l2) { err = M("no se pueden mezclar parejas y no parejas en '",
                    "cannot mix pairs and non-pairs in '") + tok + "'"; return false; }
        if (h1 == h2) {                                   // same high card, walk the kicker
            for (int r = std::min(l1, l2); r <= std::max(l1, l2); ++r)
                push_classes(h1, r, s1, out);
            return true;
        }
        if (h1 - l1 == h2 - l2) {                         // same gap, walk both
            const int gap = h1 - l1;
            for (int h = std::min(h1, h2); h <= std::max(h1, h2); ++h)
                push_classes(h, h - gap, s1, out);
            return true;
        }
        err = "'" + tok + M("' no es una línea recta de la cuadrícula",
                         "' is not a straight line through the grid");
        return false;
    }

    // ---- open ended: "QQ+", "AJs+", "KTo+" ---------------------------------
    if (!tok.empty() && tok[tok.size() - 1] == '+') {
        int hi, lo, su;
        if (!one_class(tok.substr(0, tok.size() - 1), hi, lo, su)) {
            err = M("no se entiende '", "bad token '") + tok + "'";
            return false;
        }
        if (hi == lo) {
            for (int r = hi; r <= 12; ++r) push_classes(r, r, -1, out);
        } else {
            for (int r = lo; r < hi; ++r) push_classes(hi, r, su, out);
        }
        return true;
    }

    // ---- plain class -------------------------------------------------------
    int hi, lo, su;
    if (one_class(tok, hi, lo, su)) {
        push_classes(hi, lo, su, out);
        return true;
    }
    err = M("esto no es una mano ni un rango: '",
            "unrecognised range token '") + tok + "'";
    return false;
}

} // namespace rangedet

// Parses `spec` into a weight per combo of `d`. Weights start at zero; later
// tokens overwrite earlier ones for the combos they touch.
inline bool parse_range(const std::string& spec, const Deal& d,
                        std::vector<double>& w, std::string& err) {
    w.assign(static_cast<size_t>(d.num()), 0.0);
    const std::string s = rangedet::strip(spec);
    if (s.empty()) { err = M("el rango está vacío", "empty range"); return false; }

    std::string lowered;
    for (char c : s) lowered += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lowered == "random" || lowered == "all" || lowered == "100%" || lowered == "any") {
        for (double& x : w) x = 1.0;
        return true;
    }

    // Split on commas or semicolons: different tools export different ones and
    // there is nothing to gain by refusing one of them.
    std::vector<std::string> toks;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == ';') { if (!cur.empty()) toks.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) toks.push_back(cur);

    for (std::string tok : toks) {
        double weight = 1.0;
        const size_t colon = tok.find(':');
        if (colon != std::string::npos) {
            try {
                size_t pos = 0;
                weight = std::stod(tok.substr(colon + 1), &pos);
                if (pos != tok.size() - colon - 1) throw 0;
            } catch (...) {
                err = M("peso mal escrito en '", "bad weight in '") + tok + "'";
                return false;
            }
            if (weight < 0.0) { err = M("peso negativo en '", "negative weight in '") + tok + "'"; return false; }
            // A weight above one is almost always a percentage written as a
            // number: someone means half and types 50. Taken at face value it
            // makes that hand fifty times likelier than the rest of the range
            // and quietly ruins the solve, so it is refused rather than
            // guessed at -- guessing would be just as silent when wrong.
            if (weight > 1.0) {
                err = "weight " + tok.substr(colon + 1) + " in '" + tok.substr(0, colon) +
                      "' is above 1. Weights run 0 to 1, so half is ':0.5', not ':50'";
                return false;
            }
            tok = tok.substr(0, colon);
        }

        // A ten is 'T' here, but plenty of people and plenty of exports write
        // it as '10'. Done after the weight is split off, so ':10' is not
        // mistaken for a rank.
        for (size_t z = tok.find("10"); z != std::string::npos; z = tok.find("10", z))
            tok = tok.substr(0, z) + "T" + tok.substr(z + 2);

        // A specific combo, "AsKd"?
        if (tok.size() == 4) {
            const int a = parse_card(tok.substr(0, 2));
            const int b = parse_card(tok.substr(2, 2));
            if (a >= 0 && b >= 0) {
                if (a == b) { err = "'" + tok + "' repeats a card"; return false; }
                const int idx = d.index_of(a, b);
                if (idx >= 0) w[static_cast<size_t>(idx)] = weight;
                continue;   // blocked by the board: silently nothing to weight
            }
        }

        std::vector<int> classes;
        if (!rangedet::expand(tok, classes, err)) return false;
        for (int cl : classes)
            for (int i = 0; i < d.num(); ++i)
                if (d.combos[static_cast<size_t>(i)].cls == cl)
                    w[static_cast<size_t>(i)] = weight;
    }
    return true;
}

// There used to be four "hand types" here -- nuts, value, bluff-catcher, air --
// cut at three equity percentages you could set. They are gone. The cuts were
// invented for this solver and nothing else uses them; a hand does not stop
// being a bluff-catcher because a number moved from 38 to 40, and every table
// they appeared in was really a table about those three numbers. Equity itself
// is still there, per combo, which is the part that was measured rather than
// decided.


// Showdown equity of every combo against `vill`, averaged over every runout the
// board still has to come (percent, 0..100). One O(N) sweep per runout, so a
// full flop -- 1176 runouts -- costs a few million operations, not the billions
// a pairwise enumeration would.
//
// `ya` son las cartas que YA salieron por encima del board de partida, y limita
// el promedio a los runouts que empiezan asi. Sin eso, la equity de un nodo de
// turn es equity de FLOP: promedia los 1.176 runouts sin mirar cual salio, y en
// Ah9h4h/Kd un KsQs marcaba 26,80 -- lo que vale como carta alta en el flop --
// cuando ya es pareja de reyes. Se ve sin calcular nada: una pareja de reyes no
// puede valer menos que una de damas en un board con rey, y la marcaba.
//
// Vacio significa "desde el board de partida", que es lo que quiere la equity
// del rango inicial, y ese es el unico sitio donde no sobra.
inline void compute_equity(const Deal& d, const std::vector<double>& vill,
                           std::vector<double>& eq,
                           const std::vector<int>& ya = std::vector<int>()) {
    const int n = d.num();
    std::vector<double> num(static_cast<size_t>(n), 0.0), den(static_cast<size_t>(n), 0.0);
    std::vector<double> ro(static_cast<size_t>(n)), win(static_cast<size_t>(n)),
                        lose(static_cast<size_t>(n)), compat;
    std::vector<SdEntry> pk;

    // Equity is wanted for every hand on the board, not just a range's.
    const std::vector<unsigned char> all(static_cast<size_t>(n), 1);

    for (int r = 0; r < d.num_runouts; ++r) {
        int t, rv;
        d.runout_cards(r, t, rv);
        // Un runout es un PAR SIN ORDEN: se enumeran con j > i sobre las
        // ranuras, asi que {Kd,2c} figura una vez y con el 2c primero. Comparar
        // por posicion no acertaba nunca -- el filtro descartaba los 1.176 y la
        // equity salia cero para todas las manos, el color de nueces incluido.
        // Lo que toca es pertenencia: en un nodo de turn el Kd tiene que ser
        // una de las dos, y en uno de river las dos tienen que estar.
        if (!ya.empty()) {
            bool vale = true;
            for (size_t i = 0; i < ya.size(); ++i)
                if (ya[i] != t && ya[i] != rv) { vale = false; break; }
            if (!vale) continue;
        }
        for (int h = 0; h < n; ++h) ro[static_cast<size_t>(h)] = vill[static_cast<size_t>(h)];
        if (t  >= 0) for (int h : d.with_card[static_cast<size_t>(t)])  ro[static_cast<size_t>(h)] = 0.0;
        if (rv >= 0) for (int h : d.with_card[static_cast<size_t>(rv)]) ro[static_cast<size_t>(h)] = 0.0;

        compat_counts(d, ro.data(), n, compat);
        pack_sweep(d, &d.order[static_cast<size_t>(r) * n],
                   &d.scores[static_cast<size_t>(r) * n], n, nullptr, pk);
        showdown_split(pk.data(), static_cast<int>(pk.size()),
                       ro.data(), win.data(), lose.data(), all.data());

        for (int h = 0; h < n; ++h) {
            if (t  >= 0 && d.blocks(h, t))  continue;
            if (rv >= 0 && d.blocks(h, rv)) continue;
            const double c = compat[static_cast<size_t>(h)];
            const double w = win[static_cast<size_t>(h)], l = lose[static_cast<size_t>(h)];
            num[static_cast<size_t>(h)] += w + 0.5 * (c - w - l);
            den[static_cast<size_t>(h)] += c;
        }
    }
    eq.assign(static_cast<size_t>(n), 0.0);
    for (int h = 0; h < n; ++h)
        eq[static_cast<size_t>(h)] = den[static_cast<size_t>(h)] > 1e-12
            ? 100.0 * num[static_cast<size_t>(h)] / den[static_cast<size_t>(h)] : 0.0;
}
