#pragma once
// =============================================================================
//  Con una sala de poker abierta, esto no se abre.
//
//  Todas las salas prohiben en sus terminos usar ayuda en tiempo real mientras
//  juegas: un solver abierto al lado de la mesa es motivo de confiscacion de
//  fondos y cierre de cuenta, y con razon. Este programa es para estudiar
//  ANTES y DESPUES, no durante.
//
//  Asi que el propio programa se niega: mira los procesos que hay corriendo, y
//  si encuentra el cliente de una sala no arranca. Y si la abres con el
//  programa ya abierto, se cierra solo.
//
//  LO QUE ESTO ES Y LO QUE NO ES. Esto evita el accidente -- la sala abierta de
//  antes, la mesa en segundo plano que se te olvido, el "solo miro una cosa
//  rapida" -- que es como ocurre de verdad. No evita a quien quiera saltarselo:
//  el codigo es libre y se recompila en treinta segundos. No pretende otra
//  cosa, y decir que protege de algo mas seria mentir.
//
//  La lista son trozos de nombre en minusculas, y se busca dentro del nombre
//  del ejecutable. Se eligen trozos de MARCA y no la palabra "poker" a secas,
//  porque "poker" solo se lleva por delante a PokerTracker, a Hold'em Manager y
//  a cualquier cosa con poker en el nombre -- herramientas legitimas que se
//  usan justo para esto, para estudiar.
// =============================================================================

#include "msg.hpp"

#include <cctype>
#include <string>
#include <vector>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <tlhelp32.h>
#else
  #include <cstdio>
  #include <dirent.h>
#endif

namespace rooms {

// Que se busca, y como se llama la sala cuando se encuentra.
struct Sala {
    const char* trozo;    // en minusculas, se busca DENTRO del nombre del proceso
    const char* nombre;   // lo que se le dice al usuario
};

// Las salas que pidio el autor, con los nombres con los que salen sus clientes.
// Una sala que falte se anade aqui y ya esta: es una linea.
inline const Sala SALAS[] = {
    { "pokerstars",     "PokerStars" },
    { "ggpoker",        "GGPoker" },
    { "ggnetwork",      "GGPoker" },
    { "winamax",        "Winamax" },
    { "888poker",       "888poker" },
    { "poker888",       "888poker" },
    { "pacificpoker",   "888poker" },      // el nombre viejo del cliente de 888
    { "coinpoker",      "CoinPoker" },
    { "ipoker",         "iPoker" },
    { "titanpoker",     "iPoker (Titan)" },
    { "redstarpoker",   "iPoker (Red Star)" },
    { "betfairpoker",   "iPoker (Betfair)" },
    { "williamhillpoker", "iPoker (William Hill)" },
    { "netbetpoker",    "iPoker (NetBet)" },
};

inline std::string lower_ascii(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s)
        r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// El nombre de la sala si el proceso es de una, o vacio.
//
// Aparte para poder comprobarlo: mirar los procesos de la maquina no se puede
// hacer en una bateria -- depende de lo que tenga abierto quien la corra -- y
// lo que hay que comprobar es justo esto, que la lista reconoce lo que tiene
// que reconocer y no se lleva por delante lo que no.
inline std::string room_of_process(const std::string& exe) {
    const std::string n = lower_ascii(exe);
    for (const Sala& s : SALAS)
        if (n.find(s.trozo) != std::string::npos) return s.nombre;
    return std::string();
}

// Todos los procesos de la maquina, por nombre de ejecutable.
inline std::vector<std::string> process_names() {
    std::vector<std::string> out;
#if defined(_WIN32)
    const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do { out.push_back(pe.szExeFile); } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
#else
    // En Linux, /proc/<pid>/comm. En macOS no hay /proc y aqui no se mira nada:
    // el programa se reparte para Windows y decir que vigila donde no vigila
    // seria peor que no vigilar.
    DIR* d = opendir("/proc");
    if (!d) return out;
    while (struct dirent* e = readdir(d)) {
        const char* p = e->d_name;
        bool num = *p != '\0';
        for (const char* q = p; *q; ++q) if (*q < '0' || *q > '9') num = false;
        if (!num) continue;
        const std::string ruta = std::string("/proc/") + p + "/comm";
        if (std::FILE* f = std::fopen(ruta.c_str(), "rb")) {
            char buf[256];
            const size_t n = std::fread(buf, 1, sizeof buf - 1, f);
            std::fclose(f);
            buf[n] = '\0';
            std::string s(buf);
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
            if (!s.empty()) out.push_back(s);
        }
    }
    closedir(d);
#endif
    return out;
}

// La primera sala que este abierta, o vacio si no hay ninguna.
inline std::string open_room() {
    for (const std::string& p : process_names()) {
        const std::string sala = room_of_process(p);
        if (!sala.empty()) return sala;
    }
    return std::string();
}

// Lo que se le dice a quien la tiene abierta. En los dos idiomas, porque esto
// puede ser lo primero -- y lo unico -- que el programa llegue a decir.
inline std::string why_not(const std::string& sala) {
    return std::string(M("No se abre con una sala de poker abierta: ",
                         "This does not open with a poker room running: ")) +
           sala +
           M(".\n\n  Todas las salas prohiben en sus terminos usar ayuda en tiempo\n"
             "  real mientras juegas. Este programa es para estudiar antes y\n"
             "  despues, no durante.\n\n"
             "  Cierra la sala y vuelve a abrirlo.\n",
             ".\n\n  Every poker room's terms forbid real-time assistance while you\n"
             "  play. This program is for studying before and after, not during.\n\n"
             "  Close the room and open it again.\n");
}

}  // namespace rooms
