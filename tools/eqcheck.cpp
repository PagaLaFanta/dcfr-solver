// Equity por fuerza bruta, para arbitrar.
//
// Nuestra compute_equity dice que AA vale 85% en Ah9h4h y la referencia dice 80,38.
// Comparar dos programas no decide nada: uno de los dos esta mal y no se sabe
// cual. Asi que aqui se cuenta a mano -- todas las parejas de turn y river, un
// showdown por cada una -- y se pone al lado de lo que dice la nuestra.
//
// Usa eval7, que esta comprobado aparte contra tablas conocidas. Lo unico que
// se pone a prueba es la CONTABILIDAD de la equity, que es lo que difiere.
//
//   g++ -std=c++17 -O2 -Isrc -o eqcheck tools/eqcheck.cpp
//   eqcheck saves/configs/A94hh.cfg AA
#include "cards.hpp"
#include "config.hpp"
#include "deal.hpp"
#include "range.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("uso: eqcheck <config> <clase, p.ej. AA> [oop|ip]\n");
        return 2;
    }
    const std::string quiero = argv[2];
    const bool hero_es_ip = (argc < 4) || std::string(argv[3]) != "oop";

    std::string board_spec, oop_spec, ip_spec;
    {
        std::ifstream f(argv[1]);
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream is(line);
            std::string k;
            is >> k;
            std::string rest;
            std::getline(is, rest);
            while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            if (k == "board") board_spec = rest;
            else if (k == "oop") oop_spec = rest;
            else if (k == "ip") ip_spec = rest;
        }
    }
    if (board_spec.empty()) { std::printf("no hay board en el config\n"); return 2; }

    std::vector<int> bc;
    std::string e;
    if (!parse_board(board_spec, bc, e)) { std::printf("board: %s\n", e.c_str()); return 2; }
    Deal d;
    if (!d.build(bc, e)) { std::printf("deal: %s\n", e.c_str()); return 2; }

    std::vector<double> ro, ri;
    if (!parse_range(oop_spec, d, ro, e)) { std::printf("oop: %s\n", e.c_str()); return 2; }
    if (!parse_range(ip_spec, d, ri, e)) { std::printf("ip: %s\n", e.c_str()); return 2; }
    const std::vector<double>& hero = hero_es_ip ? ri : ro;
    const std::vector<double>& vill = hero_es_ip ? ro : ri;

    // Lo que dice la nuestra, con los palos colapsados y sin colapsar. Si
    // difieren, la equity depende de una optimizacion que no deberia verse.
    std::vector<double> con, sin;
    d.build_orbits(ro, ri, true, nullptr);
    const int runouts_con = d.num_runouts;
    const int grupo = static_cast<int>(d.use_group.size());
    compute_equity(d, vill, con);
    d.build_orbits(ro, ri, false, nullptr);
    const int runouts_sin = d.num_runouts;
    compute_equity(d, vill, sin);

    // Las cartas que quedan en la baraja.
    std::vector<int> libres;
    for (int c = 0; c < 52; ++c) {
        bool en_board = false;
        for (int b : bc) if (b == c) en_board = true;
        if (!en_board) libres.push_back(c);
    }

    std::printf("board %s   hero %s   %d cartas libres\n",
                board_spec.c_str(), hero_es_ip ? "IP" : "OOP", (int)libres.size());
    std::printf("  isomorfia on: %d runouts (grupo de %d)   off: %d runouts\n",
                runouts_con, grupo, runouts_sin);
    std::printf("  %-6s  %9s  %9s  %9s\n", "combo", "iso on", "iso off", "a mano");

    double pn = 0, pm = 0, pw = 0;
    for (int h = 0; h < d.num(); ++h) {
        if (hero[(size_t)h] <= 0.0) continue;
        const Combo& k = d.combos[(size_t)h];
        if (class_name(k.cls) != quiero) continue;

        // Fuerza bruta: cada turn/river, cada mano del rival compatible.
        double num = 0.0, den = 0.0;
        for (size_t i = 0; i < libres.size(); ++i) {
            const int t = libres[i];
            if (t == k.c1 || t == k.c2) continue;
            for (size_t j = i + 1; j < libres.size(); ++j) {
                const int r = libres[j];
                if (r == k.c1 || r == k.c2) continue;
                int mine[7] = { bc[0], bc[1], bc[2], t, r, k.c1, k.c2 };
                const int sm = eval7(mine);
                for (int o = 0; o < d.num(); ++o) {
                    const double w = vill[(size_t)o];
                    if (w <= 0.0) continue;
                    const Combo& v = d.combos[(size_t)o];
                    if (v.c1 == k.c1 || v.c1 == k.c2 || v.c2 == k.c1 || v.c2 == k.c2) continue;
                    if (v.c1 == t || v.c1 == r || v.c2 == t || v.c2 == r) continue;
                    int his[7] = { bc[0], bc[1], bc[2], t, r, v.c1, v.c2 };
                    const int so = eval7(his);
                    den += w;
                    if (sm > so) num += w;
                    else if (sm == so) num += 0.5 * w;
                }
            }
        }
        const double mano = den > 0 ? 100.0 * num / den : 0.0;
        std::printf("  %s%s  %9.4f  %9.4f  %9.4f\n",
                    card_str(k.c1).c_str(), card_str(k.c2).c_str(),
                    con[(size_t)h], sin[(size_t)h], mano);
        const double pw1 = hero[(size_t)h];
        pn += pw1 * con[(size_t)h]; pm += pw1 * mano; pw += pw1;
    }
    if (pw > 0)
        std::printf("  %-6s  %9.4f  %9s  %9.4f   (media de la clase, por peso)\n",
                    quiero.c_str(), pn / pw, "", pm / pw);
    return 0;
}
