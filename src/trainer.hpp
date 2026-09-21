#pragma once
// =============================================================================
//  Jugar el arbol que acabas de resolver.
//
//  Mirar una estrategia y JUGARLA son dos cosas distintas. Delante de la
//  rejilla todo parece obvio; con una mano concreta en la mano, un bote y un
//  rival que acaba de subir, ya no. Esto reparte una mano de tu rango, te
//  sienta en el spot, y el rival juega la solucion. Al final te dice lo que te
//  costo cada decision.
//
//  LO QUE SE MIDE. En cada decision tuya se mira el EV de CADA accion PARA TU
//  MANO EXACTA -- que es lo que el motor ya calcula para pintar la rejilla -- y
//  se compara la que tomaste con la mejor. La diferencia es lo que te costo esa
//  decision, en fichas. Sumado, es tu perdida por mano.
//
//  No se puntua por "jugar la frecuencia": con AhKh en un nodo donde el solver
//  apuesta el 70% del tiempo, pasar NO es un error si pasar vale lo mismo. En
//  equilibrio las acciones que se mezclan valen lo mismo, y esa es justo la
//  razon por la que se mezclan. Puntuar contra la frecuencia castigaria lo que
//  la teoria dice que es indiferente, que es como se aprenden supersticiones.
//  Se puntua contra el EV, que es lo que cuesta dinero.
//
//  EL RIVAL juega la solucion: en cada nodo suyo se tira un dado con las
//  frecuencias de SU mano. No juega para castigarte -- no sabe lo que tienes --
//  y por eso el resultado de una mano suelta no dice nada: te puede pagar con
//  la peor mano del rango y llevarsela. Lo que dice algo es la perdida de EV,
//  que no depende de como caigan las cartas.
//
//  LA SEMILLA. Cada mano se reparte con una semilla que se ensena y se puede
//  repetir: mismas cartas tuyas, mismas suyas, mismo runout. Es la unica forma
//  de volver a jugar la mano que te salio mal y ver que pasa si haces otra cosa.
// =============================================================================

#include "msg.hpp"
#include "report.hpp"
#include "session.hpp"

#include <random>
#include <string>
#include <vector>

// Una decision tuya, con lo que costo.
struct TrainStep {
    int         ctx = 0, node = 0;
    long long   inst = 0;
    std::string street, line;
    double      pot = 0.0;
    int         A = 0, chosen = -1, best = -1;
    std::vector<std::string> codes, labels;
    std::vector<double>      freq, ev;     // por accion, PARA TU MANO
    double      loss = 0.0;                // ev[best] - ev[chosen], nunca negativo
};

class Trainer {
public:
    enum Phase { OFF = 0, YOURS, OVER };

    // ---- lo que se elige antes de jugar ------------------------------------
    void set_side(int s)     { side_ = (s == 1) ? 1 : 0; }
    // Donde empieza la mano. De fabrica, el principio del solve -- si
    // resolviste un flop, el flop --, que es lo que quiere casi todo el mundo.
    //
    // Pero entrenar UN spot concreto es justo lo que hace falta muchas veces:
    // "el turn despues de apostar y que me paguen". Asi que se puede dar
    // cualquier nodo del arbol. Las cartas del runout se pueden fijar o dejar
    // al azar: sin ellas se reparte una distinta cada mano, que es lo que
    // conviene para practicar un nodo y no una carta.
    void set_start(int ctx, int node, const std::vector<int>& slots) {
        start_ctx_ = ctx < 0 ? 0 : ctx;
        start_node_ = node;
        start_slots_ = slots;
    }
    void start_at_root() { start_ctx_ = 0; start_node_ = -1; start_slots_.clear(); }
    int  start_ctx() const  { return start_ctx_; }
    int  start_node() const { return start_node_; }
    bool starts_at_root() const { return start_ctx_ == 0 && start_node_ < 0; }
    const std::string& start_line() const { return start_line_; }
    int  side() const        { return side_; }
    void set_advice(bool v)  { advice_ = v; }
    bool advice() const      { return advice_; }

    Phase phase() const          { return phase_; }
    bool  on() const             { return phase_ != OFF; }
    unsigned seed() const        { return seed_; }
    int   hand_number() const    { return hands_; }

    // ---- la mesa -----------------------------------------------------------
    const std::vector<int>& board() const   { return board_; }
    const int* hero() const                 { return hero_; }
    const int* villain() const              { return villain_; }
    bool  villain_shown() const             { return shown_; }
    double pot() const                      { return pot_; }
    double to_call() const                  { return to_call_; }
    // Lo que cada uno tiene DELANTE en esta calle, y lo que le queda detras.
    //
    // El stack se saca del bote y no de una cuenta propia: una calle se cierra
    // cuando las apuestas estan igualadas, asi que al empezar una ronda los dos
    // llevan puesto lo mismo, (bote - bote inicial) / 2, y dentro de la ronda lo
    // que se ve delante. Con un all-in por debajo del stack del otro eso deja de
    // ser exacto, y ahi los dos numeros que importan -- el bote y lo que hay
    // delante -- siguen siendo los de verdad.
    double in_front(int p) const { return invested_[p ? 1 : 0]; }
    double stack_of(int p) const {
        const double s = cfg::STACK - committed_ - invested_[p ? 1 : 0];
        return s > 0.0 ? s : 0.0;
    }
    // El bote sin lo que hay delante, que es lo que se ensena en el centro.
    double pot_middle() const {
        const double m = pot_ - invested_[0] - invested_[1];
        return m > 0.0 ? m : 0.0;
    }
    int   street() const                    { return street_; }
    const std::vector<std::string>& log() const { return log_; }
    const std::vector<TrainStep>&   steps() const { return steps_; }
    // Con que pesos se repartio la ultima mano. La regla es que son el ALCANCE
    // en el nodo de salida, y mirando las manos que salen no se distingue de
    // repartir del rango de partida hasta que llevas cientos.
    const std::vector<double>& deal_weights(bool hero_side) const {
        return hero_side ? w_hero_ : w_vill_;
    }

    // Lo que puedes hacer ahora.
    const std::vector<std::string>& codes() const  { return codes_; }
    const std::vector<std::string>& labels() const { return labels_; }
    const std::vector<double>&      amounts() const { return amounts_; }
    // El consejo para tu mano en este nodo: frecuencias y EV por accion.
    const std::vector<double>& freqs() const { return freq_; }
    const std::vector<double>& evs() const   { return ev_; }

    // ---- el final ----------------------------------------------------------
    bool   ended() const         { return phase_ == OVER; }
    double result() const        { return result_; }      // fichas ganadas, en cero suma
    bool   showdown() const      { return showdown_; }
    const std::string& hero_made() const    { return hero_made_; }
    // Lo que llevas hecho con el board que hay puesto ahora mismo. En la mesa
    // esto se ve de un vistazo y en una pantalla no, asi que se dice.
    std::string hero_category() const {
        if (hero_[0] < 0 || board_.size() < 3) return std::string();
        return made_draw_name(made_cat(hero_[0], hero_[1], board_),
                              draw_cat(hero_[0], hero_[1], board_));
    }
    const std::string& villain_made() const { return villain_made_; }

    // ---- el marcador de la sesion -----------------------------------------
    int    hands_played() const  { return hands_done_; }
    int    decisions() const     { return decisions_; }
    double lost() const          { return lost_; }
    double won() const           { return won_; }
    double pot0() const          { return pot0_; }
    // La perdida por decision en % del bote inicial, que es la forma de que el
    // numero signifique lo mismo en un bote de 20 y en uno de 500.
    // Lo MENOS probable que ha hecho el bot en toda la sesion, segun su propia
    // estrategia. Sirve para una sola cosa, y es la que importa: si el bot
    // dejara de tirar el dado con sus frecuencias -- y jugara a lo loco, o
    // siempre igual -- este numero se iria a cero en cuanto tocara una accion
    // que su mano no toma nunca. Sin el, un bot roto se ve igual que uno bueno.
    double villain_min_freq() const { return vill_min_f_; }
    // La frecuencia MEDIA de lo que ha jugado el bot, segun su propia
    // estrategia. Un bot que tira el dado con sus frecuencias saca de media la
    // suma de los cuadrados -- alta, porque casi todas las manos juegan casi
    // siempre lo mismo -- y uno que reparte uniforme saca 1/acciones, que es
    // lo mas bajo posible. Es lo que distingue a los dos sin jugarle mil manos.
    double villain_freq_mean() const {
        return vill_f_n_ > 0 ? vill_f_sum_ / vill_f_n_ : 0.0;
    }
    int    villain_decisions() const { return vill_f_n_; }
    int    villain_no_strategy() const { return vill_sin_; }

    double loss_pct() const {
        if (decisions_ <= 0 || pot0_ <= 1e-9) return 0.0;
        return 100.0 * (lost_ / decisions_) / pot0_;
    }

    void reset_score() {
        hands_done_ = 0; decisions_ = 0; lost_ = 0.0; won_ = 0.0;
        vill_min_f_ = 2.0; vill_sin_ = 0;
        vill_f_sum_ = 0.0; vill_f_n_ = 0;
    }
    void stop() { phase_ = OFF; log_.clear(); steps_.clear(); }

    // ---- jugar -------------------------------------------------------------

    // Reparte una mano. Con semilla 0 se saca una del reloj.
    bool new_hand(Session& S, std::string& e, unsigned use_seed = 0) {
        if (!S.solved() || !S.solver() || S.solver()->iterations_done() <= 0) {
            e = M("resuelve el arbol antes de jugarlo: todavía no hay estrategia",
                  "solve the tree before playing it -- there is no strategy yet");
            return false;
        }
        const GameTree& T = S.tree();
        if (T.ctx.empty()) { e = M("no hay árbol", "no tree"); return false; }

        seed_ = use_seed ? use_seed : next_seed();
        rng_.seed(seed_);
        ++hands_;

        ctx_ = (start_ctx_ >= 0 && start_ctx_ < static_cast<int>(T.ctx.size()))
                   ? start_ctx_ : 0;
        const BetTree& bt0 = T.ctx[static_cast<size_t>(ctx_)].tree;
        node_ = (start_node_ >= 0 && start_node_ < static_cast<int>(bt0.nodes.size()))
                    ? start_node_ : bt0.root;
        slots_ = start_slots_;
        log_.clear();
        steps_.clear();
        shown_ = false;
        showdown_ = false;
        // Las de la mano anterior fuera antes de repartir nada: si no, el
        // runout de esta esquivaria unas cartas que ya no tiene nadie.
        hero_[0] = hero_[1] = villain_[0] = villain_[1] = -1;
        result_ = 0.0;
        hero_made_.clear();
        villain_made_.clear();
        pot0_ = cfg::POT0;

        // Primero las cartas que hagan falta para llegar al nodo, luego la
        // mano. En ese orden: los combos que usan una carta del board tienen
        // alcance cero ahi, asi que repartir antes es lo que hace imposible
        // que te toque una mano con una carta que ya esta en la mesa.
        tree_ = &T;
        if (!fill_runout(S, e)) return false;
        if (!deal_hands(S, e)) return false;
        set_committed();
        rebuild_invested();
        if (!starts_at_root())
            log_.push_back(std::string(M("empiezas en ", "you start at ")) + start_line_);
        phase_ = YOURS;
        return advance(S, e);
    }

    // La misma mano otra vez: mismas cartas, mismo runout, mismo dado.
    bool repeat_hand(Session& S, std::string& e) {
        if (!seed_) { e = M("no hay ninguna mano que repetir", "there is no hand to repeat"); return false; }
        const unsigned s = seed_;
        --hands_;                       // repetir no es una mano nueva
        return new_hand(S, e, s);
    }

    // Tu accion, por su codigo ("X", "B16", "C"...).
    bool act(Session& S, const std::string& code, std::string& e) {
        if (phase_ != YOURS) { e = M("no te toca", "not your turn"); return false; }
        if (!sigue_en_pie(S, e)) return false;
        int a = -1;
        for (size_t i = 0; i < codes_.size(); ++i)
            if (lower(codes_[i]) == lower(trim(code))) { a = static_cast<int>(i); break; }
        if (a < 0) { e = M("aquí no puedes hacer eso", "you cannot do that here"); return false; }

        // Lo que costo, antes de moverse.
        TrainStep p;
        p.ctx = ctx_; p.node = node_; p.inst = inst_;
        p.street = street_name();
        p.line = node_ref().path;
        p.pot = pot_;
        p.A = static_cast<int>(codes_.size());
        p.codes = codes_; p.labels = labels_;
        p.freq = freq_;   p.ev = ev_;
        p.chosen = a;
        p.best = best_action();
        if (p.best >= 0 && a >= 0) {
            const double d = p.ev[static_cast<size_t>(p.best)] - p.ev[static_cast<size_t>(a)];
            p.loss = d > 0.0 ? d : 0.0;
        }
        steps_.push_back(p);
        ++decisions_;
        lost_ += p.loss;

        say(side_, labels_[static_cast<size_t>(a)]);
        step_into(S, a);
        return advance(S, e);
    }

private:
    // El arbol de debajo puede haber cambiado mientras jugabas: cambiar el
    // board o los tamanos tira la solucion y reconstruye, y los indices de
    // contexto y de nodo que lleva esto apuntarian a otro arbol. No es un aviso
    // feo: es leer memoria de un arbol que ya no esta.
    //
    // Asi que antes de tocar nada se mira, y si ha cambiado la mano se da por
    // terminada y se dice. Reparte otra y sigues.
    bool sigue_en_pie(Session& S, std::string& e) {
        const bool vivo = S.solved() && S.solver() &&
                          S.solver()->iterations_done() > 0 &&
                          ctx_ >= 0 && ctx_ < static_cast<int>(S.tree().ctx.size()) &&
                          node_ >= 0 &&
                          node_ < static_cast<int>(
                              S.tree().ctx[static_cast<size_t>(ctx_)].tree.nodes.size());
        if (vivo) { tree_ = &S.tree(); return true; }
        phase_ = OFF;
        e = M("el árbol ha cambiado por debajo: reparte otra mano",
              "the tree changed underneath: deal another hand");
        return false;
    }

    // ---------------------------------------------------------------- estado
    int      side_ = 0;
    bool     advice_ = false;
    Phase    phase_ = OFF;
    unsigned seed_ = 0;
    std::mt19937 rng_;

    int       start_ctx_ = 0, start_node_ = -1;
    std::vector<int> start_slots_;
    std::string start_line_;
    int       ctx_ = 0, node_ = 0;
    long long inst_ = 0;
    int       perm_ = 0;
    std::vector<int> slots_;
    std::vector<int> board_;
    int       hero_[2] = { -1, -1 }, villain_[2] = { -1, -1 };
    int       combo_hero_ = -1, combo_villain_ = -1;
    int       street_ = 0;
    double    pot_ = 0.0, to_call_ = 0.0, invested_[2] = { 0.0, 0.0 };
    double    committed_ = 0.0;      // lo puesto en las calles anteriores, por cabeza
    bool      shown_ = false, showdown_ = false;
    double    result_ = 0.0;
    std::string hero_made_, villain_made_;

    std::vector<std::string> codes_, labels_, log_;
    std::vector<double>      amounts_, freq_, ev_;
    std::vector<TrainStep>   steps_;

    int    hands_ = 0, hands_done_ = 0, decisions_ = 0;
    std::vector<double> w_hero_, w_vill_;
    double vill_min_f_ = 2.0, vill_f_sum_ = 0.0;
    int    vill_sin_ = 0, vill_f_n_ = 0;
    double lost_ = 0.0, won_ = 0.0, pot0_ = 0.0;

    // --------------------------------------------------------------- ayudas
    static unsigned next_seed() {
        static std::random_device rd;
        unsigned s = rd();
        return s ? s : 1u;
    }
    const Node& node_ref() const { return cur_tree().nodes[static_cast<size_t>(node_)]; }
    const BetTree& cur_tree() const { return tree_->ctx[static_cast<size_t>(ctx_)].tree; }

    std::string street_name() const {
        const int st = tree_->ctx[static_cast<size_t>(ctx_)].street;
        return (st >= 0 && st <= 2) ? STREET_NAME[st] : "?";
    }
    void say(int who, const std::string& what) {
        log_.push_back(std::string(who == 0 ? "OOP " : "IP ") + what);
    }

    const GameTree* tree_ = nullptr;

    // Reparte tu mano y la del rival, cada una de SU rango y sin compartir
    // cartas. Se muestrea por peso: un rango con AKs a 0,5 lo da la mitad de
    // veces que uno a 1, que es lo que significa el peso.
    bool deal_hands(Session& S, std::string& e) {
        tree_ = &S.tree();
        const Deal& D = S.deal();

        // El peso con el que se reparte es el ALCANCE en este nodo, no el
        // rango de partida. En la raiz son lo mismo; tres acciones mas abajo
        // no: el rango que llega a pagar una apuesta no es el de partida, y
        // repartir del de partida te daria manos que ahi no existen -- estarias
        // practicando un spot que no se juega nunca.
        std::vector<double> mine, theirs;
        if (!reach_here(S, mine, theirs, e)) return false;
        w_hero_ = mine;
        w_vill_ = theirs;

        combo_hero_ = pick_weighted(mine, -1, -1, D);
        if (combo_hero_ < 0) {
            e = M("tu rango está vacío en este board", "your range is empty on this board");
            return false;
        }
        hero_[0] = D.combos[static_cast<size_t>(combo_hero_)].c1;
        hero_[1] = D.combos[static_cast<size_t>(combo_hero_)].c2;

        combo_villain_ = pick_weighted(theirs, hero_[0], hero_[1], D);
        if (combo_villain_ < 0) {
            e = M("el rango del rival está vacío en este board",
                  "the opponent's range is empty on this board");
            return false;
        }
        villain_[0] = D.combos[static_cast<size_t>(combo_villain_)].c1;
        villain_[1] = D.combos[static_cast<size_t>(combo_villain_)].c2;

        // El board y el bote los pone quien coloca la mano en el arbol, no
        // esto: aqui ya puede haber cartas repartidas y dinero puesto.
        return true;
    }

    // El alcance de cada uno EN ESTE NODO.
    bool reach_here(Session& S, std::vector<double>& mine,
                    std::vector<double>& theirs, std::string& e) {
        if (!locate(S, e)) return false;
        const Node& n = node_ref();
        if (n.type != NT_DECISION) {
            e = M("ahí no se decide nada", "nothing is decided there");
            return false;
        }
        NodeStats N = gather(*S.solver(), ctx_, node_, inst_, perm_, false);
        if (!N.ok) { e = M("no se pudo leer ese nodo", "could not read that node"); return false; }
        const bool soy_yo = (n.player == side_);
        mine   = soy_yo ? N.v.own_reach : N.v.opp_reach;
        theirs = soy_yo ? N.v.opp_reach : N.v.own_reach;
        return true;
    }

    // Las cartas que hagan falta para llegar al nodo de salida. Las que no se
    // hayan fijado salen al azar, y salen otra vez en cada mano.
    bool fill_runout(Session& S, std::string& e) {
        const Deal& D = S.deal();
        const int levels = tree_->ctx[static_cast<size_t>(ctx_)].street - D.start;
        if (levels <= static_cast<int>(slots_.size())) {
            slots_.resize(static_cast<size_t>(levels < 0 ? 0 : levels));
        } else {
            while (static_cast<int>(slots_.size()) < levels) {
                std::vector<int> libres;
                for (int sl = 0; sl < D.deckN(); ++sl) {
                    bool usado = false;
                    for (int s2 : slots_) if (s2 == sl) { usado = true; break; }
                    if (!usado) libres.push_back(sl);
                }
                if (libres.empty()) { e = M("no quedan cartas", "no cards left"); return false; }
                std::uniform_int_distribution<size_t> u(0, libres.size() - 1);
                slots_.push_back(libres[u(rng_)]);
            }
        }
        board_ = D.board;
        for (int sl : slots_) board_.push_back(D.deck[static_cast<size_t>(sl)]);
        // Y el nodo de salida, si no es la raiz, tambien hay que saber pasar
        // por encima de los nodos de azar que queden por delante.
        for (int guard = 0; guard < 8; ++guard) {
            const Node& n = node_ref();
            if (n.type != NT_CONT) break;
            if (!deal_card(S, e)) return false;
        }
        // El nombre de la linea, como se escribe en el resto del programa: la
        // del contexto, y detras el camino dentro de la ronda sin repetir la R
        // que ya esta delante.
        start_line_ = tree_->ctx[static_cast<size_t>(ctx_)].label;
        const std::string p = node_ref().path;
        if (p.size() > 1 && p.rfind("R/", 0) == 0) {
            if (start_line_ == "R") start_line_ = p;
            else start_line_ += p.substr(1);
        }
        return true;
    }

    // Lo que cada uno lleva puesto EN ESTA CALLE al empezar: si el punto de
    // salida esta despues de una apuesta, ese dinero ya esta en el bote y sin
    // esto la pantalla diria que no hay nada que pagar.
    // Lo que lleva puesto cada uno de las calles anteriores.
    void set_committed() {
        const double p = tree_->ctx[static_cast<size_t>(ctx_)].pot;
        committed_ = (p - cfg::POT0) * 0.5;
        if (committed_ < 0.0) committed_ = 0.0;
    }

    void rebuild_invested() {
        invested_[0] = invested_[1] = 0.0;
        const BetTree& bt = cur_tree();
        std::vector<std::pair<int, int> > camino;   // (nodo, accion)
        int cur = node_;
        for (int guard = 0; guard < 256 && cur != bt.root; ++guard) {
            int padre = -1, acc = -1;
            for (size_t i = 0; i < bt.nodes.size() && padre < 0; ++i) {
                const Node& n = bt.nodes[i];
                if (n.type != NT_DECISION) continue;
                for (int k = 0; k < n.num_actions; ++k)
                    if (bt.child(n, k) == cur) { padre = static_cast<int>(i); acc = k; break; }
            }
            if (padre < 0) break;
            camino.push_back(std::make_pair(padre, acc));
            cur = padre;
        }
        for (size_t i = camino.size(); i-- > 0; ) {
            const Node& n = bt.nodes[static_cast<size_t>(camino[i].first)];
            const ActionInfo& ai = bt.act(n, camino[i].second);
            if (ai.kind == AK_BET || ai.kind == AK_RAISE || ai.kind == AK_CALL)
                invested_[static_cast<size_t>(n.player)] = ai.to_amount;
        }
    }

    // Un combo al azar del rango, sin las cartas ya repartidas.
    int pick_weighted(const std::vector<double>& w, int no1, int no2, const Deal& D) {
        double tot = 0.0;
        for (size_t h = 0; h < w.size(); ++h) {
            if (w[h] <= 0.0) continue;
            const Combo& c = D.combos[h];
            if (c.c1 == no1 || c.c1 == no2 || c.c2 == no1 || c.c2 == no2) continue;
            tot += w[h];
        }
        if (tot <= 0.0) return -1;
        std::uniform_real_distribution<double> u(0.0, tot);
        double x = u(rng_);
        for (size_t h = 0; h < w.size(); ++h) {
            if (w[h] <= 0.0) continue;
            const Combo& c = D.combos[h];
            if (c.c1 == no1 || c.c1 == no2 || c.c2 == no1 || c.c2 == no2) continue;
            x -= w[h];
            if (x <= 0.0) return static_cast<int>(h);
        }
        return -1;
    }

    // Anda el arbol hasta que te toque a ti o se acabe la mano.
    bool advance(Session& S, std::string& e) {
        for (int guard = 0; guard < 4096; ++guard) {
            const Node& n = node_ref();
            if (n.type == NT_DECISION) {
                if (!locate(S, e)) return false;
                if (n.player == side_) { collect_actions(S); phase_ = YOURS; return true; }
                const int a = villain_action(S);
                if (a < 0) { e = M("el rival no tiene estrategia en ese nodo",
                                   "the opponent has no strategy at that node"); return false; }
                say(1 - side_, cur_tree().act(n, a).label);
                step_into(S, a);
                continue;
            }
            if (n.type == NT_CONT) {
                if (!deal_card(S, e)) return false;
                continue;
            }
            finish(S);
            return true;
        }
        e = M("el árbol no termina", "the tree does not end");
        return false;
    }

    // Donde estamos, para leer la estrategia: instancia y permutacion.
    bool locate(Session& S, std::string& e) {
        if (!S.deal().locate(slots_, inst_, perm_)) {
            e = M("no se pudo situar el runout", "could not locate the runout");
            return false;
        }
        pot_ = node_ref().pot;
        street_ = tree_->ctx[static_cast<size_t>(ctx_)].street;
        return true;
    }

    // Las acciones de ESTE nodo, con su precio, y el consejo para tu mano.
    void collect_actions(Session& S) {
        const Node& n = node_ref();
        codes_.clear(); labels_.clear(); amounts_.clear();
        freq_.assign(static_cast<size_t>(n.num_actions), 0.0);
        ev_.assign(static_cast<size_t>(n.num_actions), 0.0);
        double already = invested_[static_cast<size_t>(side_)];
        for (int a = 0; a < n.num_actions; ++a) {
            const ActionInfo& ai = cur_tree().act(n, a);
            codes_.push_back(ai.code);
            labels_.push_back(ai.label);
            amounts_.push_back(ai.to_amount);
        }
        to_call_ = 0.0;
        for (int a = 0; a < n.num_actions; ++a)
            if (cur_tree().act(n, a).kind == AK_CALL)
                to_call_ = cur_tree().act(n, a).to_amount - already;
        if (to_call_ < 0.0) to_call_ = 0.0;

        NodeStats N = gather(*S.solver(), ctx_, node_, inst_, perm_, false);
        if (!N.ok) return;
        for (int a = 0; a < n.num_actions && a < N.A; ++a) {
            freq_[static_cast<size_t>(a)] = N.freq(combo_hero_, a);
            // En la misma cuenta que el resto del programa: el EV es tu parte
            // del bote, que es lo que ensena la rejilla en modo EV. Que el
            // consejo y la rejilla digan numeros distintos de lo mismo seria
            // la forma mas rapida de que nadie se fie de ninguno de los dos.
            ev_[static_cast<size_t>(a)]   = N.ev_action(combo_hero_, a);
        }
    }

    int best_action() const {
        int b = -1;
        double top = 0.0;
        for (size_t a = 0; a < ev_.size(); ++a)
            if (b < 0 || ev_[a] > top) { b = static_cast<int>(a); top = ev_[a]; }
        return b;
    }

    // El rival tira el dado con SUS frecuencias.
    int villain_action(Session& S) {
        const Node& n = node_ref();
        NodeStats N = gather(*S.solver(), ctx_, node_, inst_, perm_, false);
        if (!N.ok) return -1;
        double tot = 0.0;
        std::vector<double> p(static_cast<size_t>(n.num_actions), 0.0);
        for (int a = 0; a < n.num_actions && a < N.A; ++a) {
            const double f = N.freq(combo_villain_, a);
            p[static_cast<size_t>(a)] = f > 0.0 ? f : 0.0;
            tot += p[static_cast<size_t>(a)];
        }
        bool sin_estrategia = false;
        if (tot <= 1e-12) {           // sin estrategia ahi: reparto uniforme
            sin_estrategia = true;
            ++vill_sin_;
            for (double& x : p) x = 1.0;
            tot = static_cast<double>(n.num_actions);
        }
        std::uniform_real_distribution<double> u(0.0, tot);
        double x = u(rng_);
        int elegida = n.num_actions - 1;
        for (int a = 0; a < n.num_actions; ++a) {
            x -= p[static_cast<size_t>(a)];
            if (x <= 0.0) { elegida = a; break; }
        }
        if (!sin_estrategia) {
            // De N.freq y no de p: son el mismo numero MIENTRAS el bot juegue su
            // estrategia, y en cuanto deje de jugarla dejan de serlo. Apuntar el
            // del dado es apuntar lo que el bot cree que esta haciendo.
            const double f = N.freq(combo_villain_, elegida);
            if (f < vill_min_f_) vill_min_f_ = f;
            vill_f_sum_ += f;
            ++vill_f_n_;
        }
        return elegida;
    }

    // Baja por una accion, apuntando lo que cuesta.
    void step_into(Session& S, int a) {
        (void)S;
        const Node& n = node_ref();
        const ActionInfo& ai = cur_tree().act(n, a);
        const int who = n.player;
        if (ai.kind == AK_BET || ai.kind == AK_RAISE || ai.kind == AK_CALL) {
            const double extra = ai.to_amount - invested_[static_cast<size_t>(who)];
            if (extra > 0.0) invested_[static_cast<size_t>(who)] = ai.to_amount;
        }
        node_ = cur_tree().child(n, a);
        pot_ = node_ref().pot;
    }

    // Una carta al azar de las que quedan, sin las cuatro que estan en mano.
    bool deal_card(Session& S, std::string& e) {
        const Deal& D = S.deal();
        const RoundCtx& rc = tree_->ctx[static_cast<size_t>(ctx_)];
        const Node& n = node_ref();
        const int child = rc.cont_ctx[static_cast<size_t>(n.cont_id)];
        if (child < 0) { e = M("no hay calle después de esta", "no street after this one"); return false; }

        std::vector<int> free_slots;
        for (int sl = 0; sl < D.deckN(); ++sl) {
            const int c = D.deck[static_cast<size_t>(sl)];
            if (c == hero_[0] || c == hero_[1] || c == villain_[0] || c == villain_[1]) continue;
            bool used = false;
            for (int s : slots_) if (s == sl) { used = true; break; }
            if (!used) free_slots.push_back(sl);
        }
        if (free_slots.empty()) { e = M("no quedan cartas", "no cards left"); return false; }
        std::uniform_int_distribution<size_t> u(0, free_slots.size() - 1);
        const int slot = free_slots[u(rng_)];
        slots_.push_back(slot);
        board_.push_back(D.deck[static_cast<size_t>(slot)]);
        log_.push_back(std::string(M("sale ", "comes ")) + card_str(D.deck[static_cast<size_t>(slot)]));

        ctx_ = child;
        node_ = tree_->ctx[static_cast<size_t>(child)].tree.root;
        // Las apuestas de la calle anterior ya estan en el bote.
        invested_[0] = invested_[1] = 0.0;
        set_committed();
        return true;
    }

    // Se acabo: quien se lleva que.
    void finish(Session& S) {
        const Node& n = node_ref();
        phase_ = OVER;
        shown_ = true;
        ++hands_done_;
        pot_ = n.pot;

        if (n.type == NT_FOLD) {
            showdown_ = false;
            // El que se retira pierde lo que puso; el otro se lleva el bote
            // menos lo que coge la casa.
            result_ = (n.player == side_) ? -n.terminal_W : (n.terminal_W - n.rake);
            // Sin linea de "se retira": la accion que acaba de escribirse ya es
            // el Fold, y decirlo dos veces seguidas en el historial sobra.
        } else {
            showdown_ = true;
            // Las siete cartas de cada uno sobre el board final.
            const int sh = score_of(hero_);
            const int sv = score_of(villain_);
            hero_made_ = score_str(sh);
            villain_made_ = score_str(sv);
            if (sh > sv)      result_ = n.terminal_W - n.rake;
            else if (sh < sv) result_ = -n.terminal_W;
            else              result_ = -n.rake * 0.5;
            log_.push_back(M("showdown", "showdown"));
        }
        won_ += result_;
        (void)S;
    }

    int score_of(const int* hole) const {
        std::vector<int> c;
        c.reserve(board_.size() + 2);
        for (int x : board_) c.push_back(x);
        c.push_back(hole[0]);
        c.push_back(hole[1]);
        return eval_best(c.data(), static_cast<int>(c.size()));
    }
};
