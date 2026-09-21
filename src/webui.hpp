#pragma once
// =============================================================================
//  Local web UI.
//
//  The binary serves a single page on 127.0.0.1 and the browser is the front
//  end, so the whole thing stays one self-contained executable with no GUI
//  toolkit.
//
//  Navigation is stateless here -- the client sends the full address
//  (context, node, dealt card slots) with every request -- so the page can hold
//  its own history without the server and the console keep its own cursor.
//
//  One thread per connection. It has to be: reading a node mid-solve waits for
//  the solver's current chunk, and on a single-threaded server that stalls the
//  progress polls queued behind it, which makes a perfectly healthy solve look
//  frozen. Progress and stop touch nothing but atomics, so they never block.
// =============================================================================

#include "msg.hpp"
#include "session.hpp"
#include "trainer.hpp"
#include "rooms.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef _WIN32
  // Lo mismo que en main.cpp: sin esto, los macros min/max de windows.h
  // se llevan por delante cualquier std::max de las cabeceras que vengan
  // detras, y solo se ve compilando con MSVC.
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define SOCK_INVALID INVALID_SOCKET
  #define SOCK_CLOSE   closesocket
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  #define SOCK_INVALID (-1)
  #define SOCK_CLOSE   close
#endif

// -----------------------------------------------------------------------------
inline std::string url_decode(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) {
            o += static_cast<char>(std::strtol(s.substr(i + 1, 2).c_str(), nullptr, 16));
            i += 2;
        } else o += s[i];
    }
    return o;
}

inline std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char b[8];
                    std::snprintf(b, sizeof(b), "\\u%04x", c);
                    o += b;
                } else o += c;
        }
    }
    return o;
}
inline std::string jnum(double v) {
    if (v != v) return "null";
    char b[32];
    std::snprintf(b, sizeof(b), "%.6g", v);
    return b;
}
inline std::string jstr(const std::string& s) { return "\"" + json_escape(s) + "\""; }

struct Params {
    std::vector<std::pair<std::string, std::string>> kv;
    void parse(const std::string& s) {
        size_t i = 0;
        while (i < s.size()) {
            size_t amp = s.find('&', i);
            if (amp == std::string::npos) amp = s.size();
            const std::string item = s.substr(i, amp - i);
            const size_t eq = item.find('=');
            if (eq != std::string::npos)
                kv.push_back(std::make_pair(url_decode(item.substr(0, eq)),
                                            url_decode(item.substr(eq + 1))));
            i = amp + 1;
        }
    }
    bool has(const std::string& k) const {
        for (const auto& p : kv) if (p.first == k) return true;
        return false;
    }
    std::string get(const std::string& k, const std::string& d = "") const {
        for (const auto& p : kv) if (p.first == k) return p.second;
        return d;
    }
    double getd(const std::string& k, double d) const {
        double v;
        return parse_double(get(k), v) ? v : d;
    }
    int geti(const std::string& k, int d) const {
        int v;
        return parse_int(get(k), v) ? v : d;
    }
};

// Holds the session's read lock for as long as it is alive.
struct SessionRead {
    Session& s;
    bool     on;
    SessionRead(Session& ss, bool active) : s(ss), on(active) { if (on) s.read_lock(); }
    ~SessionRead() { if (on) s.read_unlock(); }
};

// =============================================================================
extern const char* const WEBUI_PAGE;

class WebUI {
public:
    WebUI(Session& s, int port, bool open_browser = true)
        : S(s), port_(port), open_(open_browser) {}

    // The state the page reads, built without a socket. `--check` uses it to
    // confirm that every field the script reaches for is a field this really
    // sends: a typo there is not a crash, it is `undefined` rendered into the
    // page, which is worse, because it looks like data.
    std::string state_for_check() const { return api_state(); }
    // En que idioma contesta el motor a ESTA peticion. Lo dice la pagina, no el
    // servidor: el idioma es de quien mira, y el servidor no sabe quien mira.
    // Si no viene nada, ingles, que es como sale de fabrica el programa: hay que
    // PEDIR el español, igual que en la pagina.
    static void language_from(const Params& q, const Params& form) {
        const std::string lang = lower(q.has("lang") ? q.get("lang") : form.get("lang"));
        msg::EN = (lang != "es");
    }
    // Lo que manda el panel del engranaje al dar a Aplicar, sin socket: la
    // suite tiene que poder comprobar que hacerlo sin tocar nada no cuesta la
    // solucion que hay en pantalla.
    // El script de varios boards, sin socket.
    std::string script_for_check(const std::string& query) {
        Params q;
        q.parse(query);
        return api_script(q);
    }
    std::string config_for_check(const std::string& query) {
        Params q;
        q.parse(query);
        return api_config(q);
    }
    // El entrenador, sin socket. La suite tiene que poder mirar la respuesta
    // ENTERA: que el consejo no viaje cuando esta apagado no se puede
    // comprobar mirando la pantalla, hay que mirar el JSON.
    std::string train_for_check(const std::string& query) {
        Params q;
        q.parse(query);
        return api_train(q);
    }
    Trainer& trainer_for_check() { return train_; }

    // Arrancar y parar el servidor desde otro hilo, y saber en que puerto ha
    // quedado. Existe porque la suite tiene que levantarlo de verdad: el
    // bloqueo que dejo la interfaz muerta a mitad de un solve no lo veia
    // ninguna prueba que llamara a los metodos por dentro, hacia falta un
    // socket y peticiones abortadas de verdad. Con puerto 0 lo elige el
    // sistema, asi que la prueba no choca con la interfaz que este abierta.
    // ---------------------------------------------------------------------
    //  Cerrarse cuando se va el navegador.
    //
    //  MEDIDO despues de que alguien lo notara usandolo: cerrabas la pestana y
    //  el proceso seguia vivo con 551 MB y 19 hilos dentro, sin ventana y sin
    //  nada que pudiera cerrarlo. Lo unico que apagaba el servidor era el
    //  guardia de salas. Abrir el programa tres veces en una semana dejaba
    //  gigabyte y medio ocupado por nada.
    //
    //  La regla esta aqui suelta y es pura a proposito: asi la bateria la
    //  prueba entera -- los cuatro casos en los que NO hay que cerrarse y el
    //  unico en el que si -- sin levantar un servidor ni esperar segundos.
    //
    //  Los cuatro frenos, y por que cada uno:
    //
    //    - Si no ha llegado a conectarse ningun navegador, no se cierra nunca.
    //      Con --no-open el servidor se levanta para conectarse luego, y la
    //      bateria y los scripts lo usan asi. "La pagina se fue" y "la pagina
    //      no vino" no son lo mismo.
    //    - Si queda alguna pestana viva, no. Cerrar una de dos no cierra el
    //      programa.
    //    - Si hay un solve corriendo, tampoco. Cerrar la pestana a media
    //      resolucion de un flop y perder cuarenta minutos seria peor que la
    //      fuga que esto arregla. Cuando acabe y siga sin haber nadie, se ira.
    //    - Y aun sin nadie, hay una espera de gracia: recargar la pagina
    //      manda primero el adios y solo despues vuelve a latir.
    static bool hay_que_cerrarse(bool hubo_navegador, size_t pestanas_vivas,
                                 bool resolviendo, double segundos_sin_nadie,
                                 double gracia) {
        if (!hubo_navegador) return false;
        if (pestanas_vivas > 0) return false;
        if (resolviendo) return false;
        return segundos_sin_nadie >= gracia;
    }

    void vi_una_pestana(const std::string& id) {
        if (id.empty()) return;
        std::lock_guard<std::mutex> g(vistos_mtx_);
        vistos_[id] = std::chrono::steady_clock::now();
        hubo_navegador_.store(true);
    }

    void se_fue_una_pestana(const std::string& id) {
        if (id.empty()) return;
        std::lock_guard<std::mutex> g(vistos_mtx_);
        vistos_.erase(id);
    }

    // Cuantas pestanas siguen vivas, tirando las que llevan demasiado calladas.
    //
    //  El plazo es largo a proposito: Chrome frena los temporizadores de una
    //  pestana que no esta a la vista hasta UNA VEZ POR MINUTO. Con latido de
    //  diez segundos y plazo de ciento cincuenta hay dos minutos y medio de
    //  margen sobre el peor caso, asi que tener la pestana de fondo no mata el
    //  programa. Lo que cierra rapido es el adios, no este plazo.
    size_t pestanas_vivas() {
        const auto ahora = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> g(vistos_mtx_);
        for (auto it = vistos_.begin(); it != vistos_.end(); ) {
            const double callada =
                std::chrono::duration<double>(ahora - it->second).count();
            if (callada > mudez_) it = vistos_.erase(it); else ++it;
        }
        return vistos_.size();
    }

    void shutdown() {
        stop_.store(true);
        const sock_t s = srv_.exchange(SOCK_INVALID);
        if (s != SOCK_INVALID) SOCK_CLOSE(s);   // despierta al accept
    }
    int  bound_port() const { return bound_.load(); }
    void set_quiet(bool q) { quiet_ = q; }

    int run() {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { std::printf("winsock init failed\n"); return 1; }
#endif
        sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
        if (srv == SOCK_INVALID) { std::printf("cannot create socket\n"); return 1; }
        int yes = 1;
#ifdef _WIN32
        // En Windows, SO_REUSEADDR NO significa lo que significa en Linux: deja
        // que un SEGUNDO proceso se ate al mismo puerto, y los dos se quedan
        // escuchando. Comprobado: dos solvers en el 8790, los dos diciendo que
        // sirven, y las conexiones repartidas sin regla.
        //
        // Y pasa de verdad, porque la gente hace doble clic otra vez cuando
        // parece que no pasa nada. Montas un spot, lo resuelves, refrescas, y te
        // contesta la OTRA instancia con los rangos vacios. Se lee exactamente
        // como "el programa me ha borrado el trabajo".
        //
        // SO_EXCLUSIVEADDRUSE es lo contrario: nadie mas se ata a este puerto, y
        // el segundo bind falla como debe, para poder decirlo.
        setsockopt(srv, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char*>(&yes), sizeof(yes));
#else
        // En POSIX si hace falta: sin esto, cerrar y volver a abrir enseguida
        // choca con el TIME_WAIT del socket anterior.
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#endif

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(static_cast<unsigned short>(port_));
        addr.sin_addr.s_addr = htonl(0x7F000001);   // 127.0.0.1 only

        if (bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            // Casi siempre es el propio solver ya abierto, asi que se dice
            // eso y adonde ir, en vez de un "bind failed" que no ayuda.
            std::printf("\n  El puerto %d ya esta ocupado.\n"
                        "  Lo normal es que ya tengas un solver abierto: mira en\n"
                        "  http://127.0.0.1:%d\n"
                        "  Si quieres otro a la vez, dale otro puerto:  solver --gui %d\n\n",
                        port_, port_, port_ + 1);
            std::fflush(stdout);
            SOCK_CLOSE(srv);
            return 1;
        }
        if (listen(srv, 16) != 0) { std::printf("listen failed\n"); SOCK_CLOSE(srv); return 1; }

        // Con port_ == 0 el puerto lo elige el sistema, asi que hay que
        // preguntarlo: este es el unico sitio donde se sabe cual toco.
        {
            sockaddr_in got;
            socklen_t n = sizeof(got);
            if (getsockname(srv, reinterpret_cast<sockaddr*>(&got), &n) == 0)
                port_ = ntohs(got.sin_port);
        }
        srv_.store(srv);
        bound_.store(port_);

        if (!quiet_) {
        std::printf("\n  DCFR Solver -- web UI\n  Open  http://127.0.0.1:%d\n"
                    "  Ctrl+C here to stop.\n\n", port_);
        std::fflush(stdout);
        }
#ifdef _WIN32
        if (open_) {
            char cmd[160];
            std::snprintf(cmd, sizeof(cmd), "start \"\" \"http://127.0.0.1:%d\"", port_);
            std::system(cmd);
        }
#endif
        // El vigilante: si una sala de poker se abre con esto ya abierto, se
        // cierra solo. Mirar los procesos cuesta un par de milisegundos, asi
        // que cada tres segundos no se nota y es de sobra: lo que hay que
        // evitar es tener las dos cosas abiertas a la vez, no reaccionar en el
        // mismo instante.
        std::thread vigia([this]() {
            // Desde cuando no queda ninguna pestana. En cero, queda alguna.
            std::chrono::steady_clock::time_point solo{};
            while (!stop_.load()) {
                for (int i = 0; i < 30 && !stop_.load(); ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (stop_.load()) break;

                const std::string sala = rooms::open_room();
                if (!sala.empty()) {
                    sala_.assign(sala);
                    shutdown();
                    break;
                }

                // Y si el navegador se fue, esto no pinta nada encendido.
                const auto ahora = std::chrono::steady_clock::now();
                const size_t vivas = pestanas_vivas();
                if (vivas > 0 || !hubo_navegador_.load() || S.busy()) {
                    solo = std::chrono::steady_clock::time_point{};
                    continue;
                }
                if (solo == std::chrono::steady_clock::time_point{}) {
                    solo = ahora;
                    // Dicho en voz alta: la consola esta a la vista, y asi no
                    // parece que el programa se muera solo y sin avisar.
                    if (!quiet_) {
                        std::printf("  %s\n",
                                    M("No queda ninguna pestana abierta. Cerrando "
                                      "en unos segundos; vuelve a abrir la pagina "
                                      "si no era eso.",
                                      "No tab is open any more. Closing in a few "
                                      "seconds; open the page again if that was "
                                      "not the idea."));
                        std::fflush(stdout);
                    }
                    continue;
                }
                const double sin_nadie =
                    std::chrono::duration<double>(ahora - solo).count();
                if (hay_que_cerrarse(hubo_navegador_.load(), vivas, S.busy(),
                                     sin_nadie, gracia_)) {
                    se_fue_ = true;
                    shutdown();
                    break;
                }
            }
        });

        while (!stop_.load()) {
            sock_t cl = accept(srv, nullptr, nullptr);
            if (cl == SOCK_INVALID) {
                // Cerrar el socket de escucha es como shutdown() despierta a
                // este accept. Sin mirar la bandera esto seria un bucle cerrado
                // girando sobre un socket ya muerto.
                if (stop_.load()) break;
                continue;
            }
            std::thread([this, cl]() {
                // Un cliente que se conecta y no termina de mandar su peticion
                // se queda en recv para siempre, y con el el hilo y el socket.
                // Un navegador manda la peticion entera de golpe, asi que
                // treinta segundos sobran de largo; lo que no sobra es dejar
                // que uno a medias se quede ahi. Se descubrio colgando la
                // propia prueba de estres, que mandaba las cabeceras con saltos
                // de linea pelados y nunca cerraba el bloque.
                recv_timeout(cl, 30000);
                // A stray exception on one connection must not take the whole
                // server -- and the solve running behind it -- down.
                try { handle(cl); } catch (...) {}
                SOCK_CLOSE(cl);
            }).detach();
        }
        if (vigia.joinable()) vigia.join();
        const sock_t s = srv_.exchange(SOCK_INVALID);
        if (s != SOCK_INVALID) SOCK_CLOSE(s);
        bound_.store(0);
        // Si lo cerro una sala, hay que decirlo: si no, la ventana desaparece y
        // parece que el programa se ha muerto.
        if (!sala_.empty()) {
            std::printf("%s", rooms::why_not(sala_).c_str());
            return 3;
        }
        // Como con las salas: una ventana que desaparece sin explicacion
        // parece un cuelgue, no un cierre.
        if (se_fue_ && !quiet_) {
            std::printf("\n  %s\n\n",
                        M("Cerrado: se fue el navegador.",
                          "Closed: the browser went away."));
            std::fflush(stdout);
        }
        return 0;
    }

private:
    Session&   S;
    Trainer    train_;
    // La sala que obligo a cerrar, si fue eso. Vacio si se cerro por lo normal.
    std::string sala_;
    int        port_;
    bool       open_ = true;
    bool       quiet_ = false;
    std::atomic<bool>   stop_{false};
    std::atomic<sock_t> srv_{SOCK_INVALID};
    std::atomic<int>    bound_{0};
    std::mutex api_mtx_;   // serialises everything except progress and stop

    // Las pestanas abiertas, por el identificador que se inventa cada una al
    // cargarse, y cuando se supo de ellas por ultima vez.
    std::mutex vistos_mtx_;
    std::map<std::string, std::chrono::steady_clock::time_point> vistos_;
    std::atomic<bool> hubo_navegador_{false};
    // Segundos. Se pueden acortar para probarlo sin esperar minutos.
    double mudez_  = 150.0;
    double gracia_ = 10.0;
    // Si se cerro porque se fue el navegador, hay que decirlo.
    bool   se_fue_ = false;

    // Ceilings on what one connection may make this process allocate.
    enum : size_t { MAX_HEADERS = 1u << 20, MAX_BODY = 1u << 20 };

    static void recv_timeout(sock_t s, int ms) {
#ifdef _WIN32
        const DWORD v = static_cast<DWORD>(ms);
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&v), sizeof(v));
#else
        timeval v;
        v.tv_sec  = ms / 1000;
        v.tv_usec = (ms % 1000) * 1000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &v, sizeof(v));
#endif
    }

    static void send_all(sock_t s, const std::string& d) {
        size_t sent = 0;
        while (sent < d.size()) {
            const int n = send(s, d.data() + sent, static_cast<int>(d.size() - sent), 0);
            if (n <= 0) return;
            sent += static_cast<size_t>(n);
        }
    }
    static void respond(sock_t s, const std::string& body, const char* ctype) {
        char head[256];
        std::snprintf(head, sizeof(head),
                      "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
                      "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
                      ctype, static_cast<unsigned long>(body.size()));
        send_all(s, std::string(head) + body);
    }

    void handle(sock_t cl) {
        std::string req;
        char buf[8192];
        while (req.find("\r\n\r\n") == std::string::npos) {
            const int n = recv(cl, buf, sizeof(buf), 0);
            if (n <= 0) return;
            req.append(buf, static_cast<size_t>(n));
            if (req.size() > MAX_HEADERS) return;
        }
        const size_t hend = req.find("\r\n\r\n") + 4;
        size_t clen = 0;
        {
            const std::string low = lower(req.substr(0, hend));
            const size_t p = low.find("content-length:");
            if (p != std::string::npos)
                clen = static_cast<size_t>(std::strtoull(req.c_str() + p + 15, nullptr, 10));
        }
        // The headers are already capped; the body needs the same. Content-Length
        // is a number the client chose, and without a bound a single connection
        // claiming a gigabyte would keep this thread reading until it ran the
        // process out of memory. The largest thing anyone legitimately posts here
        // is a range string, which is a few kilobytes.
        if (clen > MAX_BODY) return;
        while (req.size() < hend + clen) {
            const int n = recv(cl, buf, sizeof(buf), 0);
            if (n <= 0) break;
            req.append(buf, static_cast<size_t>(n));
        }
        const std::string body = req.substr(hend, clen);

        const size_t sp1 = req.find(' ');
        const size_t sp2 = req.find(' ', sp1 + 1);
        if (sp1 == std::string::npos || sp2 == std::string::npos) return;
        const std::string target = req.substr(sp1 + 1, sp2 - sp1 - 1);

        std::string path = target;
        Params q, form;
        const size_t qm = target.find('?');
        if (qm != std::string::npos) { path = target.substr(0, qm); q.parse(target.substr(qm + 1)); }
        form.parse(body);

        language_from(q, form);

        // While a solve is running the worker owns the solver, so only the
        // progress and stop endpoints may be served; everything else would race.
        S.reap();
        if (path == "/" || path == "/index.html") {
            respond(cl, WEBUI_PAGE, "text/html; charset=utf-8");
        } else if (path == "/api/progress") {
            // Mientras el solve corre, atomicos y nada mas: esto no puede
            // esperar por nada o la barra se congela justo cuando hace falta.
            // Cuando ya no corre, la respuesta lleva el estado dentro, y eso
            // recorre el arbol: se construye bajo el mismo lock que /api/state.
            if (S.busy()) {
                respond(cl, api_progress(false), "application/json");
            } else {
                std::string body;
                {
                    std::lock_guard<std::mutex> api(api_mtx_);
                    SessionRead rd(S, S.busy());
                    body = api_progress(true);
                }
                respond(cl, body, "application/json");
            }
        } else if (path == "/api/stop") {
            S.request_stop();
            respond(cl, "{\"ok\":true,\"note\":\"stopping\"}", "application/json");
        } else if (path == "/api/ping") {
            // Fuera del candado del solve, como /api/stop: un latido que se
            // queda esperando a que acabe un flop no es un latido.
            vi_una_pestana(form.get("id"));
            respond(cl, "{\"ok\":true}", "application/json");
        } else if (path == "/api/bye") {
            // La pestana avisa de que se va. Llega por sendBeacon, que el
            // navegador manda aunque ya este cerrando la ventana.
            se_fue_una_pestana(form.get("id"));
            respond(cl, "{\"ok\":true}", "application/json");
        } else if (path == "/api/flops") {
            respond(cl, api_flops(q), "application/json");
        } else if (path == "/api/state" || path == "/api/node" ||
                   path == "/api/runouts") {
            // Read-only views may run mid-solve: taking the solver mutex parks
            // the worker between chunks, so the numbers are consistent. It does
            // cost the solve a little, which is the honest price of watching.
            //
            // The body is built under the lock and sent after releasing it --
            // holding a solver mutex across socket I/O would stall the worker
            // for as long as the client takes to read.
            std::string body;
            {
                std::lock_guard<std::mutex> api(api_mtx_);
                SessionRead rd(S, S.busy());
                body = (path == "/api/state")   ? api_state()
                     : (path == "/api/runouts") ? api_runouts(q)
                     :                            api_node(q);
            }
            respond(cl, body, "application/json");
        } else if (S.busy()) {
            respond(cl, "{\"ok\":false,\"busy\":true,\"note\":\"" + std::string(M("hay un solve en marcha", "a solve is running")) + "\"}",
                    "application/json");
        }
        else if (path == "/api/config" || path == "/api/solve" ||
                 path == "/api/lock"   || path == "/api/unlock" ||
                 path == "/api/saves"  || path == "/api/store" ||
                 path == "/api/script" || path == "/api/train") {
            std::lock_guard<std::mutex> api(api_mtx_);
            std::string body;
            if (path == "/api/config")      body = api_config(form);
            else if (path == "/api/train")  body = api_train(form);
            else if (path == "/api/script") body = api_script(form);
            else if (path == "/api/solve")  body = api_solve(form);
            else if (path == "/api/lock")   body = api_lock(form);
            else if (path == "/api/saves")  body = api_saves();
            else if (path == "/api/store")  body = api_store(form);
            else                            body = api_unlock(form);
            respond(cl, body, "application/json");
        }
        else {
            const std::string b = "{\"error\":\"not found\"}";
            char head[160];
            std::snprintf(head, sizeof(head),
                          "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n"
                          "Content-Length: %lu\r\nConnection: close\r\n\r\n",
                          static_cast<unsigned long>(b.size()));
            send_all(cl, std::string(head) + b);
        }
    }

    // ---------------------------------------------------------------------
    static std::vector<int> parse_slots(const std::string& s) {
        std::vector<int> out;
        int v;
        for (const std::string& t : split(s, ','))
            if (!t.empty() && parse_int(t, v)) out.push_back(v);
        return out;
    }
    std::string json_tree() const {
        const GameTree& T = S.tree();
        const std::map<std::pair<int, int>, std::string> nl = node_lines(T);
        std::string j = "[";
        for (size_t ci = 0; ci < T.ctx.size(); ++ci) {
            const RoundCtx& rc = T.ctx[ci];
            if (ci) j += ",";
            j += "{\"id\":" + std::to_string(ci) +
                 ",\"street\":" + std::to_string(rc.street) +
                 ",\"streetName\":" + jstr(STREET_NAME[rc.street]) +
                 ",\"label\":" + jstr(rc.label) +
                 ",\"parent\":" + std::to_string(rc.parent) +
                 ",\"parentCont\":" + std::to_string(rc.parent_cont) +
                 ",\"pot\":" + jnum(rc.pot) +
                 ",\"root\":" + std::to_string(rc.tree.root) +
                 ",\"instances\":" + std::to_string(rc.instances) +
                 ",\"nodes\":[";
            for (size_t ni = 0; ni < rc.tree.nodes.size(); ++ni) {
                const Node& n = rc.tree.nodes[ni];
                if (ni) j += ",";
                j += "{\"id\":" + std::to_string(ni) +
                     ",\"type\":" + std::to_string(static_cast<int>(n.type)) +
                     ",\"player\":" + std::to_string(n.player) +
                     ",\"pot\":" + jnum(n.pot) +
                     ",\"path\":" + jstr(n.path) +
                     ",\"depth\":" + std::to_string(n.depth) +
                     ",\"locked\":" + (n.is_locked ? "true" : "false");
                // El nombre de la linea, para poder cruzar este nodo con su
                // frecuencia total sin que la pagina lo tenga que reconstruir.
                {
                    std::map<std::pair<int, int>, std::string>::const_iterator li =
                        nl.find(std::make_pair(ci, static_cast<int>(ni)));
                    if (li != nl.end()) j += ",\"line\":" + jstr(li->second);
                }
                if (n.type == NT_CONT)
                    j += ",\"contCtx\":" +
                         std::to_string(rc.cont_ctx[static_cast<size_t>(n.cont_id)]);
                j += ",\"actions\":[";
                for (int a = 0; a < n.num_actions; ++a) {
                    if (a) j += ",";
                    j += "{\"code\":" + jstr(rc.tree.act(n, a).code) +
                         ",\"label\":" + jstr(rc.tree.act(n, a).label) +
                         ",\"kind\":" + std::to_string(static_cast<int>(rc.tree.act(n, a).kind)) +
                         ",\"to\":" + jnum(rc.tree.act(n, a).to_amount) +
                         ",\"child\":" + std::to_string(rc.tree.child(n, a)) + "}";
                }
                j += "]}";
            }
            j += "]}";
        }
        return j + "]";
    }

    std::string json_range(int p) const {
        const Deal& D = S.deal();
        double sum[169] = { 0.0 };
        int    cnt[169] = { 0 };
        for (int h = 0; h < D.num(); ++h) {
            const int c = D.combos[static_cast<size_t>(h)].cls;
            sum[c] += S.range(p)[static_cast<size_t>(h)];
            cnt[c]++;
        }
        std::string j = "[";
        for (int c = 0; c < 169; ++c) {
            if (c) j += ",";
            j += jnum(cnt[c] ? sum[c] / cnt[c] : 0.0);
        }
        return j + "]";
    }


    // Flops al azar para llenar una lista.
    //
    // Sin repetir y sin equivalentes: dos flops que son el mismo con los palos
    // cambiados de nombre tienen la misma solucion, y resolver los dos es pagar
    // dos veces por la misma respuesta.
    // -----------------------------------------------------------------
    //  EL ENTRENADOR
    //
    //  Una sola ruta con un `op` dentro, porque son estados de la misma
    //  partida y partirlos en seis rutas no compra nada: cada respuesta es la
    //  mesa entera tal y como hay que pintarla, y asi la pagina nunca tiene
    //  que juntar dos respuestas para saber que ensena.
    //
    //  El consejo -- frecuencias y EV por accion -- SOLO viaja si esta
    //  encendido. Mandarlo siempre y esconderlo en la pagina seria mentira:
    //  esta en el JSON, se abre el inspector y ahi esta la respuesta.
    // -----------------------------------------------------------------
    std::string api_train(const Params& f) {
        const std::string op = lower(f.get("op"));
        std::string e;

        if (f.has("side"))   train_.set_side(f.geti("side", 0));
        // Donde empieza la mano. startCtx < 0 quiere decir "por el principio".
        if (f.has("startCtx")) {
            const int c = f.geti("startCtx", 0);
            if (c < 0) train_.start_at_root();
            else train_.set_start(c, f.geti("startNode", -1),
                                  parse_slots(f.get("startSlots")));
        }
        if (f.has("advice")) {
            bool v = false;
            if (parse_onoff(f.get("advice"), v)) train_.set_advice(v);
        }

        if (op == "stop")  { train_.stop(); return train_json(); }
        if (op == "reset") { train_.reset_score(); train_.stop(); return train_json(); }
        if (op == "new" || op == "start") {
            // Con semilla: la mano que se pide, entera. Es lo que hace que el
            // historial se pueda volver a jugar y no sea solo una lista.
            unsigned semilla = 0;
            if (f.has("seed"))
                semilla = static_cast<unsigned>(
                    std::strtoul(f.get("seed").c_str(), nullptr, 10));
            if (!train_.new_hand(S, e, semilla)) return err_json(e);
            return train_json();
        }
        if (op == "repeat") {
            if (!train_.repeat_hand(S, e)) return err_json(e);
            return train_json();
        }
        if (op == "act") {
            if (!train_.act(S, f.get("action"), e)) return err_json(e);
            return train_json();
        }
        if (op == "state" || op.empty()) return train_json();
        return err_json(M("no sé qué es eso", "no idea what that is"));
    }

    static std::string err_json(const std::string& e) {
        return "{\"ok\":false,\"error\":" + jstr(e) + "}";
    }

    static std::string cards_json(const int* c, int n) {
        std::string j = "[";
        for (int i = 0; i < n; ++i) {
            if (i) j += ",";
            j += jstr(c[i] >= 0 ? card_str(c[i]) : std::string("?"));
        }
        return j + "]";
    }

    // La mesa entera. Lo que la pagina necesita para pintar, y nada mas.
    std::string train_json() {
        const Trainer& T = train_;
        std::string j = "{\"ok\":true";
        j += ",\"on\":"      + std::string(T.on() ? "true" : "false");
        j += ",\"over\":"    + std::string(T.ended() ? "true" : "false");
        j += ",\"side\":"    + std::to_string(T.side());
        j += ",\"advice\":"  + std::string(T.advice() ? "true" : "false");
        j += ",\"seed\":"    + std::to_string(static_cast<long long>(T.seed()));
        j += ",\"hand\":"    + std::to_string(T.hand_number());
        j += ",\"street\":"  + jstr(T.street() >= 0 && T.street() <= 2
                                        ? STREET_NAME[T.street()] : "");
        j += ",\"pot\":"     + jnum(T.pot());
        j += ",\"potMid\":"  + jnum(T.pot_middle());
        j += ",\"toCall\":"  + jnum(T.to_call());
        j += ",\"stack\":"   + jnum(cfg::STACK);
        // Lo de la mesa: lo que le queda a cada uno y lo que tiene delante.
        j += ",\"heroStack\":" + jnum(T.stack_of(T.side()));
        j += ",\"villStack\":" + jnum(T.stack_of(1 - T.side()));
        j += ",\"heroFront\":" + jnum(T.in_front(T.side()));
        j += ",\"villFront\":" + jnum(T.in_front(1 - T.side()));
        j += ",\"pot0\":"    + jnum(cfg::POT0);
        j += ",\"you\":"     + jstr(T.hero_category());
        j += ",\"startCtx\":"  + std::to_string(T.start_ctx());
        j += ",\"startNode\":" + std::to_string(T.start_node());
        j += ",\"atRoot\":"    + std::string(T.starts_at_root() ? "true" : "false");
        j += ",\"startLine\":" + jstr(T.start_line());

        j += ",\"board\":[";
        for (size_t i = 0; i < T.board().size(); ++i) {
            if (i) j += ",";
            j += jstr(card_str(T.board()[i]));
        }
        j += "]";
        j += ",\"hero\":"    + cards_json(T.hero(), 2);
        // Las del rival solo cuando se ensenan: mandarlas tapadas y esconderlas
        // en la pagina seria ensenarlas a quien mire la respuesta.
        if (T.villain_shown()) j += ",\"villain\":" + cards_json(T.villain(), 2);
        else                   j += ",\"villain\":null";

        j += ",\"actions\":[";
        for (size_t a = 0; a < T.codes().size(); ++a) {
            if (a) j += ",";
            j += "{\"code\":" + jstr(T.codes()[a]) +
                 ",\"label\":" + jstr(T.labels()[a]) +
                 ",\"to\":" + jnum(T.amounts()[a]);
            if (T.advice() && a < T.freqs().size()) {
                j += ",\"freq\":" + jnum(T.freqs()[a]) +
                     ",\"ev\":"   + jnum(T.evs()[a]);
            }
            j += "}";
        }
        j += "]";

        j += ",\"log\":[";
        for (size_t i = 0; i < T.log().size(); ++i) {
            if (i) j += ",";
            j += jstr(T.log()[i]);
        }
        j += "]";

        // El repaso de la mano: una fila por decision tuya, con lo que costo.
        // Viaja al terminar la mano -- ahi ya no se le adelanta nada a nadie --
        // y mientras juegas solo si el consejo esta encendido.
        j += ",\"steps\":[";
        if (T.ended() || T.advice()) {
            for (size_t i = 0; i < T.steps().size(); ++i) {
                const TrainStep& p = T.steps()[i];
                if (i) j += ",";
                j += "{\"street\":" + jstr(p.street) +
                     ",\"line\":"   + jstr(p.line) +
                     ",\"pot\":"    + jnum(p.pot) +
                     ",\"loss\":"   + jnum(p.loss) +
                     ",\"chosen\":" + std::to_string(p.chosen) +
                     ",\"best\":"   + std::to_string(p.best) +
                     ",\"acts\":[";
                for (int a = 0; a < p.A; ++a) {
                    if (a) j += ",";
                    j += "{\"code\":" + jstr(p.codes[static_cast<size_t>(a)]) +
                         ",\"label\":" + jstr(p.labels[static_cast<size_t>(a)]) +
                         ",\"freq\":" + jnum(p.freq[static_cast<size_t>(a)]) +
                         ",\"ev\":"   + jnum(p.ev[static_cast<size_t>(a)]) + "}";
                }
                j += "]}";
            }
        }
        j += "]";

        if (T.ended()) {
            j += ",\"result\":"   + jnum(T.result());
            j += ",\"showdown\":" + std::string(T.showdown() ? "true" : "false");
            j += ",\"heroMade\":" + jstr(T.hero_made());
            j += ",\"villMade\":" + jstr(T.villain_made());
        }

        j += ",\"score\":{\"hands\":" + std::to_string(T.hands_played()) +
             ",\"decisions\":" + std::to_string(T.decisions()) +
             ",\"lost\":"      + jnum(T.lost()) +
             ",\"won\":"       + jnum(T.won()) +
             ",\"lossPct\":"   + jnum(T.loss_pct()) + "}";
        return j + "}";
    }

    std::string api_flops(const Params& q) {
        int n = q.geti("n", 10);
        if (n < 1) n = 1;
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
            if (!vistos.insert(flop_canon(c[0], c[1], c[2])).second) continue;
            salen.push_back(card_str(c[0]) + card_str(c[1]) + card_str(c[2]));
        }
        std::string j = "{\"ok\":true,\"flops\":[";
        for (size_t i = 0; i < salen.size(); ++i) {
            if (i) j += ",";
            j += jstr(salen[i]);
        }
        return j + "]}";
    }

    // El script de varios boards.
    //
    // Lo escribe el SERVIDOR y no la pagina porque el script tiene que llevar el
    // spot dentro: los rangos, los tamaños, el bote, el stack. La pagina no los
    // tiene todos, y un script que resuelve veinte boards con un spot que no es
    // el tuyo es una noche tirada.
    //
    // Se resuelve guardando la config con un nombre y cargandola en la primera
    // linea. Asi el script es una receta corta y legible, y el spot es un
    // fichero que se puede volver a mirar.
    std::string api_script(const Params& f) {
        const std::string nombre = trim(f.get("name"));
        const std::string patron = trim(f.get("pattern")).empty()
                                 ? std::string("{board}") : trim(f.get("pattern"));
        const bool guardar = f.get("save") != "0";
        const double acc = f.getd("acc", S.acc_target());
        const double tmo = f.getd("timeout", 0.0);
        const int iters  = f.geti("iters", S.iters());

        // Los boards, uno por linea. Se comprueban TODOS antes de escribir nada:
        // enterarse de que la linea 14 estaba mal escrita a las tres de la
        // mañana, con trece arboles resueltos y el resto sin tocar, es
        // exactamente lo que esto viene a evitar.
        std::vector<std::string> boards;
        {
            const std::string txt = f.get("boards");
            size_t i = 0;
            int linea = 0;
            while (i <= txt.size()) {
                const size_t f2 = txt.find('\n', i);
                const std::string ln = trim(txt.substr(i, (f2 == std::string::npos ? txt.size() : f2) - i));
                i = (f2 == std::string::npos) ? txt.size() + 1 : f2 + 1;
                ++linea;
                if (ln.empty() || ln[0] == '#') continue;
                std::vector<int> cs;
                std::string e;
                if (!parse_board(ln, cs, e) || cs.size() < 3 || cs.size() > 5) {
                    char b[220];
                    std::snprintf(b, sizeof b,
                        M("linea %d, '%s': %s", "line %d, '%s': %s"),
                        linea, ln.c_str(),
                        e.empty() ? M("un board son 3, 4 o 5 cartas",
                                      "a board is 3, 4 or 5 cards") : e.c_str());
                    return "{\"ok\":false,\"note\":" + jstr(b) + "}";
                }
                boards.push_back(ln);
            }
        }
        if (boards.empty())
            return "{\"ok\":false,\"note\":" +
                   jstr(M("no hay ningun board en la lista", "no boards in the list")) + "}";
        // Un nombre que no distingue un board de otro guarda los veinte arboles
        // encima del mismo fichero, y eso no se ve hasta el final: quedaria uno.
        if (guardar && boards.size() > 1 &&
            patron.find("{board}") == std::string::npos &&
            patron.find("{n}") == std::string::npos)
            return "{\"ok\":false,\"note\":" +
                   jstr(M("con ese nombre los arboles se guardarian uno encima de otro: "
                          "pon {board} o {n} dentro",
                          "with that name the trees would be saved one on top of the "
                          "other: put {board} or {n} in it")) + "}";

        // La config, guardada: es lo que el script carga en su primera linea.
        std::string e;
        if (!S.save_config(nombre, e))
            return "{\"ok\":false,\"note\":" + jstr(e) + "}";

        std::string s;
        s += "# DCFR Solver " + std::string(cfg::VERSION) + "\n";
        s += M("# Script de varios boards: ", "# Many-board script: ") +
             std::to_string(boards.size()) + M(" boards\n", " boards\n");
        s += "#\n";
        s += M("# Se lanza desde una terminal, en la carpeta del solver:\n",
               "# Run it from a terminal, in the solver's folder:\n");
        s += "#     solver --script " + nombre + ".txt\n";
        s += "#\n";
        s += M("# Cada board: monta el arbol, resuelve, y ",
               "# For each board: build the tree, solve, and ");
        s += guardar ? M("guarda el arbol resuelto.\n", "save the solved tree.\n")
                     : M("deja el resultado sin guardar.\n", "leave it unsaved.\n");
        s += M("# El spot -- rangos, tamaños, bote, stack -- sale de la config '",
               "# The spot -- ranges, sizings, pot, stack -- comes from the config '") +
             nombre + M("',\n# que se acaba de guardar con lo que tenias en la pantalla.\n",
                        "',\n# just saved with whatever you had on screen.\n");
        s += "\n";
        s += "load config " + nombre + "\n";
        char nums[120];
        std::snprintf(nums, sizeof nums, "set accuracy %.4g\n", acc);
        s += nums;
        s += "set stopacc on\n";
        std::snprintf(nums, sizeof nums, "set timeout %.4g\n", tmo);
        s += nums;
        s += "set iters " + std::to_string(iters) + "\n";
        s += "\n";

        for (size_t k = 0; k < boards.size(); ++k) {
            std::string nom = patron;
            for (;;) {
                const size_t p = nom.find("{board}");
                if (p == std::string::npos) break;
                nom.replace(p, 7, boards[k]);
            }
            for (;;) {
                const size_t p = nom.find("{n}");
                if (p == std::string::npos) break;
                nom.replace(p, 3, std::to_string(k + 1));
            }
            // La marca antes de cada board: por la mañana el log son mil lineas
            // y lo primero que se busca es por cual iba y a que hora.
            s += "echo === " + std::to_string(k + 1) + "/" +
                 std::to_string(boards.size()) + "  " + boards[k] + "\n";
            s += "board " + boards[k] + "\n";
            s += "solve\n";
            if (guardar) s += "save tree " + nom + "\n";
            s += "\n";
        }
        s += std::string("echo ") + M("=== terminada la lista de ", "=== finished the list of ") +
             std::to_string(boards.size()) + M(" boards\n", " boards\n");
        return "{\"ok\":true,\"script\":" + jstr(s) + "}";
    }

    std::string api_state() const {
        const Deal& D = S.deal();
        const GameTree& T = S.tree();
        std::string j = "{";
        j += "\"board\":" + jstr(S.board_spec());
        j += ",\"boardCards\":[";
        for (size_t i = 0; i < D.board.size(); ++i) { if (i) j += ","; j += jstr(card_str(D.board[i])); }
        j += "]";
        j += ",\"street\":" + std::to_string(D.start);
        j += ",\"streetName\":" + jstr(STREET_NAME[D.start]);
        j += ",\"combos\":" + std::to_string(D.num());
        j += ",\"runouts\":" + std::to_string(D.num_runouts);
        j += ",\"deck\":[";
        for (size_t i = 0; i < D.deck.size(); ++i) { if (i) j += ","; j += jstr(card_str(D.deck[i])); }
        j += "]";
        j += ",\"pot\":" + jnum(cfg::POT0);
        j += ",\"stack\":" + jnum(cfg::STACK);
        j += ",\"streets\":[";
        for (int s = 0; s < 3; ++s) {
            if (s) j += ",";
            j += "{\"name\":" + jstr(STREET_NAME[s]) +
                 ",\"active\":" + ((s >= D.start) ? "true" : "false") +
                 ",\"oopBets\":"   + jstr(fmt_sizings(S.tc().bets[s][0])) +
                 ",\"oopRaises\":" + jstr(fmt_sizings(S.tc().raises[s][0])) +
                 ",\"oopAllin\":"  + std::string(S.tc().add_allin[s][0] ? "true" : "false") +
                 ",\"ipBets\":"    + jstr(fmt_sizings(S.tc().bets[s][1])) +
                 ",\"ipRaises\":"  + jstr(fmt_sizings(S.tc().raises[s][1])) +
                 ",\"ipAllin\":"   + std::string(S.tc().add_allin[s][1] ? "true" : "false") +
                 ",\"no3bet\":"    + std::string(S.tc().no_3bet[s] ? "true" : "false") +
                 ",\"donks\":"     + jstr(fmt_sizings(S.tc().donks[s])) +
                 ",\"donkable\":"  + std::string((s > D.start) ? "true" : "false") +
                 "}";
        }
        j += "]";
        j += ",\"iters\":" + std::to_string(S.iters());
        j += ",\"accPct\":" + jnum(S.acc_target());
        j += ",\"accStop\":" + std::string(S.acc_stop() ? "true" : "false");
        j += ",\"timeout\":" + jnum(S.timeout_secs());
        j += ",\"threadsCfg\":" + std::to_string(cfg::THREADS);
        j += ",\"threadsMax\":" + std::to_string(
                 static_cast<int>(std::thread::hardware_concurrency()));
        // La version, para que quien reporte algo pueda decir cual tiene.
        j += ",\"version\":" + jstr(cfg::VERSION);
        j += ",\"solved\":" + std::string(S.solved() ? "true" : "false");
        // Hay locks que el proximo solve metera y que lo que se ve todavia no
        // lleva: la estrategia en pantalla es buena, pero es la de ANTES de
        // ellos. Antes esto se decia poniendo `solved` a false, que borraba la
        // pantalla entera y hacia imposible mover dos barras seguidas.
        j += ",\"locksPending\":" +
             std::string(S.locks_pending() ? "true" : "false");
        j += ",\"done\":" + std::to_string(S.solver() ? S.solver()->iterations_done() : 0);
        j += ",\"threads\":" + std::to_string(S.solver() ? S.solver()->threads() : 0);
        {
            const Session::MemUse m = S.memory_use();
            j += ",\"memGB\":" + jnum(m.total) +
                 ",\"memParts\":{\"buffers\":" + jnum(m.buffers) +
                 ",\"stamps\":"  + jnum(m.stamps) +
                 ",\"tables\":"  + jnum(m.tables) +
                 ",\"sweep\":"   + jnum(m.sweep) +
                 ",\"scratch\":" + jnum(m.scratch) + "}";
        }
        j += ",\"expl\":"  + jnum(expl_shown(S.exploitability()));
        j += ",\"explPct\":" + jnum(S.exploitability() >= 0.0
                                     ? 100.0 * expl_shown(S.exploitability()) / cfg::POT0 : -1.0);
        j += ",\"iso\":" + std::string(S.deal().iso_on ? "true" : "false");
        j += ",\"isoOffByLock\":" + std::string(S.iso_off_by_lock() ? "true" : "false");
        // What is actually collapsed, and what the board alone would have
        // allowed. They differ when a range or a lock survives only part of
        // the group, and reporting the board's number then overstates it.
        j += ",\"isoGroup\":" + std::to_string(static_cast<int>(S.deal().use_group.size()));
        j += ",\"isoBoardGroup\":" + std::to_string(static_cast<int>(S.deal().base_group.size()));
        j += ",\"noRaises\":" + std::string(S.no_raises_anywhere() ? "true" : "false");
        j += ",\"instNodes\":" + std::to_string(T.num_instanced_nodes());
        j += ",\"oopSpec\":" + jstr(S.range_spec(0));
        j += ",\"ipSpec\":" + jstr(S.range_spec(1));
        j += ",\"oopCombos\":" + std::to_string(S.live_combos(0));
        j += ",\"ipCombos\":" + std::to_string(S.live_combos(1));
        j += ",\"maxMem\":" + jnum(cfg::MAX_MEM_GB);
        j += ",\"iso\":" + std::string(cfg::ISO ? "true" : "false");
        j += ",\"alpha\":" + jnum(cfg::DCFR_ALPHA);
        j += ",\"beta\":" + jnum(cfg::DCFR_BETA);
        j += ",\"gamma\":" + jnum(cfg::DCFR_GAMMA);
        j += ",\"allinPct\":" + jnum(cfg::ALLIN_THRESH);
        j += ",\"rakePct\":" + jnum(cfg::RAKE_PCT);
        j += ",\"rakeCap\":" + jnum(cfg::RAKE_CAP);
        j += ",\"ready\":" + std::string(S.ranges_ready() ? "true" : "false");
        j += ",\"notReady\":" + jstr(S.ranges_ready() ? "" : S.not_ready_reason());
        j += ",\"oopRange\":" + json_range(0);
        j += ",\"ipRange\":" + json_range(1);
        j += ",\"tree\":" + json_tree();
        j += ",\"locks\":[";
        for (size_t i = 0; i < S.locks().size(); ++i) {
            const LockSpec& L = S.locks()[i];
            if (i) j += ",";
            std::string mix;
            for (size_t k = 0; k < L.mix.size(); ++k) {
                if (k) mix += ",";
                mix += L.mix[k].first + "=" + jnum(L.mix[k].second);
            }
            j += "{\"ctx\":" + jstr(L.ctx_label) + ",\"node\":" + jstr(L.node_path) +
                 ",\"hands\":" + jstr(L.hand_spec) + ",\"mix\":" + jstr(mix) + "}";
        }
        j += "]";
        if (S.solved()) {
            const double ev0 = S.solver()->root_ev(0);
            const double ev1 = (cfg::RAKE_PCT > 0.0) ? S.solver()->root_ev(1)
                                                     : cfg::POT0 - ev0;
            j += ",\"evOOP\":" + jnum(ev0) + ",\"evIP\":" + jnum(ev1);
        }
        return j + "}";
    }

    std::string api_config(const Params& f) {
        std::string e, notes;
        bool okflag = true;
        auto fail = [&](const std::string& m) { okflag = false; notes += m + "; "; };

        if (f.has("board") && !S.set_board(f.get("board"), e)) fail(e);
        if (f.has("oop")   && !S.set_range(0, f.get("oop"), e)) fail("OOP: " + e);
        if (f.has("ip")    && !S.set_range(1, f.get("ip"), e))  fail("IP: " + e);
        if (f.has("pot")   && !S.set_pot(f.getd("pot", cfg::POT0), e)) fail(e);
        if (f.has("stack") && !S.set_stack(f.getd("stack", cfg::STACK), e)) fail(e);
        if (f.has("allinPct")) {
            double ap = cfg::ALLIN_THRESH;
            if (!parse_fraction(f.get("allinPct"), ap))
                fail(M("umbral de all-in: '", "all-in threshold: '") + f.get("allinPct") +
                     M("' no es un número", "' is not a number"));
            else if (!S.set_allin_thresh(ap, e)) fail("all-in threshold: " + e);
        }
        if (f.has("rakePct") || f.has("rakeCap")) {
            double rp = cfg::RAKE_PCT, rc = cfg::RAKE_CAP;
            bool okn = true;
            if (f.has("rakePct") && !parse_fraction(f.get("rakePct"), rp))
                { fail("rake: '" + f.get("rakePct") +
                        M("' no es un número", "' is not a number")); okn = false; }
            if (f.has("rakeCap") && !parse_double(trim(f.get("rakeCap")), rc))
                { fail(M("tope de rake: '", "rake cap: '") + f.get("rakeCap") +
                        M("' no es un número", "' is not a number")); okn = false; }
            if (okn && !S.set_rake(rp, rc, e)) fail(e);
        }

        for (int s = 0; s < 3; ++s) {
            const std::string sn = STREET_NAME[s];
            // Four lists per street now: OOP bets and raises, IP bets and
            // raises. The field name carries the player, so a form that only
            // sends one of them only changes that one.
            for (int q = 0; q < 2; ++q) {
                const std::string who = q ? "Ip" : "Oop";
                for (int pass = 0; pass < 2; ++pass) {
                    const bool bets = (pass == 0);
                    const std::string key = sn + who + (bets ? "Bets" : "Raises");
                    if (!f.has(key)) continue;
                    std::vector<Sizing> sz;
                    const bool leido = bets ? parse_sizings(f.get(key), sz, e)
                                            : parse_raises(f.get(key), sz, e);
                    if (!leido) fail(key + ": " + e);
                    else if (!S.set_sizings_for(bets, q, s, sz, e)) fail(key + ": " + e);
                }
                const std::string ak = sn + who + "Allin";
                if (f.has(ak)) {
                    bool on = false;
                    if (!parse_onoff(f.get(ak), on)) fail(ak + M(": si o no", ": on or off"));
                    else if (!S.set_allin_for(q, s, on, e)) fail(ak + ": " + e);
                }
            }
            if (f.has(sn + "Donks")) {
                std::vector<Sizing> sz;
                if (!parse_sizings(f.get(sn + "Donks"), sz, e)) fail(sn + " donks: " + e);
                else if (!S.set_donks(s, sz, e)) fail(sn + " donks: " + e);
            }
            if (f.has(sn + "No3bet")) {
                bool on = false;
                if (!parse_onoff(f.get(sn + "No3bet"), on)) fail(sn + M(" no3bet: si o no", " no3bet: on or off"));
                else if (!S.set_no3bet(s, on, e)) fail(e);
            }
        }
        if (f.has("iters")) S.set_iters(f.geti("iters", S.iters()));
        if (f.has("accPct"))  S.set_acc_target(f.getd("accPct", S.acc_target()));
        if (f.has("timeout")) {
            double v;
            if (!parse_double(trim(f.get("timeout")), v) || v < 0.0)
                fail(M("el tope de tiempo es un numero de segundos, 0 = sin tope",
                       "the time limit is a number of seconds, 0 = no limit"));
            else S.set_timeout_secs(v);
        }
        if (f.has("accStop")) {
            bool on = false;
            if (!parse_onoff(f.get("accStop"), on)) fail(M("accStop: si o no", "accStop: on or off"));
            else S.set_acc_stop(on);
        }
        // Solo si CAMBIA. Poner los hilos que ya estaban puestos tiraba la
        // solucion, y el panel del engranaje manda este campo entero cada vez que
        // alguien da a Aplicar, aunque solo haya tocado el limite de memoria.
        if (f.has("threads")) {
            const int t = f.geti("threads", 0);
            if (t >= 0 && t != cfg::THREADS) { cfg::THREADS = t; S.invalidate(); }
        }
        // The memory ceiling has to be reachable from here: when a tree is
        // refused, the message that comes back names this setting, and telling
        // someone in a browser to go and type a console command is a dead end.
        if (f.has("maxmem")) {
            double v;
            if (!parse_double(trim(f.get("maxmem")), v) || v <= 0.0)
                fail(M("el límite de memoria es un número de GB mayor que cero",
                       "the memory limit must be a number of GB above zero"));
            else cfg::MAX_MEM_GB = v;
        }
        // Explicitly parsed, nunca por defecto: un valor que no se entiende se
        // dice, porque darlo por "no" apaga el agrupado de palos y eso es el doble
        // de memoria y de tiempo sin que nadie lo haya pedido.
        if (f.has("iso")) {
            bool on = false;
            if (!parse_onoff(f.get("iso"), on)) fail(M("iso: si o no", "iso: on or off"));
            else if (on != cfg::ISO) { cfg::ISO = on; if (!S.rebuild(e)) fail(e); }
        }
        {
            const char* keys[3] = { "alpha", "beta", "gamma" };
            double* dst[3] = { &cfg::DCFR_ALPHA, &cfg::DCFR_BETA, &cfg::DCFR_GAMMA };
            for (int k = 0; k < 3; ++k) {
                if (!f.has(keys[k])) continue;
                double v;
                if (!parse_double(trim(f.get(keys[k])), v))
                    fail(std::string(keys[k]) + M(": no es un número", ": not a number"));
                else if (*dst[k] != v) { *dst[k] = v; S.invalidate(); }
            }
        }
        return std::string("{\"ok\":") + (okflag ? "true" : "false") +
               ",\"note\":" + jstr(notes) + ",\"state\":" + api_state() + "}";
    }

    // Starts the solve and returns at once; the page then polls /api/progress.
    std::string api_solve(const Params& f) {
        const int n = f.geti("iters", S.iters());
        S.set_iters(n);
        if (!S.ranges_ready())
            return "{\"ok\":false,\"note\":" + jstr(S.not_ready_reason()) + "}";
        if (f.geti("more", 0)) {
            if (!S.solved())
                return "{\"ok\":false,\"note\":\"" + std::string(M("no hay ningún solve al que añadir", "there is no solve to add to")) + "\"}";
            if (!S.solve_more_async(n))
                return "{\"ok\":false,\"note\":\"" + std::string(M("ya hay un solve en marcha", "a solve is already running")) + "\"}";
            return "{\"ok\":true,\"started\":true,\"more\":true}";
        }
        if (!S.solve_async(n))
            return "{\"ok\":false,\"note\":\"" + std::string(M("ya hay un solve en marcha", "a solve is already running")) + "\"}";
        return "{\"ok\":true,\"started\":true}";
    }

    std::string api_saves() {
        std::string j = "{\"ok\":true";
        // Lo que ocupa todo junto. Cada fichero ya lleva su tamano, pero nadie
        // suma doce numeros al vuelo: el que interesa es el total.
        long long total = 0;
        for (int k = 0; k < 2; ++k) {
            const bool tree = (k == 1);
            j += std::string(",\"") + (tree ? "trees" : "configs") + "\":[";
            const std::vector<std::string> v = S.list_saves(tree);
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) j += ",";
                const long long b = S.save_size(tree, v[i]);
                total += b;
                j += "{\"name\":" + jstr(v[i]) +
                     ",\"bytes\":" + std::to_string(b) + "}";
            }
            j += "]";
        }
        j += ",\"ranges\":[";
        {
            const std::vector<std::string> v = S.list_ranges();
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) j += ",";
                j += "{\"name\":" + jstr(v[i]) +
                     ",\"bytes\":" + std::to_string(S.range_size(v[i])) + "}";
            }
        }
        j += "]";
        j += ",\"savesBytes\":" + std::to_string(total);
        j += ",\"freeBytes\":" + std::to_string(Session::free_bytes_for_saves());
        j += ",\"nextTreeBytes\":" + std::to_string(S.tree_file_bytes());
        return j + "}";
    }

    // Saving a solved flop writes hundreds of megabytes, so this holds the API
    // mutex for a while. That is deliberate: a half-written file is worse than
    // a slow one, and the endpoint is already refused while a solve is running.
    std::string api_store(const Params& f) {
        const std::string verb = lower(f.get("verb"));
        const std::string kind = lower(f.get("kind"));
        const std::string name = trim(f.get("name"));
        if (kind != "config" && kind != "tree" && kind != "range")
            return "{\"ok\":false,\"note\":\"" + std::string(M("el tipo es config, tree o range", "kind must be config, tree or range")) + "\"}";
        const bool tree = (kind == "tree");
        std::string e;
        bool ok;

        // Un rango guardado son los dos, OOP e IP: es un spot concreto y un lado
        // suelto no dice nada. `player` solo se usa al cargar un fichero de los
        // de antes, que llevaban uno solo.
        if (kind == "range") {
            const int player = (lower(f.get("player")) == "ip") ? 1 : 0;
            if (verb == "save")        ok = S.save_range(name, e);
            else if (verb == "load")   ok = S.load_range(name, player, e);
            else if (verb == "delete") ok = S.delete_range(name, e);
            else return "{\"ok\":false,\"note\":\"" + std::string(M("la acción es save, load o delete", "verb must be save, load or delete")) + "\"}";
            if (!ok) return "{\"ok\":false,\"note\":" + jstr(e) + "}";
            return "{\"ok\":true,\"note\":" +
                   jstr(verb + " range '" + name + "'") +
                   ",\"state\":" + api_state() + "}";
        }
        if (verb == "save") {
            SessionRead rd(S, false);
            ok = tree ? S.save_tree(name, e) : S.save_config(name, e);
        } else if (verb == "load") {
            SessionRead rd(S, false);
            ok = tree ? S.load_tree(name, e) : S.load_config(name, e);
        } else if (verb == "delete") {
            ok = S.delete_save(tree, name, e);
        } else {
            return "{\"ok\":false,\"note\":\"" + std::string(M("la acción es save, load o delete", "verb must be save, load or delete")) + "\"}";
        }
        if (!ok) return "{\"ok\":false,\"note\":" + jstr(e) + "}";
        std::string note = verb + " " + kind + " '" + name + "'";
        if (verb == "load") {
            if (!S.locks().empty())
                note += ", " + std::to_string(S.locks().size()) + " nodelock(s)";
            if (S.dropped_locks())
                note += ", " + std::to_string(S.dropped_locks()) +
                        M(" nodelock(s) que ya no caben en este árbol",
                          " nodelock(s) no longer fit this tree");
        }
        if (verb == "save") {
            const long long b = S.save_size(tree, name);
            if (b >= 0) {
                char sz[48];
                if (b < 1024)             std::snprintf(sz, sizeof sz, "%lld B", b);
                else if (b < 1048576)     std::snprintf(sz, sizeof sz, "%.1f KB", b / 1024.0);
                else if (b < 1073741824)  std::snprintf(sz, sizeof sz, "%.1f MB", b / 1048576.0);
                else                      std::snprintf(sz, sizeof sz, "%.2f GB", b / 1073741824.0);
                note += std::string("  (") + sz + ")";
            }
        }
        return "{\"ok\":true,\"note\":" + jstr(note) + "}";
    }

    // `with_state` decide si la respuesta lleva el estado entero dentro.

    //

    // Tiene que decidirlo QUIEN LLAMA, no esta funcion, porque construir el

    // estado recorre el arbol por root_ev() sobre el unico scratch principal

    // que tambien usa el solver: hacerlo sin el mutex deja a dos recorridos

    // compartiendo los mismos buffers. Esto se llamaba a si mismo con el estado

    // dentro sin coger nada, y decia en su propio comentario que solo tocaba

    // atomicos. Tres peticiones abandonadas a medias bastaban para dejar el

    // servidor muerto: /api/progress y /api/state sin responder y todo lo demas

    // contestando al instante, que fue justo la pista que lo delato.

    std::string api_progress(bool with_state) {
        const bool run = S.busy();
        std::string j = "{\"ok\":true,\"running\":";
        j += run ? "true" : "false";
        j += ",\"done\":"    + std::to_string(S.progress_done());
        j += ",\"total\":"   + std::to_string(S.progress_total());
        j += ",\"seconds\":" + jnum(S.progress_seconds());
        j += ",\"stopped\":" + std::string(S.was_stopped() ? "true" : "false");
        j += ",\"accHit\":" + std::string(S.acc_reached() ? "true" : "false");
        j += ",\"expl\":"    + jnum(expl_shown(S.exploitability()));
        j += ",\"explPct\":" + jnum(S.exploitability() >= 0.0
                                     ? 100.0 * expl_shown(S.exploitability()) / cfg::POT0 : -1.0);
        if (!run) {
            std::string note;
            for (const std::string& m : S.async_notes()) note += m + "; ";
            j += ",\"note\":" + jstr(note);
            if (with_state) j += ",\"state\":" + api_state();
        }
        return j + "}";
    }

    // The whole next street at once: one row per card that can still come.
    std::string api_runouts(const Params& q) {
        if (!S.solved()) return "{\"ok\":false,\"note\":\"" + std::string(M("sin resolver", "not solved")) + "\"}";
        const GameTree& T = S.tree();
        const int ci  = q.geti("ctx", 0);
        const int nid = q.geti("node", -1);
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size()))
            return "{\"ok\":false,\"note\":\"" + std::string(M("contexto que no existe", "bad context")) + "\"}";
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size()))
            return "{\"ok\":false,\"note\":\"" + std::string(M("nodo que no existe", "bad node")) + "\"}";

        const std::vector<int> slots = parse_slots(q.get("slots"));
        const RunoutTable t = aggregate_runouts(*S.solver(), ci, nid, slots);
        if (!t.ok) return "{\"ok\":false,\"note\":" + jstr(t.note) + "}";

        // The answer says which node it is about, so the client can refuse to
        // render it into a table for a different one. /api/node has always
        // said; this one did not, which made the same guard unwritable here.
        std::string j = "{\"ok\":true,\"ctx\":" + std::to_string(ci) +
                        ",\"node\":" + std::to_string(nid) +
                        ",\"street\":" + jstr(t.street) +
                        ",\"player\":" + std::to_string(t.player) +
                        ",\"avgEvOOP\":" + jnum(t.avg_ev_oop) +
                        ",\"avgEvIP\":" + jnum(t.avg_ev_ip) +
                        ",\"actions\":[";
        for (size_t a = 0; a < t.codes.size(); ++a) {
            if (a) j += ",";
            j += "{\"code\":" + jstr(t.codes[a]) + ",\"label\":" + jstr(t.labels[a]) +
                 ",\"kind\":" + std::to_string(t.kinds[a]) +
                 ",\"avg\":" + jnum(100.0 * t.avg_freq[a]) + "}";
        }
        j += "],\"rows\":[";
        bool first = true;
        for (const RunoutRow& r : t.rows) {
            if (!r.ok) continue;
            if (!first) j += ",";
            first = false;
            j += "{\"slot\":" + std::to_string(r.slot) +
                 ",\"card\":" + jstr(r.name) +
                 ",\"pot\":" + jnum(r.pot) +
                 ",\"reach\":" + jnum(r.reach) +
                 ",\"evOOP\":" + jnum(r.ev_oop) +
                 ",\"evIP\":" + jnum(r.ev_ip) +
                 ",\"freq\":[";
            for (size_t a = 0; a < r.freq.size(); ++a) {
                if (a) j += ",";
                j += jnum(100.0 * r.freq[a]);
            }
            j += "]}";
        }
        return j + "]}";
    }

    std::string api_node(const Params& q) {
        if (!S.solved()) return "{\"ok\":false,\"note\":\"" + std::string(M("sin resolver", "not solved")) + "\"}";
        const GameTree& T = S.tree();
        const int ci  = q.geti("ctx", 0);
        const int nid = q.geti("node", -1);
        if (ci < 0 || ci >= static_cast<int>(T.ctx.size())) return "{\"ok\":false,\"note\":\"" + std::string(M("contexto que no existe", "bad context")) + "\"}";
        const RoundCtx& rc = T.ctx[static_cast<size_t>(ci)];
        const int node = (nid >= 0) ? nid : rc.tree.root;
        if (node < 0 || node >= static_cast<int>(rc.tree.nodes.size()))
            return "{\"ok\":false,\"note\":\"" + std::string(M("nodo que no existe", "bad node")) + "\"}";
        if (rc.tree.nodes[static_cast<size_t>(node)].type != NT_DECISION)
            return "{\"ok\":false,\"note\":\"" + std::string(M("ese nodo no es una decisión", "not a decision node")) + "\"}";

        const std::vector<int> slots = parse_slots(q.get("slots"));
        long long inst = 0;
        int perm = S.deal().identity();
        if (!S.deal().locate(slots, inst, perm) || inst < 0 || inst >= rc.instances)
            return "{\"ok\":false,\"note\":\"" + std::string(M("reparto de cartas que no existe", "bad runout")) + "\"}";

        DCFRSolver& sol = *S.solver();
        // Con equity: esta es la vista de combos, y ahi la columna tiene que
        // ser la de este nodo.
        const NodeStats N = gather(sol, ci, node, inst, perm, true);
        if (!N.ok) return "{\"ok\":false,\"note\":\"" + std::string(M("no se pudo evaluar", "could not evaluate")) + "\"}";
        const Deal& D = S.deal();

        // El board de ESTE nodo, no el de partida. Lo que tiene cada uno hecho
        // cambia con cada carta que sale, y la web navega por parametros, asi
        // que se arma de los slots en vez de leer el cursor de la sesion.
        const std::vector<int> board_aqui = S.board_of(ci, slots);

        // La mejor respuesta solo si se pide: recorre el arbol entero, o sea
        // cuesta como una iteracion, y navegar nodos no tiene por que pagarlo.
        std::vector<double> brv;
        int brA = 0;
        const bool quiere_br = q.get("br") == "1";
        if (quiere_br && !sol.br_at(ci, node, inst, brv, brA)) brA = 0;

        double evOOP = 0.0, evIP = 0.0;
        node_ev_both(sol, ci, node, inst, N.player, N.node_ev(), evOOP, evIP);
        std::string j = "{\"ok\":true,\"ctx\":" + std::to_string(ci) +
                        ",\"node\":" + std::to_string(node) +
                        ",\"ctxLabel\":" + jstr(N.label) +
                        ",\"path\":" + jstr(N.path) +
                        ",\"street\":" + std::to_string(rc.street) +
                        ",\"player\":" + std::to_string(N.player) +
                        ",\"pot\":" + jnum(N.pot) +
                        ",\"ev\":" + jnum(N.node_ev()) +
                        ",\"evOOP\":" + jnum(evOOP) +
                        ",\"evIP\":"  + jnum(evIP) +
                        ",\"reach\":" + jnum(node_reach_pct(sol, N)) +
                        ",\"locked\":" + (N.locked ? "true" : "false") +
                        ",\"instances\":" + std::to_string(rc.instances) +
                        ",\"actions\":[";
        // What this player does here on THIS runout, and what they do here
        // across every runout that reaches this decision point. On a turn node
        // the first answers "on the Ts", the second answers "on a turn" -- two
        // different questions, and reading only the first is how you end up
        // learning a card instead of a spot. Same combo weights for both, so
        // the only thing that changes is which strategy is being weighted.
        std::vector<double> blk;
        const bool have_blk = (rc.instances > 1);
        if (have_blk) {
            blk.assign(static_cast<size_t>(N.A) * N.nh, 0.0);
            sol.avg_strategy_block(ci, node, blk.data());
        }
        // Los combos que llegan aqui: el alcance propio sumado, sin multiplicar
        // por el del rival. En la raiz es el tamano del rango y mas abajo, lo
        // que queda de el despues de la linea.
        const double combos_aqui = N.combos();
        for (int a = 0; a < N.A; ++a) {
            if (a) j += ",";
            j += "{\"code\":" + jstr(N.codes[static_cast<size_t>(a)]) +
                 ",\"label\":" + jstr(N.labels[static_cast<size_t>(a)]) +
                 ",\"kind\":" + std::to_string(static_cast<int>(rc.tree.act(rc.tree.nodes[static_cast<size_t>(node)], a).kind)) +
                 ",\"freq\":" + jnum(100.0 * N.node_freq(a)) +
                 // Cuantos COMBOS hay detras de ese porcentaje. Una frecuencia
                 // sola no dice si es medio rango o dos manos, y eso es lo
                 // primero que quieres saber antes de mover nada.
                 //
                 // Aqui ponia N.wtot, que es masa de PAREJAS (mi alcance por el
                 // del rival) y no combos: en un rango de 58 combos salia 4096.
                 // El numero que se lee como combos tiene que ser combos, o al
                 // lado del de la referencia no se parece a nada.
                 ",\"combos\":" + jnum(N.node_freq(a) * combos_aqui);
            if (have_blk) {
                double num = 0.0, den = 0.0;
                for (int h = 0; h < N.nh; ++h) {
                    const double w = N.weight[static_cast<size_t>(h)];
                    if (w <= 1e-12) continue;
                    den += w;
                    num += w * blk[static_cast<size_t>(a) * N.nh + h];
                }
                j += ",\"freqAll\":" + jnum(den > 1e-12 ? 100.0 * num / den : 0.0);
            }
            j += "}";
        }
        j += "],\"nodeCombos\":" + jnum(combos_aqui);

        // 13x13 aggregation. This used to be its own loop -- a third copy of
        // the same weighted average, alongside the one in the text report and
        // the one behind the CSV. Three copies of an average is three chances
        // for the grid to say something the hover card does not, so they are
        // one function now, and the checks that hold aggregate_by_class to
        // being an average hold this too.
        {
            std::vector<const ClassAgg*> at(169, nullptr);
            const std::vector<ClassAgg> agg = aggregate_by_class(sol, N);
            for (const ClassAgg& g : agg) at[static_cast<size_t>(g.cls)] = &g;
            j += ",\"grid\":[";
            for (int c = 0; c < 169; ++c) {
                if (c) j += ",";
                const ClassAgg* g = at[static_cast<size_t>(c)];
                if (!g) { j += "null"; continue; }
                j += "{\"w\":" + jnum(N.wtot > 1e-12 ? 100.0 * g->w / N.wtot : 0.0) +
                     ",\"eq\":" + jnum(g->eq) +
                     ",\"ev\":" + jnum(g->node_ev) +
                     ",\"lk\":" + std::to_string(g->locked ? 1 : 0) + ",\"f\":[";
                for (int a = 0; a < N.A; ++a) {
                    if (a) j += ",";
                    j += jnum(100.0 * g->freq[static_cast<size_t>(a)]);
                }
                j += "],\"e\":[";
                for (int a = 0; a < N.A; ++a) {
                    if (a) j += ",";
                    j += jnum(g->ev[static_cast<size_t>(a)]);
                }
                j += "]}";
            }
            j += "]";
        }

        // Y por mano hecha, que es la otra forma de mirar el mismo rango: la
        // rejilla dice que cartas tienes, esto dice que tienes hecho. Cuesta
        // un recorrido de los combos, o sea nada al lado de leer el nodo.
        {
            const std::vector<ClassAgg> hechas = aggregate_by_made(sol, N, board_aqui);
            j += ",\"made\":[";
            for (size_t i = 0; i < hechas.size(); ++i) {
                const ClassAgg& g = hechas[i];
                if (i) j += ",";
                j += "{\"name\":" + jstr(agg_name(g)) +
                     ",\"kind\":" + std::to_string(g.kind) +
                     ",\"cls\":" + std::to_string(g.cls) +
                     ",\"w\":" + jnum(N.wtot > 1e-12 ? 100.0 * g.w / N.wtot : 0.0) +
                     // El TROZO del rango que es esta categoria, en combos.
                     // Antes iba g.w, que es masa de parejas: numeros de cinco
                     // cifras en un rango de setecientos combos.
                     ",\"combos\":" +
                         jnum(N.wtot > 1e-12 ? g.w / N.wtot * combos_aqui : 0.0) +
                     ",\"eq\":" + jnum(g.eq) +
                     ",\"ev\":" + jnum(g.node_ev) + ",\"f\":[";
                for (int a = 0; a < N.A; ++a) {
                    if (a) j += ",";
                    j += jnum(100.0 * g.freq[static_cast<size_t>(a)]);
                }
                j += "]}";
            }
            j += "]";
        }


        // Every live combo, tagged with the class it belongs to. It used to
        // send one class at a time, on demand, which is fine for a table you
        // open by clicking and useless for a card that follows the mouse: a
        // request per cell leaves the card a beat behind the pointer. Two
        // hundred combos of a dozen numbers is a few tens of kilobytes --
        // cheaper than the round trip it saves.
        {
            const int want = q.has("cls") ? q.geti("cls", -1) : -1;
            std::vector<int> bd = D.board;
            for (int s : slots) bd.push_back(D.deck[static_cast<size_t>(s)]);
            j += ",\"combos\":[";
            bool first = true;
            for (int h = 0; h < N.nh; ++h) {
                if (want >= 0 && D.combos[static_cast<size_t>(h)].cls != want) continue;
                if (N.weight[static_cast<size_t>(h)] <= 1e-12) continue;
                const Combo& k = D.combos[static_cast<size_t>(h)];
                if (!first) j += ",";
                first = false;
                // En el river, la jugada exacta ("two pair, nines and sevens");
                // antes de eso, la categoria. Aqui ponia "-" en flop y turn, que
                // es justo donde mas se mira esta columna.
                const int mi = made_cat(k.c1, k.c2, bd);
                const int di = draw_cat(k.c1, k.c2, bd);
                std::string made;
                if (bd.size() == 5) {
                    int seven[7] = { k.c1, k.c2, bd[0], bd[1], bd[2], bd[3], bd[4] };
                    made = score_str(eval7(seven));
                } else {
                    made = made_draw_name(mi, di);
                }
                j += "{\"name\":" + jstr(card_str(k.c1) + card_str(k.c2)) +
                     ",\"cls\":" + std::to_string(k.cls) +
                     ",\"made\":" + jstr(made) +
                     ",\"mi\":" + std::to_string(mi) +
                     ",\"di\":" + std::to_string(di) +
                     ",\"eq\":" + jnum(N.equity(sol, h)) +
                     ",\"w\":" + jnum(N.weight[static_cast<size_t>(h)]) +
                     ",\"lk\":" + std::to_string(sol.is_hand_locked(ci, node, N.inst, N.stored(h)) ? 1 : 0) +
                     ",\"ev\":" + jnum(N.ev_node(h)) + brjson(N, brv, brA, h) + ",\"f\":[";
                for (int a = 0; a < N.A; ++a) { if (a) j += ","; j += jnum(100.0 * N.freq(h, a)); }
                j += "],\"e\":[";
                for (int a = 0; a < N.A; ++a) { if (a) j += ","; j += jnum(N.ev_action(h, a)); }
                j += "]}";
            }
            j += "]";
        }
        return j + "}";
    }

    std::string api_lock(const Params& f) {
        // A whole dialog's worth of edits in one request: combo:mix;combo:mix.
        // One request per combo would be a hundred round trips for one drag of
        // a slider, and half-applied locks if any of them failed.
        if (f.has("bulk")) {
            const int ci = f.geti("ctx", -1), nd = f.geti("node", -1);
            int done = 0;
            std::string e;
            for (const std::string& one : split(f.get("bulk"), ';')) {
                const size_t colon = one.find(':');
                if (colon == std::string::npos) continue;
                const std::string who = trim(one.substr(0, colon));
                // El token va tal cual: puede ser un codigo ("B33") o un tipo
                // ("B"), y quien lo resuelve es el arbol al aplicarlo.
                std::vector<std::pair<std::string, double>> mix;
                for (const std::string& item : split(one.substr(colon + 1), ',')) {
                    const size_t eq = item.find('=');
                    if (eq == std::string::npos) continue;
                    double pr;
                    const std::string tok = trim(item.substr(0, eq));
                    if (tok.empty()) continue;
                    if (!parse_double(item.substr(eq + 1), pr)) continue;
                    if (pr > 1e-9) mix.push_back(std::make_pair(tok, pr));
                }
                if (mix.empty() || who.empty()) continue;
                int m = 0;
                if (!S.add_lock(ci, nd, who, mix, m, e))
                    return std::string("{\"ok\":false,\"note\":") + jstr(who + ": " + e) +
                           ",\"state\":" + api_state() + "}";
                ++done;
            }
            return std::string("{\"ok\":") + (done ? "true" : "false") +
                   ",\"note\":" + jstr(done ? (std::to_string(done) + " combo(s) locked")
                                              : "nothing to lock") +
                   ",\"state\":" + api_state() + "}";
        }
        // Two ways in besides typing a mix, and they are the ones that get used:
        // freeze what the solve produced, or move one action off it. Both keep
        // every combo's own numbers instead of averaging the class flat.
        const std::string how = lower(trim(f.get("how")));
        if (how == "current" || how == "nudge") {
            int matched = 0;
            std::string e;
            bool okflag;
            // Las ranuras vienen en la peticion tambien por aqui. Sin ellas el
            // servidor usaba su propia navegacion, que no es la del navegador:
            // congelar o mover una familia en el river del 2s podia acabar
            // escrito en otro runout.
            const std::vector<int> slots = parse_slots(f.get("slots"));
            if (how == "current") {
                okflag = S.add_lock_from_current_at(f.geti("ctx", -1), f.geti("node", -1),
                                                    slots, f.get("hands"), matched, e);
            } else {
                ActionKind k;
                double w = 0.0;
                if (!parse_action_kind(f.get("act"), k))
                    return std::string("{\"ok\":false,\"note\":") +
                           jstr(M("no existe la acción '", "unknown action '") + f.get("act") + "'") +
                           ",\"state\":" + api_state() + "}";
                if (!parse_double(trim(f.get("w")), w))
                    return std::string("{\"ok\":false,\"note\":") +
                           jstr("'" + f.get("w") + M("' no es un número", "' is not a number")) +
                           ",\"state\":" + api_state() + "}";
                const std::string md = lower(trim(f.get("mode")));
                const LockMode mode = (md == "fixed") ? LM_FIXED
                                    : (md == "scale") ? LM_SCALE : LM_ADD;
                // `acti` es la accion exacta cuando la manda quien la sabe -- la
                // pagina la tiene en la mano --, y sin ella se busca por tipo
                // como siempre.
                okflag = S.add_lock_moved_at(f.geti("ctx", -1), f.geti("node", -1),
                                             slots, f.get("hands"), k,
                                             f.geti("acti", -1), mode, w,
                                             matched, e);
            }
            return std::string("{\"ok\":") + (okflag ? "true" : "false") +
                   ",\"note\":" + jstr(okflag
                       ? (std::to_string(matched) + (how == "current"
                            ? " combo(s) frozen at what they were doing"
                            : " combo(s) moved")) : e) +
                   ",\"state\":" + api_state() + "}";
        }

        std::vector<std::pair<std::string, double>> mix;
        for (const std::string& item : split(f.get("mix"), ',')) {
            if (item.empty()) continue;
            const size_t eq = item.find('=');
            if (eq == std::string::npos) continue;
            double p;
            const std::string tok = trim(item.substr(0, eq));
            if (tok.empty()) continue;
            if (!parse_double(item.substr(eq + 1), p)) continue;
            mix.push_back(std::make_pair(tok, p));
        }
        int matched = 0;
        std::string e;
        // Las ranuras vienen en la peticion, como todo lo demas: el servidor no
        // guarda por donde anda el navegador.
        const bool okflag = S.add_lock_at(f.geti("ctx", -1), f.geti("node", -1),
                                          parse_slots(f.get("slots")),
                                          f.get("hands"), mix, matched, e);
        return std::string("{\"ok\":") + (okflag ? "true" : "false") +
               ",\"note\":" + jstr(okflag ? (std::to_string(matched) + " combos locked here") : e) +
               ",\"state\":" + api_state() + "}";
    }

    // Lo que gana en esta mano quien juega para explotar la solucion, y con que
    // accion. Vacio si no se pidio.
    static std::string brjson(const NodeStats& N, const std::vector<double>& brv,
                              int brA, int h) {
        if (brA <= 0 || brv.empty()) return "";
        const double c = N.v.compat[static_cast<size_t>(h)];
        if (c <= 1e-12) return "";
        const int st = N.stored(h);
        const int nh = N.nh;
        int mejor = 0;
        for (int a = 1; a < brA; ++a)
            if (brv[static_cast<size_t>(a) * nh + st] >
                brv[static_cast<size_t>(mejor) * nh + st]) mejor = a;
        const double ev = brv[static_cast<size_t>(mejor) * nh + st] / c + cfg::POT0 * 0.5;
        return ",\"br\":" + std::to_string(mejor) +
               ",\"brg\":" + jnum(ev - N.ev_node(h));
    }

    std::string api_unlock(const Params& f) {
        if (f.geti("all", 0)) {
            const int n = S.clear_all_locks();
            return std::string("{\"ok\":") + (n ? "true" : "false") +
                   ",\"note\":" + jstr(n ? (std::to_string(n) + " lock(s) removed")
                                           : "there were no locks") +
                   ",\"state\":" + api_state() + "}";
        }
        // Con `hands`, solo esa familia: es el boton de deshacer de cada barra.
        // Sin el, todo el nodo, que es lo que hacia el unico camino que habia.
        if (!trim(f.get("hands")).empty()) {
            int quitados = 0;
            const bool uno = S.remove_locks_for(f.geti("ctx", -1), f.geti("node", -1),
                                                parse_slots(f.get("slots")),
                                                f.get("hands"), quitados);
            return std::string("{\"ok\":") + (uno ? "true" : "false") +
                   ",\"note\":" + jstr(uno ? (std::to_string(quitados) +
                                              " combo(s) sueltos")
                                            : "ahi no habia nada puesto") +
                   ",\"state\":" + api_state() + "}";
        }
        const bool okflag = S.remove_lock_at(f.geti("ctx", -1), f.geti("node", -1),
                                             parse_slots(f.get("slots")));
        return std::string("{\"ok\":") + (okflag ? "true" : "false") +
               ",\"note\":" + jstr(okflag ? "lock removed" : "no lock there") +
               ",\"state\":" + api_state() + "}";
    }
};
