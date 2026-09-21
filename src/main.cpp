// =============================================================================
//  DCFR SOLVER -- flop, turn or river
//
//  Copyright (C) 2026 PagaLaFanta
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//  Build (Windows / MinGW):
//    g++ -std=c++17 -O3 -march=native -Wall -Isrc -static -o solver src/main.cpp -lws2_32
//  Build (Linux / macOS):
//    g++ -std=c++17 -O3 -march=native -Wall -Isrc -pthread -o solver src/main.cpp
//
//  -Wall no es decoracion: el aviso que trae es el que caza un por ciento
//  suelto dentro de un printf sin argumentos, que es como la ayuda de la
//  consola llego a imprimir "less than this 25235616201f the pot". Compila sin
//  un solo aviso; si aparece uno, es que algo se rompio.
// =============================================================================

#ifdef _WIN32
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include <cmath>
#include "bench.hpp"
#include "default_spot.hpp"
#include "check.hpp"
#include "console.hpp"
#include "webui_page.hpp"
#include "webui.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void usage() {
    std::printf(
"DCFR Solver %s -- 52-card postflop solver, Discounted CFR\n"
"Copyright (C) 2026 PagaLaFanta.  Free software under the GNU GPL v3, with NO WARRANTY.\n"
"See the LICENSE file, or <https://www.gnu.org/licenses/gpl-3.0.html>.\n"
"\n"
"  solver --gui [port]    graphical UI in the browser (default port 8777)\n"
"                         add --no-open to skip launching the browser\n"
"  solver                 interactive text console\n"
"  solver --script FILE   run console commands from FILE, then exit\n"
"  solver --bench [board] timings for a standard spot, so the numbers in the\n"
"                         README can be checked instead of believed\n"
"                         --save NAME keeps a baseline, --vs NAME compares\n"
"  solver --check         regression checks; exit code 1 if anything moved\n"
"                         add --record to print the values instead\n"
"  solver --help\n"
"\n"
"Console commands can also be piped:   echo solve | solver\n", cfg::VERSION);
}

// El limite de memoria no deberia ser un numero fijo que el usuario tenga que
// corregir. Se lee la RAM de la maquina y se deja el 75%: en un equipo de 32 GB
// eso son 24, y en uno de 8 son 6, sin que nadie toque nada. El cuarto que
// queda es para el sistema y el navegador, que tambien tienen que vivir.
static double physical_ram_gb() {
#ifdef _WIN32
    MEMORYSTATUSEX st;
    st.dwLength = sizeof(st);
    if (GlobalMemoryStatusEx(&st))
        return static_cast<double>(st.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
#else
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long psize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && psize > 0)
        return static_cast<double>(pages) * static_cast<double>(psize)
             / (1024.0 * 1024.0 * 1024.0);
#endif
    return 0.0;   // no se pudo averiguar
}

// Doble clic desde el explorador, o lanzado desde una terminal.
//
// Importa porque la respuesta correcta es distinta: quien hace doble clic
// quiere la interfaz, y quien escribe `solver` en una consola quiere la consola.
// Sin distinguirlo hay que elegir a quien decepcionar, y hasta ahora el que
// perdia era el usuario normal: se encontraba una ventana negra con un prompt.
//
// Se distingue por quien es dueno de la ventana: al hacer doble clic, Windows
// crea la consola para nosotros y somos el unico proceso enganchado a ella. Con
// un shell delante, el shell tambien cuenta.
static bool launched_by_double_click() {
#ifdef _WIN32
    DWORD pids[4];
    const DWORD n = GetConsoleProcessList(pids, 4);
    return n == 1;
#else
    return false;      // en Linux y macOS se lanza desde una terminal
#endif
}

static void set_memory_limit_from_machine() {
    const double ram = physical_ram_gb();
    if (ram <= 0.0) return;                    // se queda el valor por defecto
    // Redondeado a GB enteros: el campo de la interfaz va de uno en uno, y un
    // 23,8817 ahi seria un valor que su propio campo rechaza.
    double v = std::floor(ram * 0.75);
    if (v < 1.0) v = 1.0;                      // por debajo de esto no hay nada que hacer
    cfg::MAX_MEM_GB = v;
}

int main(int argc, char** argv) {
    set_memory_limit_from_machine();
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

    Session session;

    // El spot de arranque, de dentro del binario. Ver default_spot.hpp: quien se
    // baja el .exe no tiene carpeta saves/ al lado, asi que sin esto la primera
    // pantalla son dos rangos vacios y un cartel de "define los rangos".
    //
    // Si fallara no se dice nada ni se para: es una comodidad, no un requisito,
    // y el programa tiene que arrancar igual. Lo que no puede es arrancar a
    // medias, asi que si no carga se deja el arranque de siempre.
    {
        // Si esto fallara seria porque el spot de dentro esta mal escrito, y de
        // eso se encarga una comprobacion (`the built-in spot loads`). Aqui no
        // se intenta recuperar nada: un arranque a medias seria peor que el
        // arranque vacio de antes.
        std::string e;
        (void)session.load_config_text(DEFAULT_SPOT, e);
    }

    // Y los rangos de fabrica, si no hay ninguno guardado. Es lo mismo que el
    // spot de arranque: quien abre esto por primera vez tiene que poder resolver
    // algo de verdad sin montar media hora de rangos a mano.
    (void)session.install_default_ranges();

    // Sin argumentos y abierto con doble clic: la interfaz, que es lo que
    // espera quien llega. Desde una terminal se mantiene la consola.
    if (args.empty() && launched_by_double_click()) {
        WebUI ui(session, 8777, true);
        return ui.run();
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string a = lower(args[i]);
        if (a == "--help" || a == "-h") { usage(); return 0; }
        if (a == "--bench") {
            std::string board = "Ah9h4h", save, against;
            for (size_t k = i + 1; k < args.size(); ++k) {
                if (args[k].rfind("--", 0) != 0) { board = args[k]; continue; }
                const std::string o = lower(args[k]);
                if ((o == "--save" || o == "--vs") && k + 1 < args.size()) {
                    (o == "--save" ? save : against) = args[k + 1];
                    ++k;
                }
            }
            Bench b;
            return b.run(board, save, against);
        }
        if (a == "--check") {
            bool record = false;
            for (size_t k = i + 1; k < args.size(); ++k)
                if (lower(args[k]) == "--record") record = true;
            Checks c(record);
            return c.run();
        }
        if (a == "--gui") {
            int  port = 8777;
            bool open_browser = true;
            for (size_t k = i + 1; k < args.size(); ++k) {
                int p;
                if (lower(args[k]) == "--no-open") open_browser = false;
                else if (parse_int(args[k], p) && p > 0 && p < 65536) port = p;
            }
            WebUI ui(session, port, open_browser);
            return ui.run();
        }
        if (a == "--script") {
            if (i + 1 >= args.size()) { std::printf("--script needs a file name\n"); return 2; }
            std::ifstream f(args[i + 1].c_str());
            if (!f) { std::printf("cannot open script '%s'\n", args[i + 1].c_str()); return 2; }
            msg::EN = true;
            Console c(session);
            return c.run(f, false);
        }
        std::printf("unknown option '%s'\n\n", args[i].c_str());
        usage();
        return 2;
    }
    // La consola esta en ingles de arriba abajo -- el help, los avisos, los
    // nombres de las columnas -- asi que sus errores tambien. El navegador lo
    // pide en cada peticion y manda sobre esto.
    msg::EN = true;
    Console c(session);
    return c.run(std::cin, true);
}
