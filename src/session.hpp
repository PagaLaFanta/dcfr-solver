#pragma once
#include <set>
// =============================================================================
//  Session: the whole mutable state and every operation on it.
//  Both front ends drive this and nothing else, so they cannot drift apart.
//
//  A position in a multi-street tree is three things: which context (the line of
//  continuations), which node inside that round, and which runout got there.
//  The runout is kept as the list of dealt deck slots, from which the memory
//  instance index follows directly.
// =============================================================================

#include "default_ranges.hpp"
#include "msg.hpp"
#include "report.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
//  Where the executable lives.
//
//  WIN32_LEAN_AND_MEAN matters here: this header is pulled in before
//  <winsock2.h>, and a plain <windows.h> drags in the 1.1 winsock that then
//  collides with it. The lean variant leaves sockets out entirely. NOMINMAX
//  keeps the min/max macros from shadowing std::min and std::max.
// -----------------------------------------------------------------------------
#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

inline std::string exe_dir() {
    static const std::string cached = [] {
        std::string p;
#if defined(_WIN32)
        char buf[4096];
        const unsigned long n = GetModuleFileNameA(nullptr, buf, sizeof buf);
        if (n > 0 && n < sizeof buf) p.assign(buf, n);
#else
        std::error_code ec;
        const auto link = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec) p = link.string();
#endif
        if (p.empty()) return std::string(".");
        const size_t slash = p.find_last_of("/\\");
        return (slash == std::string::npos) ? std::string(".") : p.substr(0, slash);
    }();
    return cached;
}

// -----------------------------------------------------------------------------
//  string helpers, shared by both front ends
// -----------------------------------------------------------------------------
inline std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
inline std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}
inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(trim(cur)); cur.clear(); }
        else cur += c;
    }
    out.push_back(trim(cur));
    return out;
}
inline std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream is(s);
    std::string t;
    while (is >> t) out.push_back(t);
    return out;
}
inline bool parse_double(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        const double v = std::stod(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) { return false; }
}
// Los tamanos se escriben en PORCENTAJE del bote -- "50" es medio bote, "300"
// son tres botes -- o en multiplos de la apuesta que tienes delante -- "3x" --.
// Las dos formas se mezclan en una lista.
//
// Antes un numero desnudo era la FRACCION: "0.5" era medio bote y "3" eran tres
// botes. Es la escala de dentro (un tamano se guarda como fraccion y asi sigue)
// pero no es la que se escribe en ningun sitio: ni en la referencia, ni en una mesa,
// ni cuando uno habla. Escribir 30 y que salga un 30% es lo que espera
// cualquiera, y 0.3 para eso mismo era una traduccion que solo hacia falta aqui.
//
// `bare_is_pct` es false cuando lo que se lee es un fichero de configuracion
// guardado por una version anterior, donde el numero desnudo era la fraccion.
// Lo que guarda esta version lleva SIEMPRE su unidad (`30%`, `3x`), asi que se
// lee igual con cualquiera de las dos reglas y esto deja de importar.
inline bool parse_sizings_sep(const std::string& in, bool comma_separates,
                              bool bare_is_pct,
                              std::vector<Sizing>& out, std::string& err) {
    out.clear();
    if (lower(trim(in)) == "none" || trim(in) == "-") return true;

    std::string norm = in;
    for (char& c : norm)
        if (c == ';' || c == ' ' || c == '\t' || c == '\n' || c == '\r') c = ',';
        else if (c == ',' && !comma_separates) c = '.';

    for (const std::string& t0 : split(norm, ',')) {
        const std::string t = trim(t0);
        if (t.empty()) continue;
        Sizing z;
        std::string num = t;

        if (lower(num) == "pot" || lower(num) == "p") { z.v = 1.0; out.push_back(z); continue; }
        // La subida minima legal. Es "1x" -- subir hasta una vez lo que hay
        // puesto, que el constructor sube al minimo porque por debajo no baja --
        // pero "1x" leido literalmente suena a pagar, no a subir. Con nombre se
        // entiende, y es lo que hace falta para reproducir el arbol de fuera: alli
        // se escribe un porcentaje diminuto y significa esto.
        if (lower(num) == "min" || lower(num) == "minimum") {
            z.v = 1.0; z.xbet = true; z.minraise = true; out.push_back(z); continue;
        }

        if (num.size() > 1 && (num.back() == 'x' || num.back() == 'X')) {
            z.xbet = true;
            num.pop_back();
        }
        bool pct = false;
        if (!num.empty() && num.back() == '%') { pct = true; num.pop_back(); }

        for (size_t k = 0; k < num.size(); ++k) {
            const char c = num[k];
            const bool ok = (c >= '0' && c <= '9') || c == '.' ||
                            ((c == '+' || c == '-') && k == 0);
            if (!ok) { err = "bad sizing '" + t + "'"; return false; }
        }
        if (!parse_double(num, z.v)) { err = "bad sizing '" + t + "'"; return false; }
        if (pct || (bare_is_pct && !z.xbet)) z.v /= 100.0;
        if (z.v <= 0.0) {
            err = "'" + t + "' is not a size: a size is a positive percentage of the pot";
            return false;
        }

        if (!z.xbet) {
            // Por debajo del 5% del bote no hay tamano que exista: la apuesta
            // minima ya es mayor. Un numero ahi dentro es casi siempre la escala
            // vieja -- "0.66" queriendo decir 66, "3" queriendo decir 300 -- y
            // callarselo seria construir un arbol que no es el que se pidio.
            // Lo que se rechaza es el DECIMAL pequeno, no el entero pequeno.
            //
            // "0.66" y "2.5" solo se escriben si uno cree que sigue en la escala
            // vieja, y leerlos como un 0,66% callando seria construir otro arbol.
            // Pero un entero pequeno es legitimo y hay que aceptarlo: la referencia
            // escribe "2,5x" y eso son DOS tamanos, un 2% -- que la apuesta
            // minima sube -- y un 5x. Si el 2 se rechazara, el rescate de la coma
            // decimal de mas abajo entraria y lo leeria como 2.5x, que es
            // exactamente el error que ya costo tres dias una vez.
            if (bare_is_pct && !pct && num.find('.') != std::string::npos &&
                100.0 * z.v < 5.0) {
                char nb[32];
                std::snprintf(nb, sizeof nb, "%g", 100.0 * z.v);
                err = "'" + t + "' sale un " + nb + "% del bote, por debajo de la "
                      "apuesta minima. Los tamanos van en por ciento: dos tercios "
                      "es '66' y tres botes es '300'";
                return false;
            }
            if (!bare_is_pct && !pct && z.v >= 5.0) {
                char nb[32];
                std::snprintf(nb, sizeof nb, "%g", z.v);
                err = "'" + t + "' is " + nb + " times the pot";
                return false;
            }
            // Un techo, pero holgado: lo que este lector rechace no lo puede
            // escribir el formateador, o habria tamanos que se guardan y no se
            // vuelven a leer. Cien botes esta muy por encima de cualquier cosa
            // que el stack no tope antes, y por debajo de un dedo resbalando.
            if (z.v > 100.0) {
                char nb[32];
                std::snprintf(nb, sizeof nb, "%g", z.v);
                err = "'" + t + "' son " + nb + " botes, que no es un tamano: "
                      "van en por ciento, y tres botes es '300'";
                return false;
            }
        }
        out.push_back(z);
    }
    if (out.empty()) { err = "need at least one sizing"; return false; }
    return true;
}

inline bool parse_sizings_scale(const std::string& in, bool bare_is_pct,
                                std::vector<Sizing>& out, std::string& err) {
    return parse_sizings_sep(in, true, bare_is_pct, out, err);
}

// Aqui habia un rescate para la coma decimal a la espanola: si "0,6" no se podia
// leer como lista, se reintentaba leyendola como 0.6. Con la escala en por
// ciento ya no hace falta y ademas no llega a entrar nunca. Los tamanos son
// enteros -- 30, 66, 300 -- y no se escribe la coma; lo unico que caia en el
// rescate eran valores de la escala vieja, que ahora hay que rechazar y no
// arreglar. Dejarlo puesto habria sido codigo que ninguna entrada alcanza.
//
// Queda un caso al que no se puede llegar por las buenas: "12,5" queriendo decir
// 12,5% se lee como DOS tamanos, 12% y 5%, porque la coma separa la lista y
// "33,75" tiene que seguir siendo dos. Eso no se adivina. La red es que lo
// entendido se devuelve a la pantalla -- "bets 12%,5%" -- y ahi se ve.

// Lo que se teclea: el numero desnudo es por ciento.
inline bool parse_sizings(const std::string& in, std::vector<Sizing>& out,
                          std::string& err) {
    return parse_sizings_scale(in, true, out, err);
}
// Las subidas solo en multiplos: "3x", "2,5x", o "min".
//
// Un porcentaje tambien se podia, y era justo lo lioso: en el mismo campo "50"
// queria decir medio bote encima de la igualada y "3x" tres veces lo que tienes
// delante, dos cuentas distintas sin nada que las distinguiera salvo una letra.
//
// Y prohibir el porcentaje aqui arregla de paso la coma. "2,5x" era ambiguo --
// lista de "2" y "5x", o el decimal 2.5x -- porque las dos lecturas eran
// legales. Sin porcentajes solo queda una: "2" a secas no es una subida, luego
// la coma es decimal. Se intenta primero como lista, que es lo que hace que
// "2x,3x" siga siendo dos.
inline bool parse_raises(const std::string& in, std::vector<Sizing>& out,
                         std::string& err) {
    std::vector<Sizing> lista;
    std::string e1;
    bool ok = parse_sizings_sep(in, true, true, lista, e1);
    bool limpia = ok;
    if (ok) for (const Sizing& z : lista) if (!z.xbet) { limpia = false; break; }
    if (limpia) { out.swap(lista); return true; }

    // Si como lista no sale, y hay una coma entre digitos, era un decimal.
    bool coma_decimal = false;
    for (size_t i = 1; i + 1 < in.size(); ++i)
        if (in[i] == ',' && std::isdigit(static_cast<unsigned char>(in[i - 1])) &&
            std::isdigit(static_cast<unsigned char>(in[i + 1])))
            coma_decimal = true;
    if (coma_decimal) {
        std::vector<Sizing> alt;
        std::string e2;
        if (parse_sizings_sep(in, false, true, alt, e2)) {
            bool alt_limpia = true;
            for (const Sizing& z : alt) if (!z.xbet) { alt_limpia = false; break; }
            if (alt_limpia) { out.swap(alt); return true; }
        }
    }

    // Si en el texto no hay ni una 'x' ni un "min", lo que sobra no es un
    // tamano mal escrito: es un porcentaje en el campo equivocado. Da igual con
    // que error concreto se atragantara el lector -- "0.5" lo para el suelo de
    // decimales antes de llegar aqui -- lo que hay que decir es la regla del
    // campo, no el tropiezo del camino.
    const std::string bajo = lower(in);
    if (bajo.find('x') == std::string::npos &&
        bajo.find("min") == std::string::npos) {
        err = "las subidas van en multiplos de la apuesta que tienes delante: "
              "'3x', '2.5x', o 'min' para la minima legal. 'none' las quita";
        return false;
    }
    if (!ok) { err = e1; return false; }
    err = "las subidas van en multiplos de la apuesta que tienes delante: "
          "'3x', '2.5x', o 'min' para la minima legal. 'none' las quita";
    return false;
}

// Lo que se lee de un fichero viejo: el numero desnudo era la fraccion.
inline bool parse_sizings_file(const std::string& in, std::vector<Sizing>& out,
                               std::string& err) {
    return parse_sizings_scale(in, false, out, err);
}
inline std::string fmt_num(double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%g", v);
    return std::string(b);
}
// Lo que esto escribe lleva SIEMPRE su unidad: "30%" o "3x", nunca un numero
// desnudo. Asi el fichero dice lo que vale sin que haya que saber con que
// version se guardo, y se lee igual con las dos escalas.
//
// Antes solo se marcaban los grandes, y el fichero guardado dependia de la
// regla del lector: un "3" era tres botes para una version y un 3% para la
// siguiente. El mismo fichero, dos arboles distintos y ni un aviso.
inline std::string fmt_sizings(const std::vector<Sizing>& v) {
    if (v.empty()) return "none";
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ",";
        if (v[i].minraise) { s += "min"; }
        else if (v[i].xbet) { s += fmt_num(v[i].v); s += "x"; }
        else           { s += fmt_num(100.0 * v[i].v); s += "%"; }
    }
    return s;
}
// Bets are plain fractions; raises can be multipliers, so they have their own
// overload above. Both have to write something the parser takes back.
inline std::string fmt_sizings(const std::vector<double>& v) {
    if (v.empty()) return "none";
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ",";
        s += fmt_num(100.0 * v[i]);
        s += "%";
    }
    return s;
}

inline bool parse_int(const std::string& s, int& out) {
    double d;
    if (!parse_double(s, d)) return false;
    out = static_cast<int>(d);
    return true;
}
// Range text is whitespace-insensitive, and a config line is read back with a
// whitespace tokenizer, so the spaces have to go before the line is written.
inline std::string no_space(const std::string& s) {
    std::string o;
    for (char c : s) if (!std::isspace(static_cast<unsigned char>(c))) o += c;
    return o;
}
// A fraction that may be written as a percentage. The solver PRINTS "rake 5%"
// and used to refuse "rake 5%" typed back at it, which is the kind of small
// inconsistency that makes a tool feel unreliable: whatever it shows you
// should be something you can hand back.
inline bool parse_fraction(const std::string& in, double& out) {
    std::string t = trim(in);
    bool pct = false;
    if (!t.empty() && t.back() == (char)37) { pct = true; t.pop_back(); }
    if (!parse_double(t, out)) return false;
    if (pct) out /= 100.0;
    return true;
}
// Un si/no escrito a mano.
//
// MEDIDO: `set iso yes` dejaba el agrupado de palos APAGADO. Tres sitios
// comparaban a mano contra "on", "1" y "true" y daban por falso todo lo demas,
// asi que la palabra mas natural que se puede escribir ahi valia "no" sin
// decirlo: el mismo spot pasaba de 2796 a 9608 nodos instanciados -- el doble de
// memoria y de tiempo -- por una palabra que el programa acepto y no entendio.
// Un valor que no se reconoce ahora es un error, no un no.
inline bool parse_onoff(const std::string& in, bool& out) {
    const std::string v = lower(trim(in));
    if (v == "on"  || v == "1" || v == "true"  || v == "t" || v == "yes" ||
        v == "y"   || v == "si" || v == "s"    || v == "sí" ||
        v == "enable" || v == "enabled") { out = true;  return true; }
    if (v == "off" || v == "0" || v == "false" || v == "f" || v == "no" ||
        v == "n"   || v == "none" ||
        v == "disable" || v == "disabled") { out = false; return true; }
    return false;
}
// `parse_action_kind` vive en tree.hpp, con ActionKind y kind_code.
inline int street_of_name(const std::string& s) {
    const std::string l = lower(trim(s));
    if (l == "flop"  || l == "f") return ST_FLOP;
    if (l == "turn"  || l == "t") return ST_TURN;
    if (l == "river" || l == "r") return ST_RIVER;
    return -1;
}

// A lock survives a rebuild: the node is addressed by context label and path,
// the hands by the text the user selected, the actions by kind not index.
//
// It does not survive a change of board. Context labels are lines ("R|C"),
// not cards, so the root of a turn tree and the root of a river tree wear the
// same label: without the board recorded here, a lock set on one spot would
// quietly reattach itself to a different street's node and go on constraining
// a strategy nobody asked it to.
// How a single action is moved, with the rest of the mix rebalancing around it.
// The usual dialogs offer the first two per action; the third is the one you reach
// for when you are reading a strategy off the screen and want it a bit higher.
enum LockMode {
    LM_FIXED,   // this action becomes exactly w
    LM_SCALE,   // this action becomes its current value times w
    LM_ADD      // this action becomes its current value plus w
};

// Que accion de este nodo nombra un token del mix: primero por CODIGO exacto
// ("B33"), y si no cuadra, por TIPO ("B"), que es lo que escribe la gente en la
// consola y lo que llevan dentro las configuraciones guardadas antes de que
// esto distinguiera tamanos.
inline int action_of(const BetTree& bt, const Node& n, const std::string& tok) {
    for (int a = 0; a < n.num_actions; ++a)
        if (bt.act(n, a).code == tok) return a;
    ActionKind k;
    if (!parse_action_kind(tok, k)) return -1;
    return bt.action_index(n, k);
}

struct LockSpec {
    std::string                                board;
    std::string                                ctx_label;
    std::string                                node_path;
    // Las cartas repartidas para llegar a este nodo, p.ej. "3s" en un turn o
    // "Ah3s" en un flop. Es lo que separa "que sobreapueste en el 3s" de "que
    // sobreapueste en cualquier river", que son dos preguntas distintas y en un
    // turn de 48 cartas se llevan tres fichas sobre un bote de 100.
    //
    // Vacio significa "en todos los runouts": es la semantica que tenia esto
    // antes, y se conserva para poder leer las configuraciones ya guardadas.
    std::string                                runout;
    std::string                                hand_spec;
    std::vector<std::pair<std::string, double>> mix;
};

// =============================================================================
class Session {
public:
    Session() {
        board_spec_ = "Ah9h4h";          // start on the flop: the whole game
        // No default ranges. Guessing what someone meant to study is worse than
        // making them say it, and a leftover default that silently solves the
        // wrong spot is the kind of thing you only notice much later.
        range_spec_[0].clear();
        range_spec_[1].clear();
        std::string e;
        reset_deal(e);
        rebuild(e);
    }

    // Nothing can be solved until both players have a live range.
    const std::string& build_error() const { return build_error_; }
    bool ranges_ready() const {
        for (int p = 0; p < 2; ++p) {
            bool live = false;
            for (double x : range_[p]) if (x > 0.0) { live = true; break; }
            if (!live) return false;
        }
        return true;
    }
    std::string not_ready_reason() const {
        const bool a = !range_spec_[0].empty(), b = !range_spec_[1].empty();
        if (!a && !b) return M("define los rangos de OOP y de IP",
                               "set the OOP and IP ranges");
        if (!a)       return M("define el rango de OOP", "set the OOP range");
        if (!b)       return M("define el rango de IP", "set the IP range");
        return M("los rangos no dejan ninguna combinación en este board",
                 "the ranges leave no combination on this board");
    }

    // ---- accessors -------------------------------------------------------
    const Deal&        deal()       const { return deal_; }
    // A tree with no raise sizings anywhere has no raises, which is correct and
    // is also the first thing somebody concludes is a bug. Worth being able to
    // say out loud rather than leaving them to work it out from an empty field.
    bool no_raises_anywhere() const {
        for (int st = deal_.start; st < 3; ++st)
            for (int q = 0; q < 2; ++q)
                if (!tc_.raises[st][q].empty()) return false;
        return true;
    }
    const GameTree&    tree()       const { return tree_; }
    DCFRSolver*        solver()     const { return S_.get(); }
    bool               solved()     const { return solved_ && S_; }
    const std::string& board_spec() const { return board_spec_; }
    const std::string& range_spec(int p) const { return range_spec_[p]; }
    const std::vector<double>& range(int p) const { return range_[p]; }
    const TreeConfig&  tc()         const { return tc_; }
    const std::vector<LockSpec>& locks() const { return locks_; }
    int  iters() const { return iters_; }
    void set_iters(int n) { if (n > 0) iters_ = n; }

    // Un objetivo de precision, como el de la referencia: parar cuando la
    // explotabilidad baje del X% del bote. El tope de iteraciones pasa a ser
    // una red de seguridad y no el criterio -- que es lo que tiene que ser,
    // porque medido aqui mismo la explotabilidad NO baja de forma monotona con
    // las iteraciones: en un flop de medio bote iba de 0,0042 a 1810 vueltas a
    // 0,0618 a 1880, quince veces peor haciendo mas trabajo. Un contador no
    // dice donde has caido; un objetivo si.
    //
    // El objetivo esta en la convencion estandar, igual que todo lo que se ensena:
    // lo pone expl_shown(). Antes se dividia por dos aqui a mano y la pantalla no,
    // y de ahi salia que pidieras 1% y te ensenara 1,46%.
    double acc_target() const { return acc_target_; }
    bool   acc_stop()   const { return acc_stop_; }
    void   set_acc_target(double pct) { if (pct >= 0.0) acc_target_ = pct; }
    void   set_acc_stop(bool on)      { acc_stop_ = on; }
    bool   acc_reached() const { return acc_hit_.load(); }
    // El tope de TIEMPO de un solve, en segundos. 0 = sin tope.
    //
    // Es lo que hace falta para dejar una lista de boards corriendo de noche: sin
    // esto, un board que no alcanza la precision pedida se come las horas que le
    // tocaban a los otros veinte. Con el objetivo de precision se llevan bien:
    // para el primero que llegue, y dice cual de los dos fue.
    double timeout_secs() const { return timeout_secs_; }
    void   set_timeout_secs(double v) { timeout_secs_ = (v > 0.0) ? v : 0.0; }
    bool   timeout_reached() const { return timeout_hit_.load(); }
    bool   accuracy_met() const {
        if (!acc_stop_ || acc_target_ <= 0.0 || cfg::POT0 <= 0.0) return false;
        const double e = expl_.load();
        if (e < 0.0) return false;
        return (100.0 * expl_shown(e) / cfg::POT0) <= acc_target_;
    }
    void invalidate() { solved_ = false; }
    // Hay locks puestos que el solve todavia no ha metido dentro. La estrategia
    // en pantalla es buena, pero es la de ANTES de esos locks.
    bool locks_pending() const { return locks_pending_ && !locks_.empty(); }
    int  dropped_locks() const { return dropped_locks_; }
    bool locks_pending_ = false;
    // Set by a load that had to reinterpret something in the file. Empty when
    // the file said exactly what the solver still means.
    const std::string& load_note() const { return load_note_; }
    // True when suit collapsing was asked for and turned down because a lock
    // names cards. Worth saying: the solve is correct either way, but it is
    // several times slower and the reason is not obvious.
    bool iso_off_by_lock() const { return iso_off_by_lock_; }

    int       cur_ctx()  const { return cur_ctx_; }
    int       cur_node() const { return cur_node_; }
    const std::vector<int>& slots() const { return slots_; }
    // With suit isomorphism only representative runouts are stored, so a
    // concrete runout resolves to (stored instance, permutation back to it).
    bool cur_addr(long long& inst, int& perm) const {
        return deal_.locate(slots_, inst, perm);
    }
    long long cur_inst() const {
        long long i = 0; int p = 0;
        deal_.locate(slots_, i, p);
        return i;
    }
    int cur_perm() const {
        long long i = 0; int p = deal_.identity();
        deal_.locate(slots_, i, p);
        return p;
    }
    const Node& node() const {
        return tree_.ctx[static_cast<size_t>(cur_ctx_)].tree.nodes[static_cast<size_t>(cur_node_)];
    }
    const BetTree& btree() const { return tree_.ctx[static_cast<size_t>(cur_ctx_)].tree; }

    // Las cartas repartidas para llegar a un contexto, sin espacios: "3s",
    // "Ah3s". Vacio donde todavia no ha caido ninguna, y ahi no hay runout que
    // distinguir porque la decision se toma una sola vez.
    //
    // Se recorta a las calles que ese contexto tiene POR ENCIMA, no a lo que
    // haya en slots_. El cursor de la sesion puede venir de otro sitio -- las
    // comprobaciones reutilizan la misma sesion, y una navegacion anterior deja
    // sus ranuras puestas -- y usarlas tal cual metia el river de otra prueba
    // en un lock del flop. Con el binario optimizado eso era un segfault; con
    // suerte, porque en silencio habria sido peor.
    std::string runout_of_ctx(const RoundCtx& rc, const std::vector<int>& slots) const {
        const int levels = rc.street - deal_.start;
        std::string s;
        if (levels <= 0) return s;
        for (size_t i = 0; i < slots.size() && static_cast<int>(i) < levels; ++i) {
            const int sl = slots[i];
            if (sl < 0 || sl >= deal_.deckN()) return std::string();
            s += card_str(deal_.deck[static_cast<size_t>(sl)]);
        }
        return s;
    }
    std::string cur_runout() const {
        return runout_of_ctx(tree_.ctx[static_cast<size_t>(cur_ctx_)], slots_);
    }

    // El board ENTERO donde estas: el de partida mas las cartas repartidas
    // para llegar a este contexto.
    //
    // Hace falta en cuanto se pregunta algo sobre la mano y no sobre el arbol
    // -- que tiene cada uno hecho, cuales son las nueces -- porque esas
    // preguntas cambian con cada carta que sale. Con el board de partida, en
    // Ah9h4h/Kd un AK seguia figurando como pareja top cuando ya es doble
    // pareja, y la columna decia una cosa que no era.
    // Una sola manera de armarlo, para la consola y para la web. La web navega
    // por parametros y la consola por cursor, y con la cuenta de calles escrita
    // dos veces solo hace falta arreglar una para que la otra siga mintiendo.
    std::vector<int> board_of(int ci, const std::vector<int>& slots) const {
        std::vector<int> b = deal_.board;
        if (ci < 0 || ci >= static_cast<int>(tree_.ctx.size())) return b;
        const int levels = tree_.ctx[static_cast<size_t>(ci)].street - deal_.start;
        for (size_t i = 0; i < slots.size() && static_cast<int>(i) < levels; ++i) {
            const int sl = slots[i];
            if (sl < 0 || sl >= deal_.deckN()) return deal_.board;   // no me invento nada
            b.push_back(deal_.deck[static_cast<size_t>(sl)]);
        }
        return b;
    }
    std::vector<int> board_here() const { return board_of(cur_ctx_, slots_); }
    // Y de vuelta: de "3s" a las ranuras de la baraja. Falla si alguna carta ya
    // no existe -- tablero distinto, o carta que ahora esta en el board.
    bool runout_slots(const std::string& r, std::vector<int>& out) const {
        out.clear();
        if (r.empty()) return true;
        if (r.size() % 2 != 0) return false;
        for (size_t i = 0; i + 1 < r.size(); i += 2) {
            const int c = parse_card(r.substr(i, 2));
            if (c < 0) return false;
            const int sl = deal_.slot_of[c];
            if (sl < 0) return false;
            out.push_back(sl);
        }
        return true;
    }

    // "R|R/X  (Ts Kd)  OOP to act, pot 30.0"
    std::string where() const {
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(cur_ctx_)];
        const Node& n = node();
        std::string s = rc.label + " " + n.path;
        if (!slots_.empty()) {
            s += "  [";
            for (size_t i = 0; i < slots_.size(); ++i) {
                if (i) s += " ";
                s += card_str(deal_.deck[static_cast<size_t>(slots_[i])]);
            }
            s += "]";
        }
        return s;
    }
    std::string prompt() const {
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(cur_ctx_)];
        std::string s = STREET_NAME[rc.street];
        s += ":";
        s += node().path;
        for (int sl : slots_) { s += "/"; s += card_str(deal_.deck[static_cast<size_t>(sl)]); }
        return s;
    }

    // ---- configuration ---------------------------------------------------
    bool set_board(const std::string& spec, std::string& e) {
        // Re-setting the board it already has is a no-op for the deal, and the
        // deal is the expensive part: a flop board means building strength
        // tables for 2401 runouts. The tree is still rebuilt, so the solver is
        // discarded either way and the caller sees no difference.
        if (trim(spec) == board_spec_ && deal_.num() > 0) return rebuild(e);
        load_note_.clear();
        const std::string backup = board_spec_;
        board_spec_ = trim(spec);
        if (!reset_deal(e)) { board_spec_ = backup; reset_deal(e); return false; }
        return rebuild(e);
    }
    bool set_range(int p, const std::string& spec, std::string& e) {
        if (p < 0 || p > 1) { e = M("el jugador es oop o ip", "player must be oop or ip"); return false; }
        if (trim(spec).empty()) {                 // clearing it is legitimate
            range_spec_[p].clear();
            range_[p].assign(static_cast<size_t>(deal_.num()), 0.0);
            return rebuild(e);
        }
        std::vector<double> w;
        if (!parse_range(spec, deal_, w, e)) return false;
        int live = 0;
        for (double x : w) if (x > 0.0) ++live;
        if (live == 0) { e = M("ese rango no tiene ninguna mano en este board", "that range is empty on this board"); return false; }
        range_spec_[p] = spec;
        range_[p] = w;
        // Range symmetry decides whether isomorphism is legal, and that changes
        // how many runouts are stored, so the tree has to be rebuilt.
        return rebuild(e);
    }
    // El pot y el stack cambian el arbol -- un stack de 1e23 hace una escalera de
    // subidas que no termina -- y si no sale, se deshacen. Antes se quedaban
    // puestos con el arbol viejo en memoria, o sea que `show` contaba un pot que
    // el arbol no tenia.
    bool set_pot(double v, std::string& e) {
        if (v <= 0.0) { e = M("el bote tiene que ser mayor que 0", "pot must be > 0"); return false; }
        if (v == cfg::POT0) return true;
        const double back = cfg::POT0;
        cfg::POT0 = v;
        if (!rebuild(e)) { cfg::POT0 = back; return false; }
        return true;
    }
    bool set_stack(double v, std::string& e) {
        if (v <= 0.0) { e = M("el stack tiene que ser mayor que 0", "stack must be > 0"); return false; }
        if (v == cfg::STACK) return true;
        const double back = cfg::STACK;
        cfg::STACK = v;
        if (!rebuild(e)) { cfg::STACK = back; return false; }
        return true;
    }
    // street < 0 applies to every street.
    // Un arbol que no se puede construir deja la config como estaba, asi que la
    // sesion nunca se queda con un arbol que no concuerda con lo que dice tener.
    //
    // Y no hay que rehacer nada: rebuild() no toca el arbol, ni la solucion, ni el
    // cursor, ni los locks hasta que el arbol nuevo esta armado y aceptado. Lo que
    // hay en memoria sigue siendo el de `back`. Antes esto lo rehacia -- y ese
    // segundo rebuild era el que te borraba el solve por un campo mal escrito.
    bool rebuild_or_revert(const TreeConfig& back, std::string& e) {
        if (rebuild(e)) return true;
        tc_ = back;
        return false;
    }
    // What a list of sizings has to satisfy before it goes near the builder.
    // Empty passes, and is load-bearing: it is how a street says the action
    // does not exist. That used to be the bet+raise cap's job, and having both
    // meant the sizings could be overruled without a word.
    static bool sizings_are_sane(bool bets, const std::vector<Sizing>& sz, std::string& e) {
        for (const Sizing& z : sz) {
            if (z.v <= 0.0) { e = M("los tamaños tienen que ser positivos", "sizings must be positive"); return false; }
            if (bets && z.xbet) {
                e = M("los tamaños con 'x' son solo para subir: una apuesta de apertura no tiene nada que multiplicar", "'x' sizes are for raises only -- an opening bet has no bet to multiply");
                return false;
            }
            if (z.xbet && !z.minraise && z.v < 2.0) {
                e = M("una subida con 'x' es 2x como mínimo (2x ES la subida mínima sobre una primera apuesta)", "an 'x' raise must be at least 2x (2x IS the min-raise over a first bet)");
                return false;
            }
        }
        return true;
    }
    // street < 0 is every street, player < 0 is both players.
    bool set_sizings_for(bool bets, int player, int street,
                         const std::vector<Sizing>& sz, std::string& e) {
        if (!sizings_are_sane(bets, sz, e)) return false;
        const TreeConfig back = tc_;
        std::vector<double> vs;
        for (const Sizing& z : sz) vs.push_back(z.v);
        for (int s = 0; s < 3; ++s) {
            if (street >= 0 && s != street) continue;
            for (int q = 0; q < 2; ++q) {
                if (player >= 0 && q != player) continue;
                if (bets) tc_.bets[s][q] = vs; else tc_.raises[s][q] = sz;
            }
        }
        return rebuild_or_revert(back, e);
    }
    bool set_sizings(bool bets, int street, const std::vector<Sizing>& sz, std::string& e) {
        return set_sizings_for(bets, -1, street, sz, e);
    }
    // OOP leading into last street's aggressor. Its own list, because it is its
    // own decision: you do not lead into the raiser with the size you
    // continuation-bet. Refused on the street the solve starts on -- there is
    // no previous street there, so nobody was aggressive, so OOP opens with its
    // own bet sizes and the builder never looks at this list.
    bool set_donks(int street, const std::vector<Sizing>& sz, std::string& e) {
        if (!sz.empty() && street >= 0 && street == deal_.start) {
            e = std::string(M("no hay donk en el ", "there are no donk bets on the ")) +
                STREET_NAME[street] +
                M(": es la calle donde empieza el solve, nadie fue agresivo antes, "
                  "así que OOP abre con sus propios tamaños de apuesta",
                  ": it is the street this solve starts on, so nobody was aggressive "
                  "before it and OOP opens with its own bet sizes anyway");
            return false;
        }
        if (!sizings_are_sane(true, sz, e)) return false;
        const TreeConfig back = tc_;
        std::vector<double> vs;
        for (const Sizing& z : sz) vs.push_back(z.v);
        for (int s = 0; s < 3; ++s) {
            if (street >= 0 && s != street) continue;
            if (s == deal_.start) continue;
            tc_.donks[s] = vs;
        }
        return rebuild_or_revert(back, e);
    }
    // IP only, as is standard: no third aggressive action of the street from IP.
    bool set_no3bet(int street, bool on, std::string& e) {
        const TreeConfig back = tc_;
        for (int s = 0; s < 3; ++s) if (street < 0 || s == street) tc_.no_3bet[s] = on;
        return rebuild_or_revert(back, e);
    }
    // The usual rule, and the usual number: a bet committing more than this fraction of
    // the starting effective stack becomes an all-in. One number for the hand,
    // not one per street -- what it measures does not change with the street.
    bool set_allin_thresh(double v, std::string& e) {
        if (v < 0.0 || v > 1.0) {
            e = M("el umbral de all-in es una fracción del stack de partida, así que "
                  "va de 0 a 1: dos tercios es '0.67' o '67%'. Con 1 se desactiva",
                  "the all-in threshold is a fraction of the starting stack, so it "
                  "runs 0 to 1: two thirds is '0.67' or '67%'. 1 turns it off");
            return false;
        }
        if (v == cfg::ALLIN_THRESH) return true;
        const double back = cfg::ALLIN_THRESH;
        cfg::ALLIN_THRESH = v;
        if (!rebuild(e)) { cfg::ALLIN_THRESH = back; return false; }
        return true;
    }
    bool set_allin_for(int player, int street, bool on, std::string& e) {
        const TreeConfig back = tc_;
        for (int s = 0; s < 3; ++s) {
            if (street >= 0 && s != street) continue;
            for (int q = 0; q < 2; ++q) {
                if (player >= 0 && q != player) continue;
                tc_.add_allin[s][q] = on;
            }
        }
        return rebuild_or_revert(back, e);
    }
    bool set_allin(int street, bool on, std::string& e) {
        return set_allin_for(-1, street, on, e);
    }
    // Poner lo que ya hay puesto no es un cambio, y no puede costar la
    // solucion: rebuild() la tira, y estos dos se mandan enteros cada vez que
    // alguien da a Aplicar en el engranaje aunque no haya tocado el campo.
    bool set_rake(double pct, double cap, std::string& e) {
        if (pct < 0.0 || pct > 0.5) { e = M("el rake va entre 0 y 0.5", "rake must be between 0 and 0.5"); return false; }
        if (cap < 0.0)              { e = M("el tope de rake no puede ser negativo", "the rake cap cannot be negative"); return false; }
        if (pct == cfg::RAKE_PCT && cap == cfg::RAKE_CAP) return true;
        const double bp = cfg::RAKE_PCT, bc = cfg::RAKE_CAP;
        cfg::RAKE_PCT = pct;
        cfg::RAKE_CAP = cap;
        if (!rebuild(e)) { cfg::RAKE_PCT = bp; cfg::RAKE_CAP = bc; return false; }
        return true;
    }

    // ---- saved setups and saved solutions ---------------------------------
    //
    //  Two different things, deliberately kept apart:
    //    a CONFIG is the recipe -- board, ranges, sizings, everything you typed
    //    in. A couple of kilobytes, readable, diffable, easy to share.
    //    a TREE is the recipe plus the solution itself, both float buffers. It
    //    is as big as the solve was, and it exists so you can come back to a
    //    finished spot without paying for it again, or carry on iterating.
    //
    //  The tree file embeds its own config, so loading one restores the setup
    //  first and then drops the numbers back in on top.

    // Saved work belongs to the program, not to whatever directory you happened
    // to be in when you launched it. Hanging `saves/` off the current directory
    // meant starting the solver from somewhere else made your setups vanish
    // with no message at all.
    static std::string saves_dir(bool tree) {
        return exe_dir() + "/saves/" + (tree ? "trees" : "configs");
    }
    // Names become filenames, so they get to be boring on purpose.
    static bool valid_name(const std::string& n, std::string& e) {
        if (n.empty() || n.size() > 64) { e = M("el nombre tiene entre 1 y 64 caracteres", "the name must be 1-64 characters"); return false; }
        for (char c : n)
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.')) {
                e = M("solo letras, números, '_', '-' y '.'", "use only letters, digits, '_', '-' and '.'");
                return false;
            }
        if (n.front() == '.') { e = M("el nombre no puede empezar por un punto", "the name cannot start with a dot"); return false; }
        return true;
    }
    static std::string save_path(bool tree, const std::string& name) {
        return saves_dir(tree) + "/" + name + (tree ? ".tree" : ".cfg");
    }
    // v1 had no trailing checksum; it is still readable.
    enum { TREE_FORMAT = 2 };

    std::string config_text() const {
        std::ostringstream o;
        o << "# poker solver config\n";
        o << "board " << board_spec_ << "\n";
        o << "oop " << range_spec_[0] << "\n";
        o << "ip " << range_spec_[1] << "\n";
        o << "pot " << fmt_num(cfg::POT0) << "\n";
        o << "stack " << fmt_num(cfg::STACK) << "\n";
        o << "alpha " << fmt_num(cfg::DCFR_ALPHA) << "\n";
        o << "beta " << fmt_num(cfg::DCFR_BETA) << "\n";
        o << "gamma " << fmt_num(cfg::DCFR_GAMMA) << "\n";
        o << "rake " << fmt_num(cfg::RAKE_PCT) << "," << fmt_num(cfg::RAKE_CAP) << "\n";
        o << "allinpct " << fmt_num(cfg::ALLIN_THRESH) << "\n";
        o << "iters " << iters_ << "\n";
        o << "accuracy " << acc_target_ << " " << (acc_stop_ ? 1 : 0) << "\n";
        o << "threads " << cfg::THREADS << "\n";
        o << "iso " << (cfg::ISO ? 1 : 0) << "\n";
        // One line for what the street shares, then one per player. The player
        // token is what tells them apart; a line without one comes from before
        // the split and applies to both.
        for (int s = 0; s < 3; ++s) {
            o << "street " << STREET_NAME[s] << " oop"
              << " bets "   << fmt_sizings(tc_.bets[s][0])
              << " raises " << fmt_sizings(tc_.raises[s][0])
              << " donks "  << fmt_sizings(tc_.donks[s])
              << " allin "  << (tc_.add_allin[s][0] ? 1 : 0) << "\n";
            o << "street " << STREET_NAME[s] << " ip"
              << " bets "   << fmt_sizings(tc_.bets[s][1])
              << " raises " << fmt_sizings(tc_.raises[s][1])
              << " allin "  << (tc_.add_allin[s][1] ? 1 : 0)
              << " no3bet " << (tc_.no_3bet[s] ? 1 : 0) << "\n";
        }
        // Locks last: they name nodes, so they only mean anything once the
        // streets above have described the tree those nodes live in.
        for (const LockSpec& L : locks_) {
            o << "lock " << L.ctx_label << " " << L.node_path << " "
              << (L.runout.empty() ? "*" : L.runout) << " "
              << no_space(L.hand_spec) << " ";
            for (size_t i = 0; i < L.mix.size(); ++i)
                o << (i ? "," : "") << L.mix[i].first << "="
                  << fmt_num(L.mix[i].second);
            o << "\n";
        }
        return o.str();
    }

    // Applies a whole config in one go. Anything the file does not mention is
    // left alone, and the tree is rebuilt exactly once at the end.
    bool load_config_text(const std::string& text, std::string& e) {
        load_note_.clear();
        TreeConfig  nt   = tc_;
        std::string nb   = board_spec_;
        std::string nr[2] = { range_spec_[0], range_spec_[1] };
        double np = cfg::POT0, ns = cfg::STACK;
        double na = cfg::DCFR_ALPHA, nbeta = cfg::DCFR_BETA, ng = cfg::DCFR_GAMMA;
        double rkp = cfg::RAKE_PCT, rkc = cfg::RAKE_CAP;
        double nap = cfg::ALLIN_THRESH;
        int    ni = iters_, nth = cfg::THREADS, niso = cfg::ISO ? 1 : 0;
        double nacc = acc_target_; int nstop = acc_stop_ ? 1 : 0;

        std::vector<LockSpec> nlocks;

        std::istringstream in(text);
        std::string line;
        int lineno = 0;
        while (std::getline(in, line)) {
            ++lineno;
            const std::string t = trim(line);
            if (t.empty() || t[0] == '#') continue;
            const std::vector<std::string> w = tokenize(t);
            const std::string k = lower(w[0]);
            auto need = [&](size_t n) { return w.size() >= n; };
            auto bad = [&](const std::string& m) {
                e = M("línea ", "line ") + std::to_string(lineno) + ": " + m;
                return false;
            };
            if (k == "lock") {
                // Formato nuevo: lock <ctx> <nodo> <runout|*> <manos> <mezcla>.
                // El viejo no traia runout y valia para todos los runouts, que
                // es justo lo que "*" significa: una configuracion guardada
                // antes de este cambio sigue queriendo decir lo mismo que
                // queria decir entonces.
                const bool viejo = (w.size() == 5);
                if (w.size() < 5) return bad("lock wants <ctx> <node> <runout> <hands> <mix>");
                LockSpec L;
                L.board     = nb;
                L.ctx_label = w[1];
                L.node_path = w[2];
                L.runout    = viejo ? "" : (w[3] == "*" ? "" : w[3]);
                L.hand_spec = viejo ? w[3] : w[4];
                for (const std::string& item : split(w[viejo ? 4 : 5], (char)44)) {
                    if (item.empty()) continue;
                    const size_t eq = item.find((char)61);
                    if (eq == std::string::npos) return bad("lock mix wants ACT=p");
                    // El token se guarda TAL CUAL: puede ser un codigo de
                    // accion ("B33") o un tipo ("B"), y cual de los dos es solo
                    // se sabe contra el arbol, que aqui todavia no esta montado.
                    // Si al aplicarlo no existe, el lock se cae con su aviso.
                    double p = 0.0;
                    const std::string tok = trim(item.substr(0, eq));
                    if (tok.empty()) return bad("bad action in lock mix");
                    if (!parse_double(item.substr(eq + 1), p) || p < 0.0)
                        return bad("bad probability in lock mix");
                    L.mix.push_back(std::make_pair(tok, p));
                }
                if (L.mix.empty()) return bad("empty lock mix");
                nlocks.push_back(L);
            }
            else if (k == "board" && need(2))   nb = w[1];
            else if (k == "oop")                nr[0] = (w.size() >= 2) ? w[1] : "";
            else if (k == "ip")                 nr[1] = (w.size() >= 2) ? w[1] : "";
            else if (k == "pot" && need(2))     { if (!parse_double(w[1], np)) return bad("bad pot"); }
            else if (k == "stack" && need(2))   { if (!parse_double(w[1], ns)) return bad("bad stack"); }
            else if (k == "alpha" && need(2))   { if (!parse_double(w[1], na)) return bad("bad alpha"); }
            else if (k == "beta" && need(2))    { if (!parse_double(w[1], nbeta)) return bad("bad beta"); }
            else if (k == "gamma" && need(2))   { if (!parse_double(w[1], ng)) return bad("bad gamma"); }
            else if (k == "rake" && need(2)) {
                const std::vector<std::string> p2 = split(w[1], ',');
                if (p2.size() != 2 || !parse_double(p2[0], rkp) || !parse_double(p2[1], rkc))
                    return bad("rake wants pct,cap");
            }
            else if (k == "allinpct" && need(2)) {
                if (!parse_fraction(w[1], nap) || nap < 0.0 || nap > 1.0)
                    return bad("bad allinpct: a fraction of the starting stack, 0 to 1");
            }
            else if (k == "iters" && need(2))   { if (!parse_int(w[1], ni)) return bad("bad iters"); }
            else if (k == "accuracy" && need(2)) {
                if (!parse_double(w[1], nacc)) return bad("bad accuracy");
                if (w.size() > 2 && !parse_int(w[2], nstop)) return bad("bad accuracy flag");
            }
            else if (k == "threads" && need(2)) { if (!parse_int(w[1], nth)) return bad("bad threads"); }
            else if (k == "iso" && need(2))     { if (!parse_int(w[1], niso)) return bad("bad iso"); }
            // Configs written when there were hand-type buckets. The three
            // numbers cut nuts from value from bluff-catcher from air, and there
            // are no buckets any more, so the line is read and dropped -- said
            // out loud, like every other field that stopped meaning something.
            else if (k == "eq" && need(2)) {
                load_note_ = "this config sets the old nuts/value/bluff-catcher "
                             "equity cuts; those buckets are gone and the line is "
                             "ignored";
            } else if (k == "street" && need(2)) {
                int s = -1;
                for (int j = 0; j < 3; ++j) if (lower(w[1]) == lower(STREET_NAME[j])) s = j;
                if (s < 0) return bad("unknown street '" + w[1] + "'");
                // A player token after the street name aims every field on the
                // line at that player. Without one the line is from before the
                // split and means both, which is exactly what it meant then.
                size_t i = 2;
                int p0 = 0, p1 = 1;
                if (w.size() > 2 && (lower(w[2]) == "oop" || lower(w[2]) == "ip")) {
                    p0 = p1 = (lower(w[2]) == "ip") ? 1 : 0;
                    i = 3;
                }
                for (; i + 1 < w.size(); i += 2) {
                    const std::string f = lower(w[i]), v = w[i + 1];
                    int iv = 0; double dv = 0.0; bool bv = false;
                    if (f == "bets") {
                        std::vector<Sizing> sz;
                        if (!parse_sizings_file(v, sz, e)) return bad(e);
                        std::vector<double> vs;
                        for (const Sizing& z : sz) {
                            if (z.xbet) return bad("'x' sizes are for raises only");
                            vs.push_back(z.v);
                        }
                        for (int q = p0; q <= p1; ++q) nt.bets[s][q] = vs;
                    } else if (f == "raises") {
                        std::vector<Sizing> sz;
                        if (!parse_sizings_file(v, sz, e)) return bad(e);
                        for (int q = p0; q <= p1; ++q) nt.raises[s][q] = sz;
                    } else if (f == "donks") {
                        std::vector<Sizing> sz;
                        if (!parse_sizings_file(v, sz, e)) return bad(e);
                        nt.donks[s].clear();
                        for (const Sizing& z : sz) {
                            if (z.xbet) return bad("'x' sizes are for raises only");
                            nt.donks[s].push_back(z.v);
                        }
                    } else if (f == "no3bet") {
                        if (!parse_onoff(v, bv)) return bad("no3bet is on or off, not '" + v + "'");
                        nt.no_3bet[s] = bv;
                    } else if (f == "cap") {
                        // Configs written before the cap was removed. It said
                        // how many bets+raises a street allowed, counting the
                        // bet as the first; the raise list says that now. A cap
                        // of 1 or 0 is the only part that still carries
                        // information, and it means "no raises".
                        if (!parse_int(v, iv) || iv < 0) return bad("bad cap");
                        for (int q = p0; q <= p1; ++q) {
                            if (iv <= 1) nt.raises[s][q].clear();
                            if (iv <= 0) nt.bets[s][q].clear();
                        }
                    } else if (f == "allin") {
                        if (!parse_onoff(v, bv)) return bad("allin is on or off, not '" + v + "'");
                        for (int q = p0; q <= p1; ++q) nt.add_allin[s][q] = bv;
                    } else if (f == "donk") {
                        // The old on/off switch. On meant OOP could lead with
                        // its normal bet sizes, so that is exactly what it
                        // translates to; off meant no lead, which is an empty
                        // list. Reading it needs the bets, so a config that
                        // writes bets after donk on the same line would come out
                        // wrong -- ours never did, and never will now that the
                        // field is gone from the writer.
                        if (!parse_onoff(v, bv)) return bad("donk is on or off, not '" + v + "'");
                        nt.donks[s] = bv ? nt.bets[s][0] : std::vector<double>();
                    } else if (f == "thresh") {
                        // Configs from when the threshold was per street and was
                        // measured against the resulting POT. It is one number
                        // for the hand now and measured against the starting
                        // STACK, so the old value does not convert -- the two
                        // mean different things. Read, dropped, and said out
                        // loud, because a number quietly ignored is the whole
                        // family of bug this project keeps finding.
                        if (!parse_double(v, dv) || dv < 0.0 || dv > 5.0) return bad("bad thresh");
                        load_note_ = "this config carries the old per-street all-in "
                                     "threshold (" + fmt_num(dv) + " of the pot). That "
                                     "is measured against the starting stack now, one "
                                     "number for the hand, and the file's value does not "
                                     "convert -- using " + fmt_num(cfg::ALLIN_THRESH);
                    } else if (f == "merge") {
                        // Configs from when this was a per-street field. It is
                        // one pinned number now, so a file asking for another
                        // one does not get it -- and is told, rather than left
                        // to notice that its tree came out a different shape.
                        if (!parse_double(v, dv) || dv < 0.0 || dv > 0.9) return bad("bad merge");
                        if (std::fabs(dv - cfg::MERGE_PCT) > 1e-9)
                            load_note_ = "this config asked to merge sizings within " +
                                         fmt_num(dv) + " on the " + STREET_NAME[s] +
                                         "; that is pinned at " + fmt_num(cfg::MERGE_PCT) +
                                         " now and the tree is built with it";
                    } else {
                        return bad("unknown street field '" + w[i] + "'");
                    }
                }
            } else {
                return bad("unknown key '" + w[0] + "'");
            }
        }
        if (np <= 0.0 || ns <= 0.0) { e = M("el bote y el stack tienen que ser mayores que 0", "pot and stack must be > 0"); return false; }

        // Everything parsed. Swap it in, and roll the whole lot back together
        // if the board or a range turns out not to fit.
        const std::string ob = board_spec_, or0 = range_spec_[0], or1 = range_spec_[1];
        const TreeConfig  ot = tc_;
        const std::vector<LockSpec> ol = locks_;
        const double op = cfg::POT0, os = cfg::STACK, oa = cfg::ALLIN_THRESH;
        board_spec_ = nb;
        range_spec_[0] = nr[0];
        range_spec_[1] = nr[1];
        tc_ = nt;
        cfg::POT0 = np; cfg::STACK = ns; cfg::ALLIN_THRESH = nap;
        cfg::DCFR_ALPHA = na; cfg::DCFR_BETA = nbeta; cfg::DCFR_GAMMA = ng;
        cfg::RAKE_PCT = rkp; cfg::RAKE_CAP = rkc;
        cfg::ISO = (niso != 0);
        // The rebuild below is what decides whether a lock still addresses a
        // node that exists on this board, and counts the ones that do not.
        locks_ = nlocks;
        if (ni > 0) iters_ = ni;
        if (nacc >= 0.0) acc_target_ = nacc;
        acc_stop_ = (nstop != 0);
        if (nth >= 0) cfg::THREADS = nth;
        if (!reset_deal(e) || !rebuild(e)) {
            board_spec_ = ob; range_spec_[0] = or0; range_spec_[1] = or1;
            tc_ = ot; cfg::POT0 = op; cfg::STACK = os; cfg::ALLIN_THRESH = oa;
            locks_ = ol;
            std::string ignored;
            reset_deal(ignored);
            rebuild(ignored);
            return false;
        }
        return true;
    }

    static std::vector<std::string> list_in(const std::string& dir, const char* want) {
        std::vector<std::string> out;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return out;
        for (const auto& d : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!d.is_regular_file()) continue;
            if (d.path().extension().string() != want) continue;
            out.push_back(d.path().stem().string());
        }
        std::sort(out.begin(), out.end());
        return out;
    }
    std::vector<std::string> list_saves(bool tree) const {
        return list_in(saves_dir(tree), tree ? ".tree" : ".cfg");
    }

    long long save_size(bool tree, const std::string& name) const {
        std::error_code ec;
        const auto n = std::filesystem::file_size(save_path(tree, name), ec);
        return ec ? -1 : static_cast<long long>(n);
    }

    bool delete_save(bool tree, const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        std::error_code ec;
        if (!std::filesystem::remove(save_path(tree, name), ec) || ec) {
            e = "'" + name + M("' no existe", "' does not exist");
            return false;
        }
        return true;
    }

    // ---- rangos guardados -------------------------------------------------
    //  Pintar un rango cuesta tiempo y se repite entre simulaciones parecidas.
    //  Se guardan como el texto del rango, que es lo que la gente escribiria de
    //  todas formas: legible, diminuto, y editable con el bloc de notas.
    //
    //  Sin jugador dentro a proposito. Un rango guardado desde OOP se puede
    //  cargar en IP, que es justo lo que quieres al montar el mismo spot desde
    //  el otro lado.
    static std::string ranges_dir() { return exe_dir() + "/saves/ranges"; }
    static std::string range_path(const std::string& name) {
        return ranges_dir() + "/" + name + ".rng";
    }

    // Los rangos de fabrica, la primera vez.
    //
    // Quien se baja el .exe se encuentra la lista vacia, y montar un rango de
    // 25bb a mano es media hora antes de poder resolver nada.
    //
    // SOLO si no hay ninguno: los tuyos no se tocan, y uno que borres no vuelve
    // al arrancar -- que es lo que convertiria un regalo en una molestia.
    //
    // La regla vive aqui sola, en una funcion que no toca el disco, porque es lo
    // unico que hay que comprobar y con la carpeta de verdad delante no se puede:
    // para ver que con SEIS rangos guardados no se rellena el septimo haria falta
    // borrar uno del usuario.
    static bool should_install_defaults(const std::vector<std::string>& hay) {
        return hay.empty();
    }

    // Devuelve cuantos escribio.
    int install_default_ranges() {
        if (!should_install_defaults(list_ranges())) return 0;
        std::error_code ec;
        std::filesystem::create_directories(ranges_dir(), ec);
        int n = 0;
        for (const RangoDeFabrica& r : DEFAULT_RANGES) {
            std::string e;
            if (!valid_name(r.nombre, e)) continue;
            const std::string ruta = range_path(r.nombre);
            if (std::filesystem::exists(ruta, ec)) continue;
            std::FILE* f = std::fopen(ruta.c_str(), "wb");
            if (!f) continue;
            const size_t len = std::strlen(r.texto);
            if (std::fwrite(r.texto, 1, len, f) == len) ++n;
            std::fclose(f);
        }
        return n;
    }

    std::vector<std::string> list_ranges() const {
        return list_in(ranges_dir(), ".rng");
    }
    long long range_size(const std::string& name) const {
        std::error_code ec;
        const auto n = std::filesystem::file_size(range_path(name), ec);
        return ec ? -1 : static_cast<long long>(n);
    }
    bool delete_range(const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        std::error_code ec;
        if (!std::filesystem::remove(range_path(name), ec) || ec) {
            e = "'" + name + M("' no existe", "' does not exist");
            return false;
        }
        return true;
    }
    // Un rango guardado son LOS DOS, OOP e IP, en el mismo fichero.
    //
    // Un rango no dice nada solo: "BU vs BB 25bb" es un par, y guardar un lado
    // suelto obliga a acordarse de cual era el otro. Se guardan juntos y se
    // cargan juntos, que es como se usan.
    //
    // Los ficheros de antes llevaban un solo rango y ninguna etiqueta. Se
    // siguen leyendo: ver load_range.
    bool save_range(const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        const std::string oop = trim(range_spec_[0]), ip = trim(range_spec_[1]);
        if (oop.empty() || ip.empty()) {
            e = std::string(M("un rango guardado son los dos, y ",
                              "a saved range is the pair, and ")) +
                (oop.empty() ? "OOP" : "IP") +
                M(" todavía no tiene ninguno", " has none yet");
            return false;
        }
        std::error_code ec;
        std::filesystem::create_directories(ranges_dir(), ec);
        std::FILE* f = std::fopen(range_path(name).c_str(), "wb");
        if (!f) { e = M("no se pudo escribir ", "cannot write ") + range_path(name); return false; }
        const std::string t = "OOP " + oop + "\nIP " + ip + "\n";
        const bool ok = std::fwrite(t.data(), 1, t.size(), f) == t.size();
        std::fclose(f);
        if (!ok) { e = M("no se pudo escribir", "write failed"); return false; }
        return true;
    }
    // `player` solo se mira en los ficheros de antes, que llevan un rango suelto
    // y no dicen de quien es: ahi se carga en el lado que se pida. Los de ahora
    // llevan los dos con su etiqueta y se cargan los dos.
    bool load_range(const std::string& name, int player, std::string& e) {
        if (!valid_name(name, e)) return false;
        if (player < 0 || player > 1) { e = M("el jugador es OOP o IP", "player must be OOP or IP"); return false; }
        std::FILE* f = std::fopen(range_path(name).c_str(), "rb");
        if (!f) { e = "'" + name + M("' no existe", "' does not exist"); return false; }
        std::string t;
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) t.append(buf, n);
        std::fclose(f);
        std::string oop, ip;
        size_t i = 0;
        while (i < t.size()) {
            size_t f2 = t.find('\n', i);
            if (f2 == std::string::npos) f2 = t.size();
            const std::string ln = trim(t.substr(i, f2 - i));
            i = f2 + 1;
            if (ln.empty()) continue;
            if (upper(ln.substr(0, 4)) == "OOP ")     oop = trim(ln.substr(4));
            else if (upper(ln.substr(0, 3)) == "IP ") ip  = trim(ln.substr(3));
        }
        // Un rango guardado con un board y cargado con otro puede quedarse sin
        // combos: set_range lo dira, y decirlo es mejor que cargar la nada.
        if (oop.empty() && ip.empty()) return set_range(player, trim(t), e);
        // Los dos, o ninguno: dejar uno puesto y el otro no seria peor que
        // fallar, porque la pantalla ensenaria una pareja que nadie guardo.
        const std::string back0 = range_spec_[0], back1 = range_spec_[1];
        if (!set_range(0, oop, e)) return false;
        if (!set_range(1, ip, e)) {
            std::string ig;
            set_range(0, back0, ig);
            (void)back1;
            return false;
        }
        return true;
    }
    bool save_config(const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        std::error_code ec;
        std::filesystem::create_directories(saves_dir(false), ec);
        std::FILE* f = std::fopen(save_path(false, name).c_str(), "wb");
        if (!f) { e = M("no se pudo escribir ", "cannot write ") + save_path(false, name); return false; }
        const std::string t = config_text();
        const bool ok = std::fwrite(t.data(), 1, t.size(), f) == t.size();
        std::fclose(f);
        if (!ok) { e = M("no se pudo escribir", "write failed"); return false; }
        return true;
    }

    bool load_config(const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        std::FILE* f = std::fopen(save_path(false, name).c_str(), "rb");
        if (!f) { e = "'" + name + M("' no existe", "' does not exist"); return false; }
        std::string t;
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) t.append(buf, n);
        std::fclose(f);
        return load_config_text(t, e);
    }

    // El tamano EXACTO que va a tener el fichero del arbol: la cabecera, la
    // config que lleva dentro, el estado del solver y el checksum del final.
    long long tree_file_bytes() const {
        if (!S_ || !solved_) return 0;
        return 8 +                                   // SOLVTREE
               2 * static_cast<long long>(sizeof(int)) +   // version y largo
               static_cast<long long>(config_text().size()) +
               S_->state_bytes() +
               static_cast<long long>(sizeof(unsigned long long));
    }
    // Lo que queda libre en el disco donde se guarda. -1 si no se puede saber,
    // y entonces no se estorba a nadie: se intenta guardar y ya.
    static long long free_bytes_for_saves() {
        std::error_code ec;
        std::filesystem::create_directories(saves_dir(true), ec);
        const std::filesystem::space_info si = std::filesystem::space(saves_dir(true), ec);
        if (ec) return -1;
        return static_cast<long long>(si.available);
    }
    // Y la decision, aparte para poder probarla con numeros inventados. El
    // margen no es un capricho: dejar el disco a cero no rompe solo al solver,
    // rompe a Windows, y de eso no se sale desde aqui.
    static bool fits_on_disk(long long need, long long free_bytes) {
        if (free_bytes < 0 || need <= 0) return true;
        return free_bytes - need >= 256LL * 1024 * 1024;
    }

    // The magic and version are checked before anything is touched, so a wrong
    // or truncated file cannot leave a half-loaded session behind.
    //
    // `libres_forzados` es para poder probar el aviso de "no cabe" sin llenar
    // un disco de verdad, que es la unica forma de provocarlo: -2, que es lo
    // que usa todo el mundo, mira el disco.
    bool save_tree(const std::string& name, std::string& e,
                   long long libres_forzados = -2) {
        if (!valid_name(name, e)) return false;
        if (!S_ || !solved_) { e = M("todavía no hay nada resuelto", "nothing solved yet"); return false; }
        // Cabe, o no se empieza. Un arbol resuelto son gigas; el fichero a
        // medias ya se limpia mas abajo, pero el disco lleno no lo arregla
        // nadie desde aqui, y el aviso despues de escribir dos gigas llega
        // tarde. Se sabe antes porque el tamano se sabe exacto.
        const long long pide = tree_file_bytes();
        const long long libres = (libres_forzados >= -1) ? libres_forzados
                                                         : free_bytes_for_saves();
        if (!fits_on_disk(pide, libres)) {
            char b[256];
            std::snprintf(b, sizeof b,
                          M("este árbol ocupa %.1f GB y en el disco quedan %.1f GB: "
                            "no cabe. Haz sitio o borra algún árbol guardado",
                            "this tree takes %.1f GB and the disk has %.1f GB left: it "
                            "does not fit. Make room, or delete a saved tree"),
                          static_cast<double>(pide) / 1073741824.0,
                          static_cast<double>(libres) / 1073741824.0);
            e = b;
            return false;
        }
        std::error_code ec;
        std::filesystem::create_directories(saves_dir(true), ec);
        const std::string path = save_path(true, name);
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) { e = M("no se pudo escribir ", "cannot write ") + path; return false; }
        const char magic[8] = { 'S','O','L','V','T','R','E','E' };
        const int  version  = TREE_FORMAT;
        const std::string cfgtxt = config_text();
        const int  clen = static_cast<int>(cfgtxt.size());
        bool ok = std::fwrite(magic, 1, 8, f) == 8 &&
                  std::fwrite(&version, sizeof version, 1, f) == 1 &&
                  std::fwrite(&clen, sizeof clen, 1, f) == 1 &&
                  std::fwrite(cfgtxt.data(), 1, cfgtxt.size(), f) == cfgtxt.size();
        if (ok) ok = S_->save_state(f);
        if (ok) {
            // Trailing checksum, so a file that was cut short mid-write is
            // rejected on load instead of quietly restoring half a solve.
            const unsigned long long h = S_->state_hash();
            ok = std::fwrite(&h, sizeof h, 1, f) == 1;
        }
        if (ok) ok = std::fflush(f) == 0;
        std::fclose(f);
        if (!ok) {
            std::filesystem::remove(path, ec);
            e = M("no se pudo escribir -- ¿queda sitio en el disco?", "write failed -- out of disk?");
            return false;
        }
        return true;
    }

    bool load_tree(const std::string& name, std::string& e) {
        if (!valid_name(name, e)) return false;
        const std::string path = save_path(true, name);
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) { e = "'" + name + M("' no existe", "' does not exist"); return false; }
        char magic[8] = { 0 };
        int version = 0, clen = 0;
        if (std::fread(magic, 1, 8, f) != 8 || std::memcmp(magic, "SOLVTREE", 8) != 0) {
            std::fclose(f); e = M("eso no es un árbol guardado", "that is not a saved tree"); return false;
        }
        if (std::fread(&version, sizeof version, 1, f) != 1 ||
            version < 1 || version > TREE_FORMAT) {
            std::fclose(f);
            e = M("lo guardó otra versión del solver", "saved by a different version of the solver");
            return false;
        }
        if (std::fread(&clen, sizeof clen, 1, f) != 1 || clen < 0 || clen > (1 << 20)) {
            std::fclose(f); e = M("la cabecera está corrupta", "corrupt header"); return false;
        }
        std::string cfgtxt(static_cast<size_t>(clen), '\0');
        if (clen && std::fread(&cfgtxt[0], 1, static_cast<size_t>(clen), f)
                    != static_cast<size_t>(clen)) {
            std::fclose(f); e = M("la cabecera está corrupta", "corrupt header"); return false;
        }
        if (!load_config_text(cfgtxt, e)) { std::fclose(f); return false; }
        if (!build_solver(nullptr)) {
            std::fclose(f);
            e = build_error_.empty() ? "could not build the tree for this file" : build_error_;
            return false;
        }
        bool ok = S_->load_state(f, e);
        if (ok && version >= 2) {
            unsigned long long want = 0;
            if (std::fread(&want, sizeof want, 1, f) != 1) {
                e = M("al fichero le falta su suma de control: se quedó a medias", "the file is missing its checksum -- it was cut short");
                ok = false;
            } else if (want != S_->state_hash()) {
                e = M("la suma de control no cuadra: el fichero está corrupto", "checksum mismatch -- the file is corrupt");
                ok = false;
            }
        }
        std::fclose(f);
        if (!ok) { S_.reset(); solved_ = false; return false; }
        solved_ = true;
        return true;
    }

    // Collapsing suits is only legal while nothing in the spot tells the suits
    // apart. Ranges were checked from the start; locks were not, and a lock is
    // just as capable of breaking it. `lock AsKs B=1` pins one spade combo and
    // leaves its heart twin free, so the runouts that are supposed to be the
    // same solve stop being the same solve -- measured on a flop, it moved the
    // root betting frequency by two points and doubled the exploitability.
    //
    // Bucket selectors (NUTS, AIR, ...) are not a risk: they come out of
    // equity, and equity on a symmetric range is symmetric. It is the ones that
    // name cards that can break it.
    // Adding or removing a lock can flip the verdict, and the verdict is made
    // at rebuild time -- so a lock added after the tree was built has to ask
    // for the tree again. Only when the answer actually changes: a rebuild
    // throws the solve away.
    // A lock that names cards makes the suits non-interchangeable, so the tree
    // has to be rebuilt without suit collapsing. That rebuild throws the solver
    // away -- which used to happen the instant the first such lock was set, and
    // took the strategy on screen with it. The second edit then had nothing to
    // read, which is exactly the flow nodelocking exists for: take the calls off
    // this group, make that one bet more, THEN solve.
    //
    // So it waits for the solve. Between edits the tree is whatever it was, the
    // reported size says so, and `iso_off_by_lock()` says what the next solve
    // will do about it.
    void resync_iso() {
        if (!iso_resync_pending()) return;
        std::string ignored;
        rebuild(ignored);
    }
    // Not a boolean any more: a lock does not switch collapsing off, it cuts
    // the group down to the part that survives it, and "would rebuilding change
    // what is collapsed?" is a question about that group and not about a flag.
    bool iso_resync_pending() const {
        std::vector<std::vector<double>> m;
        lock_masks(m);
        return deal_.group_for(range_[0], range_[1], m.empty() ? nullptr : &m,
                               cfg::ISO) != deal_.use_group;
    }

    // One mask per lock, not one mask for all of them. `lock AsKs to bet` and
    // `lock AhKh to check` have a union the swap (s h) maps onto itself while
    // swapping a bet for a check, so a merged mask would call that symmetric.
    void lock_masks(std::vector<std::vector<double>>& out) const {
        out.clear();
        if (locks_.empty() || deal_.num() == 0) return;
        for (const LockSpec& L : locks_) {
            if (L.board != board_spec_) continue;
            std::vector<double> w;
            std::string ignored;
            if (!parse_range(L.hand_spec, deal_, w, ignored)) continue;
            bool any = false;
            for (int h = 0; h < deal_.num(); ++h) {
                const bool on = w[static_cast<size_t>(h)] > 0.0;
                w[static_cast<size_t>(h)] = on ? 1.0 : 0.0;
                any = any || on;
            }
            if (any) out.push_back(w);

            // Y si el lock nombra un runout, el grupo tampoco puede mover ESAS
            // CARTAS. Un indicador sobre "los combos que contienen la carta c"
            // solo lo conserva una permutacion que mande c a c: si mandara c a
            // otra carta, el conjunto cambiaria. Asi que la restriccion de
            // carta se expresa con la misma maquinaria de siempre y no hace
            // falta tocar el grupo.
            //
            // Sin esto, bloquear el 3s bloquearia tambien el 3h en cuanto los
            // dos cayeran en la misma orbita, que es justo lo que este cambio
            // viene a evitar.
            std::vector<int> sl;
            if (!L.runout.empty() && runout_slots(L.runout, sl)) {
                for (int s2 : sl) {
                    const int c = deal_.deck[static_cast<size_t>(s2)];
                    std::vector<double> m(static_cast<size_t>(deal_.num()), 0.0);
                    bool hay = false;
                    for (int h : deal_.with_card[static_cast<size_t>(c)]) {
                        m[static_cast<size_t>(h)] = 1.0;
                        hay = true;
                    }
                    if (hay) out.push_back(m);
                }
            }
        }
    }

    // Kept for the callers that only want the yes/no: does every lock survive
    // the whole board group?
    bool locks_suit_symmetric() const {
        std::vector<std::vector<double>> m;
        lock_masks(m);
        for (const std::vector<double>& w : m)
            if (!deal_.range_symmetric(deal_.base_group, w)) return false;
        return true;
    }

    // ---- tree ------------------------------------------------------------
    bool rebuild(std::string& e) {
        std::vector<std::vector<double>> lm;
        lock_masks(lm);
        deal_.build_orbits(range_[0], range_[1], cfg::ISO,
                           lm.empty() ? nullptr : &lm);
        // What the locks cost, as opposed to what the ranges cost: the group
        // the ranges alone would have allowed, against the one actually in use.
        iso_off_by_lock_ =
            cfg::ISO && !lm.empty() &&
            deal_.use_group.size() <
                deal_.group_for(range_[0], range_[1], nullptr, cfg::ISO).size();

        // El arbol se arma aparte y solo entra si sale bien, y la solucion no se
        // tira hasta ese momento.
        //
        // MEDIDO: `set bets 10,20,30,40,50,60,70,80,90,100,150,200` -- doce
        // tamanos donde caben ocho acciones -- se rechaza, deja el arbol como
        // estaba... y antes se llevaba por delante el solve que hubiera hecho.
        // Veinte iteraciones de river se convertian en "no solution yet -- run
        // `solve` first" por un campo mal escrito que el programa no llego a
        // aplicar. En un flop de diez minutos eso son diez minutos.
        //
        // Se puede quedar con el solver porque lo unico que mira TreeBuilder es
        // tc_ y la calle de salida: si falla, lo que agrupa los repartos -- los
        // rangos, el agrupado de palos, los locks -- no ha cambiado, asi que los
        // orbitos que acaba de rehacer son los mismos de antes.
        TreeBuilder tb(tc_, deal_);
        GameTree g = tb.build();
        if (!tb.ok()) { e = tb.error(); return false; }
        S_.reset();
        solved_ = false;
        tree_ = g;
        // Memory is laid out over the combos each player can actually hold, so
        // the reported size has to be laid out the same way. A player with no
        // range yet counts as the whole board, so the size shown before you
        // type anything is an upper bound rather than a meaningless zero.
        {
            int nl[2];
            for (int p = 0; p < 2; ++p) {
                nl[p] = live_combos(p);
                if (nl[p] == 0) nl[p] = deal_.num();
            }
            tree_.layout(nl[0], nl[1]);
        }

        cur_ctx_ = 0;
        cur_node_ = tree_.ctx[0].tree.root;
        slots_.clear();

        dropped_locks_ = 0;
        std::vector<LockSpec> keep;
        for (const LockSpec& L : locks_) {
            const int c = (L.board == board_spec_) ? tree_.find_ctx(L.ctx_label) : -1;
            if (c >= 0 && tree_.find_node(c, L.node_path) >= 0) keep.push_back(L);
            else ++dropped_locks_;
        }
        locks_.swap(keep);
        return true;
    }

    // ---- navigation ------------------------------------------------------
    void go_root() { cur_ctx_ = 0; cur_node_ = tree_.ctx[0].tree.root; slots_.clear(); }

    bool go(const std::string& arg, std::string& e) {
        const std::string a = trim(arg);
        if (a == "/" || lower(a) == "root") { go_root(); return true; }
        if (a == "..") return go_up(e);

        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(cur_ctx_)];
        const Node& n = node();

        // At a chance node the argument is a card to deal.
        if (n.type == NT_CONT) {
            const int c = parse_card(a);
            if (c < 0) { e = M("en un nodo de azar hay que decir qué carta se reparte (por ejemplo `cd Ts`)", "at a chance node, name a card to deal (e.g. `cd Ts`)"); return false; }
            int slot = -1;
            for (size_t i = 0; i < deal_.deck.size(); ++i)
                if (deal_.deck[i] == c) { slot = static_cast<int>(i); break; }
            if (slot < 0) { e = card_str(c) + " is on the board already"; return false; }
            for (int s : slots_)
                if (s == slot) { e = card_str(c) + " has already been dealt"; return false; }
            const int child = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
            if (child < 0) { e = M("no hay calle después de esta", "no street after this one"); return false; }
            slots_.push_back(slot);
            cur_ctx_ = child;
            cur_node_ = tree_.ctx[static_cast<size_t>(child)].tree.root;
            return true;
        }

        if (n.type != NT_DECISION) { e = M("este nodo es un final de mano", "this is a terminal node"); return false; }
        for (int i = 0; i < n.num_actions; ++i)
            if (lower(rc.tree.act(n, i).code) == lower(a)) {
                cur_node_ = rc.tree.child(n, i);
                return true;
            }
        e = M("aquí no hay ninguna acción '", "no action '") + a +
            M("'", "' here");
        return false;
    }

    bool go_up(std::string& e) {
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(cur_ctx_)];
        if (cur_node_ != rc.tree.root) {
            const int p = parent_in_round(rc.tree, cur_node_);
            if (p < 0) { e = M("no se puede subir", "cannot go up"); return false; }
            cur_node_ = p;
            return true;
        }
        if (rc.parent < 0) { e = M("ya estás en la raíz", "already at the root"); return false; }
        const RoundCtx& pr = tree_.ctx[static_cast<size_t>(rc.parent)];
        int cont_node = -1;
        for (size_t i = 0; i < pr.tree.nodes.size(); ++i)
            if (pr.tree.nodes[i].type == NT_CONT && pr.tree.nodes[i].cont_id == rc.parent_cont) {
                cont_node = static_cast<int>(i);
                break;
            }
        if (cont_node < 0) { e = M("no se puede subir", "cannot go up"); return false; }
        cur_ctx_ = rc.parent;
        cur_node_ = cont_node;
        if (!slots_.empty()) slots_.pop_back();
        return true;
    }

    static int parent_in_round(const BetTree& bt, int nid) {
        for (size_t i = 0; i < bt.nodes.size(); ++i) {
            const Node& n = bt.nodes[i];
            if (n.type != NT_DECISION) continue;
            for (int a = 0; a < n.num_actions; ++a)
                if (bt.child(n, a) == nid) return static_cast<int>(i);
        }
        return -1;
    }

    // ---- solving ---------------------------------------------------------
    // Builds a fresh solver and applies every recorded lock to it.
    bool build_solver(std::vector<std::string>* notes) {
        // Now, not when the lock was set: see resync_iso().
        if (iso_resync_pending()) {
            resync_iso();
            if (notes) notes->push_back(iso_off_by_lock_
                ? (deal_.iso_on
                   ? M("el agrupado de palos se recorta: hay un lock que nombra "
                       "cartas, así que solo se agrupan los cambios de palo que "
                       "sobreviven a ese lock",
                       "suit collapsing cut down: a lock names cards, so only the "
                       "suit swaps it survives are still collapsed")
                   : M("agrupado de palos apagado: hay un lock que nombra cartas, "
                       "así que los palos ya no son intercambiables",
                       "suit collapsing off: a lock names cards, so the suits are "
                       "no longer interchangeable"))
                : M("agrupado de palos otra vez encendido: ya no hay ningún lock "
                    "que nombre cartas",
                    "suit collapsing back on: no lock names cards any more"));
        }
        const MemUse mu = memory_use();
        const double gb = mu.total;
        if (gb > cfg::MAX_MEM_GB) {
            char m[420];
            // Which knob to turn depends on where the memory actually is, and
            // "shrink the sizings" is no help when the sizings are not the
            // problem. The buffers are the tree; everything else is the board
            // and the ranges, and no amount of trimming bets will touch it.
            const double rest = gb - mu.buffers;
            const char* advice =
                (mu.buffers > 0.6 * gb)
                    ? M("eso es el árbol de apuestas: usa menos tamaños, o quita las "
                        "subidas de alguna calle",
                        "that is the betting tree: use fewer sizings, or drop the "
                        "raises on a street with `set raises <street> none`")
                    : M("eso es sobre todo el board y los rangos, no el arbol de "
                        "apuestas: recortar tamaños apenas lo mueve, y un rango mas "
                        "estrecho o empezar una calle mas tarde si",
                        "that is mostly the board and the ranges, not the betting tree: "
                        "trimming sizings will barely move it, and a narrower range or a "
                        "later street will");
            std::snprintf(m, sizeof m,
                M("este spot necesita %.2f GB y el límite son %.2f (%.2f el árbol de "
                  "apuestas, %.2f todo lo demas) -- %s, o sube el límite de memoria "
                  "(el engranaje, en Avanzado, o `set maxmem` en la consola)",
                  "this spot needs %.2f GB and the limit is %.2f (%.2f betting tree, "
                  "%.2f everything else) -- %s, or raise the memory limit (the gear, "
                  "under Avanzado, or `set maxmem` in the console)"),
                gb, cfg::MAX_MEM_GB, mu.buffers, rest, advice);
            if (notes) notes->push_back(m);
            build_error_ = m;
            S_.reset();
            return false;
        }
        try {
            S_ = std::unique_ptr<DCFRSolver>(new DCFRSolver(tree_, deal_, range_[0], range_[1]));
        } catch (const std::bad_alloc&) {
            build_error_ = M("se acabó la memoria montando el solver de este árbol",
                             "out of memory building the solver for this tree");
            if (notes) notes->push_back(build_error_);
            S_.reset();
            return false;
        }
        build_error_.clear();
        // Borrar antes de poner. Un lock que ya no esta en la lista pudo dejar
        // su bloque en el nodo -- al soltarse por un rebuild, por ejemplo -- y
        // ese resto seguiria congelando manos que nadie ha pedido congelar.
        for (size_t c = 0; c < tree_.ctx.size(); ++c) {
            BetTree& bt = tree_.ctx[c].tree;
            for (size_t k = 0; k < bt.nodes.size(); ++k)
                if (bt.nodes[k].is_locked) S_->clear_locks(static_cast<int>(c),
                                                           static_cast<int>(k));
        }
        int applied = 0;
        for (const LockSpec& L : locks_) {
            const int c = tree_.find_ctx(L.ctx_label);
            const int n = (c >= 0) ? tree_.find_node(c, L.node_path) : -1;
            if (c < 0 || n < 0) continue;
            std::vector<int> hands;
            std::string e;
            const int owner = tree_.ctx[static_cast<size_t>(c)].tree.nodes[static_cast<size_t>(n)].player;
            if (!resolve_hands(L.hand_spec, owner, hands, e)) {
                if (notes) notes->push_back(M("lock en ", "lock at ") + L.ctx_label + " " +
                                        L.node_path + ": " + e);
                continue;
            }
            // En que runout va este lock. Vacio = en todos, que es lo que
            // significaban los locks antes de que existiera este campo.
            long long inst = -1;
            if (!L.runout.empty()) {
                std::vector<int> sl;
                int perm = deal_.identity();
                if (!runout_slots(L.runout, sl) || !deal_.locate(sl, inst, perm)) {
                    if (notes) notes->push_back(M("lock en ", "lock at ") + L.node_path +
                                                M(": el reparto ", ": the runout ") +
                                                L.runout + " is not on this board any more");
                    continue;
                }
                // Con los palos colapsados, el runout guardado puede ser un
                // representante visto desde otro marco. Las manos hay que
                // llevarlas a ESE marco o el lock cae en combos distintos de
                // los que el usuario eligio.
                if (perm != deal_.identity()) {
                    const std::vector<int>& back = deal_.perm_combo[
                        static_cast<size_t>(deal_.perm_inv[perm])];
                    for (int& h : hands) h = back[static_cast<size_t>(h)];
                }
            }
            if (S_->lock_hands_coded(c, n, inst, hands, L.mix)) ++applied;
            else if (notes) notes->push_back(M("lock en ", "lock at ") + L.node_path +
                                             M(": esa acción no existe ahí",
                                               ": action not available there"));
        }
        if (applied && notes)
            notes->push_back(std::to_string(applied) + M(" nodelock(s) aplicados",
                                                         " nodelock(s) applied"));
        // Ya estan dentro: lo que se vea a partir de ahora si los lleva.
        locks_pending_ = false;
        return true;
    }

    void solve(int iterations, int report_every, std::vector<std::string>* notes = nullptr,
               std::atomic<int>* progress = nullptr, std::atomic<bool>* cancel = nullptr) {
        if (!build_solver(notes)) return;
        acc_hit_.store(false);
        timeout_hit_.store(false);
        const bool para_por_precision = (acc_stop_ && acc_target_ > 0.0);
        if (!para_por_precision && timeout_secs_ <= 0.0) {
            S_->run(iterations, report_every, progress, cancel);
        } else {
            // A trozos, para poder mirar la precision sin pagarla cada vuelta:
            // medirla cuesta un recorrido entero del arbol por cada jugador.
            //
            // Esto tiene que estar aqui y no solo en el worker asincrono. El
            // camino sincrono es el de la consola y el de --script, y un ajuste
            // que solo funciona en la interfaz es un ajuste que no funciona: se
            // pone, no protesta, y no hace nada.
            using clock = std::chrono::steady_clock;
            // El trozo con el que se avanza NO es la cadencia de los avisos.
            // Atarlos era lo que hacia que un objetivo flojo costara diez veces
            // mas de lo que pide: la consola llama con report_every = n/10, o
            // sea que pidiendo `solve 20000` la precision no se miraba hasta la
            // iteracion 2000 -- y estaba alcanzada en la 200. Medido: objetivo
            // del 1% del bote, 2000 iteraciones y 79 segundos para acabar en el
            // 0,003%, trescientas veces mas fino de lo que se pidio.
            //
            // Sesenta y cuatro es barato: lo que cuesta dinero es MEDIR la
            // explotabilidad, y de eso ya se encarga el reloj de aqui abajo,
            // que no la mide mas de una vez cada segundo y medio.
            // Con tope de tiempo, la PRIMERA tanda tambien tiene que ser corta:
            // empezando por 64 en un flop grande se van cinco segundos antes de
            // mirar el reloj por primera vez, y un tope de tres no puede
            // cumplirse asi. Cuatro cuesta un tercio de segundo, y desde la
            // segunda tanda ya se ajusta sola a lo que mida la maquina.
            int step = (timeout_secs_ > 0.0) ? 4 : 64;
            const clock::time_point t0 = clock::now();
            clock::time_point next = clock::now();
            for (int done = 0; done < iterations; ) {
                const int c = std::min(step, iterations - done);
                const clock::time_point c0 = clock::now();
                S_->run(c, report_every, progress, cancel);
                done += c;
                // El trozo se ajusta al reloj: en un flop grande una iteracion
                // cuesta un cuarto de segundo, o sea que un trozo fijo de 64 mira
                // la hora cada diecisiete. MEDIDO: con tope de 25s paraba a los 35.
                // Ahora el trozo dura como mucho dos segundos.
                if (timeout_secs_ > 0.0) {
                    const double por =
                        std::chrono::duration<double>(clock::now() - c0).count() /
                        std::max(1, c);
                    step = (por > 1e-9)
                         ? std::max(1, std::min(64, static_cast<int>(2.0 / por)))
                         : 64;
                }
                if (cancel && cancel->load()) break;
                // El reloj se mira SIEMPRE, aunque no haya objetivo de precision:
                // es el tope que decide cuando hay veinte boards esperando.
                if (timeout_secs_ > 0.0 &&
                    std::chrono::duration<double>(clock::now() - t0).count() >= timeout_secs_) {
                    timeout_hit_.store(true);
                    break;
                }
                if (para_por_precision && clock::now() >= next) {
                    const clock::time_point e0 = clock::now();
                    expl_.store(S_->exploitability());
                    const double cost =
                        std::chrono::duration<double>(clock::now() - e0).count();
                    next = clock::now() + std::chrono::milliseconds(
                               std::max(1500, static_cast<int>(cost * 20000.0)));
                    if (accuracy_met()) { acc_hit_.store(true); break; }
                }
            }
        }
        solved_ = true;
        expl_.store(S_->exploitability());
    }

    bool iterate(int iterations, int report_every) {
        if (!S_) return false;
        S_->run(iterations, report_every);
        solved_ = true;
        return true;
    }

    // ---- solving in the background --------------------------------------
    //  The web UI cannot block its single request thread for minutes, so a
    //  solve runs on its own thread while progress is polled and a stop flag
    //  can be raised. Callers must treat the session as untouchable while
    //  busy() is true: the worker owns the solver during that window.
    bool solve_async(int iterations) {
        // Every worker lifecycle operation is serialised: the web server now
        // handles connections on their own threads, and two of them calling
        // join() on the same std::thread is undefined behaviour.
        std::lock_guard<std::mutex> life(life_mtx_);
        if (running_.load()) return false;
        if (!ranges_ready()) return false;
        if (worker_.joinable()) worker_.join();
        cancel_.store(false);
        prog_.store(0);
        expl_.store(-1.0);
        prog_total_   = iterations;
        prog_seconds_ = 0.0;
        async_notes_.clear();
        started_ = std::chrono::steady_clock::now();
        running_.store(true);
        worker_ = std::thread([this, iterations]() { this->worker_body(iterations, true); });
        return true;
    }

    // The same thing, but adding iterations to the solve that is already there
    // instead of throwing it away. Pressing Solve builds a fresh solver, which
    // is right when something about the spot changed and wrong when you have
    // just watched 109 iterations of 5000 go by and want the other 4891.
    bool solve_more_async(int iterations) {
        std::lock_guard<std::mutex> life(life_mtx_);
        if (running_.load()) return false;
        if (!S_ || !solved_) return false;
        if (worker_.joinable()) worker_.join();
        cancel_.store(false);
        prog_.store(0);
        expl_.store(-1.0);
        prog_total_   = iterations;
        prog_seconds_ = 0.0;
        async_notes_.clear();
        started_ = std::chrono::steady_clock::now();
        running_.store(true);
        worker_ = std::thread([this, iterations]() { this->worker_body(iterations, false); });
        return true;
    }

    // Reading the solver while a solve runs goes through these. A plain mutex
    // is not enough: the worker re-locks the instant it lets go, and nothing
    // stops it winning that race every time, so a reader can starve forever.
    // Readers announce themselves first and the worker stands aside for them.
    void read_lock() {
        // Stamped so the worker knows somebody is looking. A chunk is sized for
        // throughput, and holding the solver for a third of a second is fine
        // when nobody is watching and awful when somebody just clicked: the
        // click sits there doing nothing for as long as the chunk lasts.
        last_read_.store(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        readers_.fetch_add(1);
        mtx_.lock();
        readers_.fetch_sub(1);
    }
    void read_unlock() { mtx_.unlock(); }
    double exploitability() const { return expl_.load(); }
    void request_stop() { cancel_.store(true); }
    bool busy() const { return running_.load(); }
    // Joins a worker that has already finished, so the solver is safe to touch.
    void reap() {
        std::lock_guard<std::mutex> life(life_mtx_);
        if (!running_.load() && worker_.joinable()) worker_.join();
    }

    int    progress_done()  const { return prog_.load(); }
    int    progress_total() const { return prog_total_; }
    bool   was_stopped()    const { return cancel_.load(); }
    double progress_seconds() const {
        if (running_.load())
            return std::chrono::duration<double>(
                std::chrono::steady_clock::now() - started_).count();
        return prog_seconds_;
    }
    const std::vector<std::string>& async_notes() const { return async_notes_; }

    ~Session() {
        cancel_.store(true);
        std::lock_guard<std::mutex> life(life_mtx_);
        if (worker_.joinable()) worker_.join();
    }

    // ---- locks -----------------------------------------------------------
    // El board TAL COMO ESTA en el nodo donde uno esta: el de partida mas las
    // cartas repartidas por el camino. Clasificar una mano con el flop cuando
    // se esta en el river la clasifica mal, y de eso vive todo lo de abajo.
    std::vector<int> board_with(const std::vector<int>& slots) const {
        std::vector<int> bd = deal_.board;
        for (int s : slots)
            if (s >= 0 && s < static_cast<int>(deal_.deck.size()))
                bd.push_back(deal_.deck[static_cast<size_t>(s)]);
        return bd;
    }
    std::vector<int> board_now() const { return board_with(slots_); }

    // Una FAMILIA de las de la referencia como selector de lock: `lock top_pair B=90%`.
    //
    // Es lo que pidio el usuario para las barras, y de paso la consola lo gana
    // igual. Los nombres son los mismos que se ensenan en el reparto por
    // categorias -- los 18 de mano hecha y los 5 de proyecto -- porque si en
    // pantalla pone `two_pair` y hay que escribir otra cosa, el nombre no sirve.
    //
    // Devuelve kind (0 mano hecha, 1 proyecto) y el indice dentro de su tabla.
    static bool family_selector(const std::string& spec, int& kind, int& cls) {
        const std::string s = lower(trim(spec));
        for (int i = 0; i < MC_COUNT; ++i)
            if (s == lower(MC_NAME[i])) { kind = 0; cls = i; return true; }
        for (int i = 0; i < DR_COUNT; ++i)
            if (s == lower(DR_NAME[i])) { kind = 1; cls = i; return true; }
        return false;
    }

    bool resolve_hands(const std::string& spec, int player, std::vector<int>& out,
                       std::string& e) const {
        return resolve_hands_on(spec, player, board_now(), out, e);
    }
    // `board` es el del NODO, no el de partida: una familia se mira con las
    // cartas que hay en la mesa ahi.
    bool resolve_hands_on(const std::string& spec, int player,
                          const std::vector<int>& board,
                          std::vector<int>& out, std::string& e) const {
        out.clear();
        int kind = -1, cls = -1;
        if (family_selector(spec, kind, cls)) {
            for (int h = 0; h < deal_.num(); ++h) {
                if (range_[player][static_cast<size_t>(h)] <= 0.0) continue;
                const Combo& k = deal_.combos[static_cast<size_t>(h)];
                const int idx = (kind == 1) ? draw_cat(k.c1, k.c2, board)
                                            : made_cat(k.c1, k.c2, board);
                if (idx == cls) out.push_back(h);
            }
            if (out.empty())
                e = "'" + spec + M("' no encaja con ninguna mano que ese jugador tenga aquí",
                                   "' does not match a single hand that player has here");
            return !out.empty();
        }
        std::vector<double> w;
        if (!parse_range(spec, deal_, w, e)) return false;
        for (int h = 0; h < deal_.num(); ++h)
            if (w[static_cast<size_t>(h)] > 0.0 && range_[player][static_cast<size_t>(h)] > 0.0)
                out.push_back(h);
        if (out.empty()) e = "'" + spec +
            M("' no encontro nada dentro del rango de ese jugador",
              "' matched nothing inside that player's range");
        return !out.empty();
    }

    // La consola bloquea donde tiene el cursor; la interfaz web manda la
    // direccion en cada peticion y NO toca la navegacion del servidor. Si el
    // runout saliera de slots_, desde el navegador se bloquearian siempre todos
    // los runouts -- justo lo que este cambio viene a arreglar -- y encima
    // dependeria de por donde anduviera la ultima consola.
    bool add_lock(int ci, int nid, const std::string& hand_spec,
                  const std::vector<std::pair<std::string, double>>& mix,
                  int& matched, std::string& e) {
        return add_lock_at(ci, nid, slots_, hand_spec, mix, matched, e);
    }
    bool add_lock_at(int ci, int nid, const std::vector<int>& slots,
                     const std::string& hand_spec,
                     const std::vector<std::pair<std::string, double>>& mix,
                     int& matched, std::string& e) {
        if (ci < 0 || ci >= static_cast<int>(tree_.ctx.size())) { e = M("no existe ese nodo", "no such node"); return false; }
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) { e = M("no existe ese nodo", "no such node"); return false; }
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) { e = M("ese nodo no es una decisión", "not a decision node"); return false; }
        if (mix.empty()) { e = M("el reparto de acciones está vacío", "empty action mix"); return false; }

        double total = 0.0;
        for (const auto& pr : mix) {
            if (pr.second < 0.0) { e = M("las probabilidades no pueden ser negativas", "probabilities must be >= 0"); return false; }
            total += pr.second;
            if (action_of(rc.tree, n, pr.first) < 0) {
                e = M("este nodo no tiene la acción '", "this node has no '") +
                    pr.first + "'";
                return false;
            }
        }
        if (total <= 1e-12) { e = M("el reparto de acciones suma cero", "action mix sums to zero"); return false; }

        std::vector<int> hands;
        if (!resolve_hands(hand_spec, n.player, hands, e)) return false;
        matched = static_cast<int>(hands.size());

        // Only a lock on the SAME hands is replaced. A node holds as many locks
        // as you put on it, because that is the whole job: you take the calls
        // off one group and make another bet more, at the same node, and the
        // second one used to delete the first without a word.
        const std::string cl = rc.label, np = n.path, ro = runout_of_ctx(rc, slots);
        // Un bucle a mano y no remove_if con lambda.
        //
        // Con `-march=native` y esta cadena de comparaciones, el compilador de
        // aqui (GCC 16.1.0, WinLibs) genera codigo que se va de cabeza en un
        // borrado sobre una lista VACIA -- una operacion que no hace nada.
        // Comprobado con la lista instrumentada: entra con locks_=0 y no sale.
        // Sin -march=native, o a -O2, o con una comparacion menos en la lambda,
        // no pasa. No es logica de aqui, es generacion de codigo, y el bucle
        // hace lo mismo sin darle ocasion.
        const std::string hs = no_space(hand_spec);
        std::vector<LockSpec> quedan;
        quedan.reserve(locks_.size());
        for (size_t i = 0; i < locks_.size(); ++i) {
            const LockSpec& K = locks_[i];
            const bool mismo = K.ctx_label == cl && K.node_path == np &&
                               K.runout == ro && no_space(K.hand_spec) == hs;
            if (!mismo) quedan.push_back(K);
        }
        locks_.swap(quedan);
        LockSpec L;
        L.board     = board_spec_;
        L.ctx_label = cl;
        L.node_path = np;
        L.runout    = ro;
        L.hand_spec = hand_spec;
        L.mix = mix;
        locks_.push_back(L);
        // Y la solucion que hay en pantalla SE QUEDA.
        //
        // Antes esto ponia `solved_ = false`, o sea que el primer lock borraba
        // la estrategia, las barras y todo hasta el siguiente solve. Con eso no
        // se puede mover mas de una barra: hay que resolver entre una y otra, y
        // la segunda edicion se hace a ciegas. Es exactamente lo contrario del
        // flujo para el que existe el nodelock, que este mismo fichero describe
        // unas lineas mas abajo: quitar los pagos a un grupo, hacer que otro
        // apueste mas, y LUEGO resolver.
        //
        // Lo que se ve sigue siendo una solucion de verdad -- la de antes del
        // lock -- y lo que cambia es que hay locks sin aplicar. Eso se dice, que
        // es distinto de borrarlo todo.
        locks_pending_ = true;
        return true;
    }

    // The way nodelocking is actually used: solve, look at what came out, and
    // move it. Retyping the numbers first is not "looking at the strategy", so
    // this reads them back. One lock per combo, because two combos of the same
    // class rarely play the same -- AJo with the heart checks 90% here and the
    // rest check 67% -- and averaging them would quietly invent a strategy
    // neither of them had.
    //
    // With `nudge`, one action moves by `delta` and the rest give up (or take)
    // the difference in proportion to what they already had. That is the actual
    // sentence people say: this group calls less, that group bets more.
    // `slots` son las cartas repartidas del nodo que se esta tocando. Vienen en
    // la peticion, como todo lo demas: el servidor no guarda por donde anda el
    // navegador, y leerlas de su propia navegacion metia el lock en el runout
    // equivocado en cuanto la pagina miraba otro.
    // `which_idx` es la accion EXACTA, por su indice en el nodo; -1 deja que la
    // busque por tipo. Hace falta porque el tipo no basta: un river con dos
    // tamanos tiene dos acciones de apostar, y "la apuesta" no dice cual. Por
    // tipo se quedaba con la ultima, en silencio.
    bool lock_from_current(int ci, int nid, const std::vector<int>& slots,
                           const std::string& hand_spec,
                           bool nudge, ActionKind which, int which_idx,
                           LockMode mode, double w,
                           int& matched, std::string& e) {
        if (ci < 0 || ci >= static_cast<int>(tree_.ctx.size())) { e = M("no existe ese nodo", "no such node"); return false; }
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) { e = M("no existe ese nodo", "no such node"); return false; }
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) { e = M("ese nodo no es una decisión", "not a decision node"); return false; }

        std::vector<int> hands;
        if (!resolve_hands_on(hand_spec, n.player, board_with(slots), hands, e))
            return false;

        // Where the numbers come from, in order. A lock already sitting on this
        // combo wins: once you have frozen a group, "bet twenty more" means
        // twenty more than what you froze, not twenty more than a solve that no
        // longer exists. Naming cards turns suit collapsing off and rebuilds the
        // solver, so without this the second edit in a row was refused -- which
        // is precisely the flow the whole feature is for.
        const int A = rc.tree.nodes[static_cast<size_t>(nid)].num_actions;
        std::vector<double> have(static_cast<size_t>(A) * deal_.num(), 0.0);
        std::vector<unsigned char> known(static_cast<size_t>(deal_.num()), 0u);
        for (const LockSpec& L : locks_) {
            if (L.ctx_label != rc.label || L.node_path != n.path) continue;
            std::vector<int> lh;
            std::string ignored;
            if (!resolve_hands(L.hand_spec, n.player, lh, ignored)) continue;
            for (int h : lh) {
                known[static_cast<size_t>(h)] = 1u;
                for (int a = 0; a < A; ++a) have[static_cast<size_t>(a) * deal_.num() + h] = 0.0;
                double tt = 0.0;
                for (const auto& pr : L.mix) tt += pr.second;
                if (tt <= 1e-12) continue;
                for (const auto& pr : L.mix) {
                    const int a = action_of(rc.tree, n, pr.first);
                    if (a >= 0) have[static_cast<size_t>(a) * deal_.num() + h] += pr.second / tt;
                }
            }
        }

        // Anything not already locked comes from the solve on screen.
        bool need_solver = false;
        for (int h : hands) if (!known[static_cast<size_t>(h)]) need_solver = true;
        NodeStats N;
        if (need_solver) {
            if (!S_ || S_->iterations_done() <= 0) {
                e = M("resuelve primero: todavía no hay estrategia que mirar", "solve first -- there is no strategy to look at yet");
                return false;
            }
            long long inst = 0;
            int perm = deal_.identity();
            deal_.locate(slots, inst, perm);
            N = gather(*S_, ci, nid, inst, perm, true);
            if (!N.ok) { e = M("no se pudo leer la estrategia de ese nodo", "could not read the strategy at that node"); return false; }
        }

        matched = 0;
        for (int h : hands) {
            std::vector<double> pv(static_cast<size_t>(A), 0.0);
            double tot = 0.0;
            if (known[static_cast<size_t>(h)]) {
                for (int a = 0; a < A; ++a) {
                    pv[static_cast<size_t>(a)] = have[static_cast<size_t>(a) * deal_.num() + h];
                    tot += pv[static_cast<size_t>(a)];
                }
            } else {
                const int hs = N.stored(h);
                for (int a = 0; a < A; ++a) {
                    pv[static_cast<size_t>(a)] = N.strat[static_cast<size_t>(a) * N.nh + hs];
                    tot += pv[static_cast<size_t>(a)];
                }
            }
            std::vector<std::pair<std::string, double>> mix;
            for (int a = 0; a < A; ++a)
                if (pv[static_cast<size_t>(a)] > 1e-9)
                    mix.push_back(std::make_pair(rc.tree.act(n, a).code,
                                                 pv[static_cast<size_t>(a)]));
            if (tot <= 1e-9) continue;                  // a combo with no reach here
            if (nudge) {
                // An action currently at zero still has to be reachable, or
                // "bet 20 more" could never start a bet that is not there yet.
                int target = -1;
                if (which_idx >= 0 && which_idx < A) target = which_idx;
                else for (int a = 0; a < A; ++a)
                    if (rc.tree.act(n, a).kind == which) target = a;
                if (target < 0) {
                    e = std::string(M("este nodo no tiene acción de ",
                                      "this node has no ")) + kind_name(which) +
                        M("", " action");
                    return false;
                }
                const double had = pv[static_cast<size_t>(target)];
                double want = (mode == LM_FIXED) ? w
                            : (mode == LM_SCALE) ? had * w
                                                 : had + w;
                if (want < 0.0) want = 0.0;
                if (want > 1.0) want = 1.0;
                const double moved = want - pv[static_cast<size_t>(target)];
                double rest = 0.0;
                for (int a = 0; a < A; ++a) if (a != target) rest += pv[static_cast<size_t>(a)];
                pv[static_cast<size_t>(target)] = want;
                for (int a = 0; a < A; ++a) {
                    if (a == target) continue;
                    // Proportional while there is something to take from; once
                    // the rest is empty the only place to put it is evenly, and
                    // that is a choice worth stating rather than a rounding.
                    pv[static_cast<size_t>(a)] -= (rest > 1e-12)
                        ? moved * pv[static_cast<size_t>(a)] / rest
                        : moved / (A - 1);
                    if (pv[static_cast<size_t>(a)] < 0.0) pv[static_cast<size_t>(a)] = 0.0;
                }
                mix.clear();
                for (int a = 0; a < A; ++a)
                    if (pv[static_cast<size_t>(a)] > 1e-9)
                        mix.push_back(std::make_pair(rc.tree.act(n, a).code,
                                                     pv[static_cast<size_t>(a)]));
            }
            if (mix.empty()) continue;
            int one = 0;
            const Combo& k = deal_.combos[static_cast<size_t>(h)];
            const std::string name = card_str(k.c1) + card_str(k.c2);
            // Con las ranuras DEL NODO, no con las de la sesion: leer la
            // jugada de un runout y escribir el lock en otro es exactamente el
            // fallo que esto evita.
            if (!add_lock_at(ci, nid, slots, name, mix, one, e)) return false;
            ++matched;
        }
        if (matched == 0) { e = M("ninguno de esos combos llega a este nodo", "none of those combos reaches this node"); return false; }
        return true;
    }
    bool add_lock_from_current(int ci, int nid, const std::string& hand_spec,
                               int& matched, std::string& e) {
        return lock_from_current(ci, nid, slots_, hand_spec, false, AK_CHECK, -1,
                                 LM_ADD, 0.0, matched, e);
    }
    bool add_lock_from_current_at(int ci, int nid, const std::vector<int>& slots,
                                  const std::string& hand_spec,
                                  int& matched, std::string& e) {
        return lock_from_current(ci, nid, slots, hand_spec, false, AK_CHECK, -1,
                                 LM_ADD, 0.0, matched, e);
    }
    bool add_lock_moved(int ci, int nid, const std::string& hand_spec,
                        ActionKind which, LockMode mode, double w,
                        int& matched, std::string& e) {
        return lock_from_current(ci, nid, slots_, hand_spec, true, which, -1, mode, w,
                                 matched, e);
    }
    bool add_lock_moved_at(int ci, int nid, const std::vector<int>& slots,
                           const std::string& hand_spec, ActionKind which,
                           int which_idx, LockMode mode, double w,
                           int& matched, std::string& e) {
        return lock_from_current(ci, nid, slots, hand_spec, true, which, which_idx,
                                 mode, w, matched, e);
    }
    bool add_lock_nudged(int ci, int nid, const std::string& hand_spec,
                         ActionKind which, double delta, int& matched, std::string& e) {
        return add_lock_moved(ci, nid, hand_spec, which, LM_ADD, delta, matched, e);
    }

    // Same resolution the locks use, exposed so a check can ask which combos
    // a spec names without duplicating the rules.
    bool resolve_hands_public(const std::string& spec, int player,
                              std::vector<int>& out, std::string& e) const {
        return resolve_hands(spec, player, out, e);
    }
    bool remove_lock(int ci, int nid) { return remove_lock_at(ci, nid, slots_); }
    // Quitar los locks de UNA FAMILIA en este nodo, y dejar los demas donde
    // estan: es el boton de deshacer de cada barra. Sin esto, arrepentirse de
    // haber movido las dobles parejas se llevaba por delante todo lo demas que
    // hubieras tocado en el mismo nodo.
    bool remove_locks_for(int ci, int nid, const std::vector<int>& slots,
                          const std::string& hand_spec, int& quitados) {
        quitados = 0;
        if (ci < 0 || ci >= static_cast<int>(tree_.ctx.size())) return false;
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) return false;
        const Node& n = rc.tree.nodes[static_cast<size_t>(nid)];
        if (n.type != NT_DECISION) return false;
        std::vector<int> hands;
        std::string e;
        if (!resolve_hands_on(hand_spec, n.player, board_with(slots), hands, e))
            return false;
        std::set<std::string> nombres;
        for (int h : hands) {
            const Combo& k = deal_.combos[static_cast<size_t>(h)];
            nombres.insert(card_str(k.c1) + card_str(k.c2));
        }
        const std::string cl = rc.label, np = n.path;
        const std::string ro = runout_of_ctx(rc, slots);
        std::vector<LockSpec> quedan;
        quedan.reserve(locks_.size());
        for (const LockSpec& K : locks_) {
            const bool aqui = (K.ctx_label == cl && K.node_path == np && K.runout == ro);
            if (aqui && nombres.count(no_space(K.hand_spec))) { ++quitados; continue; }
            quedan.push_back(K);
        }
        if (!quitados) return false;
        locks_.swap(quedan);
        // El bloque del nodo se limpia entero y los que quedan vuelven a entrar
        // en el proximo solve, que es cuando se aplican de todas formas.
        if (S_) S_->clear_locks(ci, nid);
        locks_pending_ = true;
        return true;
    }

    bool remove_lock_at(int ci, int nid, const std::vector<int>& slots) {
        if (ci < 0 || ci >= static_cast<int>(tree_.ctx.size())) return false;
        const RoundCtx& rc = tree_.ctx[static_cast<size_t>(ci)];
        if (nid < 0 || nid >= static_cast<int>(rc.tree.nodes.size())) return false;
        // Solo el runout donde estas. Quitar el lock del 3s no puede llevarse
        // por delante el que pusiste en el As, que es otro nodo distinto.
        const std::string cl = rc.label, np = rc.tree.nodes[static_cast<size_t>(nid)].path;
        const std::string ro = runout_of_ctx(rc, slots);
        const size_t before = locks_.size();
        // A mano por lo mismo que el de add_lock: ver el comentario de alli.
        std::vector<LockSpec> quedan;
        quedan.reserve(locks_.size());
        for (size_t i = 0; i < locks_.size(); ++i) {
            const LockSpec& K = locks_[i];
            if (!(K.ctx_label == cl && K.node_path == np && K.runout == ro))
                quedan.push_back(K);
        }
        locks_.swap(quedan);
        if (locks_.size() == before) return false;
        if (S_) S_->clear_locks(ci, nid);
        locks_pending_ = true;
        return true;
    }

    // Drops every lock at once. Removing them one node at a time meant
    // hunting through the tree for nodes you had forgotten you locked; it
    // also meant a leftover lock quietly biasing the next thing you looked at.
    int clear_all_locks() {
        const int n = static_cast<int>(locks_.size());
        if (S_) {
            for (const LockSpec& L : locks_) {
                const int c = tree_.find_ctx(L.ctx_label);
                if (c < 0) continue;
                const int nd = tree_.find_node(c, L.node_path);
                if (nd >= 0) S_->clear_locks(c, nd);
            }
        }
        locks_.clear();
        dropped_locks_ = 0;
        if (n) locks_pending_ = true;
        return n;
    }

    // ---- memory ----------------------------------------------------------
    //  What the solve will actually take, not just its two big buffers. The
    //  limit used to be checked against the buffers alone, so the number in
    //  front of the user was a fifth low on a small flop -- and the thing it
    //  was protecting was the process, which does not care which vector the
    //  bytes are in.
    struct MemUse {
        double buffers = 0;   // regret + strategy sum
        double stamps  = 0;   // one per decision node instance
        double tables  = 0;   // the deal's scores and strength order
        double sweep   = 0;   // the packed showdown order
        double scratch = 0;   // per-thread traversal frames
        double total   = 0;
    };

    MemUse memory_use() const {
        const double G = 1024.0 * 1024.0 * 1024.0;
        MemUse m;
        const long long nh = deal_.num();
        m.buffers = tree_.total_gb();
        m.stamps  = static_cast<double>(tree_.stamp_size *
                    static_cast<long long>(sizeof(int))) / G;
        m.tables  = static_cast<double>(deal_.table_bytes()) / G;

        // The sweep table covers the union of the two live ranges. With no
        // range set yet the union is empty, but the buffers above are laid
        // out over the whole board in that case -- so this counts the whole
        // board too. Half an upper bound and half a zero is a number that
        // means nothing.
        long long uni = 0;
        for (int h = 0; h < nh; ++h)
            if (range_[0][static_cast<size_t>(h)] > 0.0 ||
                range_[1][static_cast<size_t>(h)] > 0.0) ++uni;
        if (uni == 0) uni = nh;
        m.sweep = static_cast<double>(deal_.num_runouts * uni *
                  static_cast<long long>(sizeof(SdEntry))) / G;

        int th = static_cast<int>(std::thread::hardware_concurrency());
        if (cfg::THREADS > 0) th = cfg::THREADS;
        if (th < 1) th = 1;
        if (th > 64) th = 64;
        const long long frames = tree_.max_depth + 4;
        const long long perframe = (2LL * cfg::MAX_ACTIONS + 3) * nh *
                                   static_cast<long long>(sizeof(double));
        m.scratch = static_cast<double>((th + 1) * frames * perframe) / G;

        m.total = m.buffers + m.stamps + m.tables + m.sweep + m.scratch;
        return m;
    }
    double memory_gb() const { return memory_use().total; }

    // ---- misc ------------------------------------------------------------
    int live_combos(int p) const {
        int n = 0;
        for (int h = 0; h < deal_.num(); ++h) if (range_[p][static_cast<size_t>(h)] > 0.0) ++n;
        return n;
    }
    double range_weight(int p) const {
        double w = 0.0;
        for (int h = 0; h < deal_.num(); ++h) w += range_[p][static_cast<size_t>(h)];
        return w;
    }

private:
    // Runs the solve in chunks rather than one long call, so the mutex is
    // released regularly: that is what lets the UI read strategies mid-solve.
    // Chunk length adapts to hit roughly a third of a second, and the accuracy
    // check is rate limited to a small slice of the total time -- on a flop one
    // exploitability pass costs about two iterations.
    void worker_body(int total, bool fresh) {
        using clock = std::chrono::steady_clock;
        acc_hit_.store(false);
        timeout_hit_.store(false);
        const clock::time_point empezo = clock::now();
        if (fresh) {
            std::lock_guard<std::mutex> g(mtx_);
            if (!build_solver(&async_notes_)) {
                solved_ = false;
                running_.store(false);
                return;
            }
            solved_ = false;
        }
        // Zero after a fresh build, and the count already done when adding to
        // an existing solve -- which is what makes the progress bar measure the
        // iterations asked for rather than the total ever run.
        const int base = S_->iterations_done();
        int    chunk = 1;
        double explCost = 0.0;
        clock::time_point nextExpl = clock::now();

        while (!cancel_.load()) {
            const int done = S_->iterations_done() - base;
            if (done >= total) break;
            if (timeout_secs_ > 0.0 &&
                std::chrono::duration<double>(clock::now() - empezo).count() >= timeout_secs_) {
                timeout_hit_.store(true);
                break;
            }
            const int c = std::min(chunk, total - done);

            const clock::time_point t0 = clock::now();
            {
                std::lock_guard<std::mutex> g(mtx_);
                S_->run(c, 0, nullptr, &cancel_);
                solved_ = true;          // partial results are already readable
            }
            const double secs = std::chrono::duration<double>(clock::now() - t0).count();
            prog_.store(S_->iterations_done() - base);

            // Let anyone waiting to read actually get in before grabbing the
            // lock again for the next chunk.
            while (readers_.load() > 0 && !cancel_.load())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Long chunks while nobody is reading, short ones while somebody
            // is: a click during a solve should not wait out a chunk sized for
            // a screen nobody is looking at. Two seconds of quiet and it goes
            // back to the long ones.
            const long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                clock::now().time_since_epoch()).count();
            const bool watched = readers_.load() > 0 ||
                                 (now_ms - last_read_.load()) < 2000;
            const double target = watched ? 0.04 : 0.33;
            const double per = (c > 0) ? secs / c : 0.0;
            chunk = (per > 1e-9) ? std::max(1, static_cast<int>(target / per)) : 64;

            if (clock::now() >= nextExpl) {
                const clock::time_point e0 = clock::now();
                {
                    std::lock_guard<std::mutex> g(mtx_);
                    expl_.store(S_->exploitability());
                }
                if (accuracy_met()) { acc_hit_.store(true); break; }
                explCost = std::chrono::duration<double>(clock::now() - e0).count();
                nextExpl = clock::now() +
                           std::chrono::milliseconds(
                               std::max(1500, static_cast<int>(explCost * 20000.0)));
            }
        }
        {   // a final, accurate reading
            std::lock_guard<std::mutex> g(mtx_);
            expl_.store(S_->exploitability());
        }
        prog_seconds_ = std::chrono::duration<double>(clock::now() - started_).count();
        running_.store(false);
    }

    Deal                        deal_;
    std::string                 board_spec_;
    std::string                 range_spec_[2];
    std::vector<double>         range_[2];
    TreeConfig                  tc_;
    GameTree                    tree_;
    std::vector<LockSpec>       locks_;
    std::unique_ptr<DCFRSolver> S_;
    int                         cur_ctx_  = 0;
    int                         cur_node_ = 0;
    std::vector<int>            slots_;
    // Un tope alto a proposito: es la red de seguridad, no el criterio. Con
    // 300 el tope llegaba antes que el objetivo en cuanto el arbol crecia, y
    // entonces el campo de precision parecia no hacer nada.
    int                         iters_    = 3000;
    double                      acc_target_ = 1.0;   // % del bote, convencion estandar
    bool                        acc_stop_   = true;
    std::atomic<bool>           acc_hit_{false};
    double                      timeout_secs_ = 0.0;
    std::atomic<bool>           timeout_hit_{false};
    std::atomic<bool>           solved_{false};
    int                         dropped_locks_ = 0;
    std::string                 load_note_;
    bool                        iso_off_by_lock_ = false;

    std::thread                 worker_;
    std::atomic<bool>           running_{false};
    std::atomic<bool>           cancel_{false};
    std::atomic<int>            prog_{0};
    std::atomic<long long>      last_read_{0};
    int                         prog_total_   = 0;
    double                      prog_seconds_ = 0.0;
    std::chrono::steady_clock::time_point started_{};
    std::vector<std::string>    async_notes_;
    std::string                 build_error_;
    std::atomic<double>         expl_{-1.0};
    std::mutex                  mtx_;         // guards the solver's data
    std::mutex                  life_mtx_;    // guards the worker thread itself
    std::atomic<int>            readers_{0};

    bool reset_deal(std::string& e) {
        std::vector<int> b;
        if (!parse_board(board_spec_, b, e)) return false;
        if (!deal_.build(b, e)) return false;
        for (int p = 0; p < 2; ++p) {
            if (trim(range_spec_[p]).empty())
                range_[p].assign(static_cast<size_t>(deal_.num()), 0.0);
            else if (!parse_range(range_spec_[p], deal_, range_[p], e)) return false;
        }
        deal_.build_orbits(range_[0], range_[1], cfg::ISO);
        // Nobody was aggressive before the street this starts on, so there is
        // nothing to lead into and the builder never reads that list. It can
        // only be non-empty because the board moved under it or a config came
        // from a deeper one, and either way it is dropped -- out loud, because
        // a setting read and silently discarded is the bug this whole file
        // keeps guarding against.
        if (!tc_.donks[deal_.start].empty()) {
            tc_.donks[deal_.start].clear();
            if (!load_note_.empty()) load_note_ += "; ";
            load_note_ += std::string("dropped the donk sizes on the ") +
                          STREET_NAME[deal_.start] + ": it is the street this "
                          "solve starts on, so there is nobody to lead into";
        }
        return true;
    }
};
