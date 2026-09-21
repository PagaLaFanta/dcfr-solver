#pragma once
// =============================================================================
//  52-card deck, 7-card hand evaluator, 169 starting-hand classes.
//
//  Card encoding: index 0..51,  card = rank*4 + suit
//                 rank 0..12 = 2..A,  suit 0..3 = c,d,h,s
//
//  The evaluator is exhaustive rather than table-driven: it scores all 21
//  five-card subsets of the seven available cards. On a river solve every combo
//  is evaluated exactly once per board (a thousand-odd calls), so the constant
//  factor is irrelevant and correctness is easy to see by eye.
// =============================================================================

#include "msg.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
//  Cards
// -----------------------------------------------------------------------------
static const char RANK_CH[13] = { '2','3','4','5','6','7','8','9','T','J','Q','K','A' };
static const char SUIT_CH[4]  = { 'c','d','h','s' };

inline int  card_of(int rank, int suit) { return rank * 4 + suit; }
inline int  card_rank(int c)            { return c >> 2; }
inline int  card_suit(int c)            { return c & 3; }

inline std::string card_str(int c) {
    std::string s(1, RANK_CH[card_rank(c)]);
    s += SUIT_CH[card_suit(c)];
    return s;
}

// La "misma" mesa con los palos cambiados de nombre.
//
// Ah9h4h y As9s4s son el mismo problema: si se cambia el nombre de los palos,
// uno es el otro. Resolver los dos es pagar dos veces por la misma respuesta, y
// en una lista de flops para dejar de noche eso son horas.
//
// Devuelve una clave: misma clave, mismo flop salvo nombres de palo.
inline std::string flop_canon(int a, int b, int c) {
    int cs[3] = { a, b, c };
    // Por rango, y dentro del rango por palo: asi el orden no depende de como
    // venian, que es lo que haria que el mismo flop diera dos claves.
    std::sort(cs, cs + 3, [](int x, int y) { return x > y; });
    int mapa[4] = { -1, -1, -1, -1 };
    int siguiente = 0;
    std::string k;
    for (int i = 0; i < 3; ++i) {
        const int s = card_suit(cs[i]);
        if (mapa[s] < 0) mapa[s] = siguiente++;
        k += static_cast<char>(48 + card_rank(cs[i]));
        k += static_cast<char>(48 + mapa[s]);
    }
    return k;
}

inline int parse_rank(char ch) {
    const char u = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    for (int r = 0; r < 13; ++r) if (RANK_CH[r] == u) return r;
    return -1;
}

inline int parse_suit(char ch) {
    const char l = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    for (int s = 0; s < 4; ++s) if (SUIT_CH[s] == l) return s;
    return -1;
}

// "As", "Td", "7h". Returns -1 if the token is not a card.
inline int parse_card(const std::string& s) {
    if (s.size() != 2) return -1;
    const int r = parse_rank(s[0]);
    const int u = parse_suit(s[1]);
    if (r < 0 || u < 0) return -1;
    return card_of(r, u);
}

// Parses a whole board: "AsKd7h2c9s" or "As Kd 7h 2c 9s" (3, 4 or 5 cards).
// Returns false and fills `err` on a bad token or a repeated card.
inline bool parse_board(const std::string& in, std::vector<int>& out, std::string& err) {
    std::string s;
    for (char c : in) if (!std::isspace(static_cast<unsigned char>(c)) && c != ',') s += c;
    // A ten is 'T' here, and plenty of people write it '10'. Same courtesy as
    // the range parser, and for the same reason: refusing it teaches nothing.
    for (size_t z = s.find("10"); z != std::string::npos; z = s.find("10", z))
        s = s.substr(0, z) + "T" + s.substr(z + 2);
    if (s.size() % 2 != 0) { err = "board must be pairs of rank+suit"; return false; }

    out.clear();
    for (size_t i = 0; i < s.size(); i += 2) {
        const int c = parse_card(s.substr(i, 2));
        if (c < 0) { err = M("eso no es una carta: '", "not a card: '") + s.substr(i, 2) + "'"; return false; }
        for (int prev : out)
            if (prev == c) { err = M("carta repetida: ", "duplicated card: ") + card_str(c); return false; }
        out.push_back(c);
    }
    return true;
}

inline std::string board_str(const std::vector<int>& b) {
    std::string s;
    for (size_t i = 0; i < b.size(); ++i) {
        if (i) s += " ";
        s += card_str(b[i]);
    }
    return s;
}

// -----------------------------------------------------------------------------
//  Five-card scoring
//
//  score = category<<20 | k1<<16 | k2<<12 | k3<<8 | k4<<4 | k5
//  Kickers are rank indices, most significant first, so a plain integer compare
//  ranks two hands correctly and equal integers mean a genuine chop.
// -----------------------------------------------------------------------------
enum HandCat {
    HC_HIGH = 0, HC_PAIR, HC_TWOPAIR, HC_TRIPS, HC_STRAIGHT,
    HC_FLUSH, HC_FULL, HC_QUADS, HC_STRFLUSH
};

static const char* const HC_NAME[9] = {
    "high card", "pair", "two pair", "trips", "straight",
    "flush", "full house", "quads", "straight flush"
};

inline int eval5(const int* c) {
    int cnt[13] = { 0 }, suits[4] = { 0 }, mask = 0;
    for (int i = 0; i < 5; ++i) {
        cnt[card_rank(c[i])]++;
        suits[card_suit(c[i])]++;
        mask |= 1 << card_rank(c[i]);
    }

    bool flush = false;
    for (int s = 0; s < 4; ++s) if (suits[s] == 5) flush = true;

    int sh = -1;                                   // straight high rank
    for (int top = 12; top >= 4; --top)
        if (((mask >> (top - 4)) & 0x1F) == 0x1F) { sh = top; break; }
    if (sh < 0 && (mask & 0x100F) == 0x100F) sh = 3;   // wheel A-2-3-4-5

    // Ranks present, ordered by (count desc, rank desc). Insertion sort with a
    // strict comparison keeps equal counts in rank-descending order.
    int order[13], n = 0;
    for (int r = 12; r >= 0; --r) if (cnt[r]) order[n++] = r;
    for (int i = 1; i < n; ++i) {
        const int k = order[i];
        int j = i - 1;
        while (j >= 0 && cnt[order[j]] < cnt[k]) { order[j + 1] = order[j]; --j; }
        order[j + 1] = k;
    }

    int cat;
    if (flush && sh >= 0)                                          cat = HC_STRFLUSH;
    else if (cnt[order[0]] == 4)                                   cat = HC_QUADS;
    else if (cnt[order[0]] == 3 && n >= 2 && cnt[order[1]] == 2)   cat = HC_FULL;
    else if (flush)                                                cat = HC_FLUSH;
    else if (sh >= 0)                                              cat = HC_STRAIGHT;
    else if (cnt[order[0]] == 3)                                   cat = HC_TRIPS;
    else if (cnt[order[0]] == 2 && n >= 2 && cnt[order[1]] == 2)   cat = HC_TWOPAIR;
    else if (cnt[order[0]] == 2)                                   cat = HC_PAIR;
    else                                                           cat = HC_HIGH;

    int k[5] = { 0, 0, 0, 0, 0 };
    if (cat == HC_STRFLUSH || cat == HC_STRAIGHT) {
        k[0] = sh;                                  // only the top card matters
    } else {
        for (int i = 0; i < n && i < 5; ++i) k[i] = order[i];
    }
    return (cat << 20) | (k[0] << 16) | (k[1] << 12) | (k[2] << 8) | (k[3] << 4) | k[4];
}

// Best five-card hand out of seven, by excluding each pair of cards in turn.
inline int eval7(const int* c7) {
    int best = -1, h[5];
    for (int x = 0; x < 7; ++x)
        for (int y = x + 1; y < 7; ++y) {
            int n = 0;
            for (int i = 0; i < 7; ++i) if (i != x && i != y) h[n++] = c7[i];
            const int v = eval5(h);
            if (v > best) best = v;
        }
    return best;
}

// La mejor de cinco entre las que haya: cinco, seis o siete. Un flop son cinco
// cartas con las dos del jugador, un turn seis y un river siete, y nombrar la
// mano hecha tiene que funcionar en las tres.
inline int eval_best(const int* c, int n) {
    if (n == 5) return eval5(c);
    if (n == 7) return eval7(c);
    int best = -1, h[5];
    for (int x = 0; x < n; ++x) {
        int m = 0;
        for (int i = 0; i < n; ++i) if (i != x) h[m++] = c[i];
        if (m != 5) continue;
        const int v = eval5(h);
        if (v > best) best = v;
    }
    return best;
}

inline int score_category(int score) { return score >> 20; }

// -----------------------------------------------------------------------------
//  La mano hecha, nombrada como la nombra la referencia.
//
//  Las nueve categorias del evaluador no sirven para estudiar: meten un overpair
//  y un par de doses en el mismo cajon, y esa es justo la distincion que decide
//  la mano. Lo que hace falta es RELATIVO AL BOARD, porque es asi como se piensa
//  un rango.
//
//  Los nombres y la escalera son LOS DLA REFERENCIA, y no de oido. Trae dos comandos
//  que su manual no menciona, `show_category_names` y `show_cats_pp <board>`, y
//  con el segundo se le pregunto por 260 boards. Su tabla, de mas floja a mas
//  fuerte, es esta:
//
//     nothing king_high ace_high low_pair 3rd-pair 2nd-pair underpair
//     top_pair top_pair_tp overpair two_pair trips set straight flush
//     fullhouse top_fullhouse quads straight_flush
//
//  Diecinueve nombres. `top_fullhouse` tardo en aparecer porque hace falta una
//  forma que el azar no da -- board de trio con la pareja MAS ALTA que el trio,
//  como 2-2-2-3-3 --; `top_pair_tp` no ha aparecido todavia en ningun board, ni
//  por forma ni al azar. Quedan dieciocho, que son los de aqui, en su orden.
//
//  Y debajo hay UNA sola idea, aplicada sin excepciones: la categoria es lo que
//  ponen TUS DOS CARTAS. Se toma la mejor mano de cinco entre las siete y, de
//  las que empatan, la que use MENOS cartas tuyas -- si el board se basta, la
//  jugada no es tuya. Despues se mira que grupo (pareja, trio, poker) de esas
//  cinco lleva una carta tuya:
//
//     ninguno            ->  carta alta: ace_high si tu as sigue en las cinco
//                            de patada, king_high si es tu rey, nothing si no
//     una pareja tuya    ->  se nombra por POSICION contra los rangos del board
//     dos parejas tuyas  ->  two_pair, y solo si el board viene sin pareja
//     un trio            ->  set si pusiste dos cartas, trips si pusiste una
//
//  La posicion se cuenta contra los rangos DISTINTOS del board, de mayor a
//  menor: ligar el primero es top_pair, el segundo 2nd-pair, el tercero
//  3rd-pair, el cuarto o el quinto low_pair. Una pareja servida se coloca por
//  cuantos rangos del board le quedan por encima: ninguno overpair, uno
//  underpair, dos 3rd-pair, tres o mas low_pair.
//
//  De ahi salen cosas que son suyas y que yo no habria puesto:
//
//    - el rey suelto tiene cajon propio, igual que el as
//    - en A-9-4 el 88 y el 77 caen con el que liga el 4 (dos rangos por
//      encima), pero el KK y el QQ tienen el suyo (uno por encima)
//    - en K-K-7-7-2 un 33 es `nothing`: las cinco son K,K,7,7,3 y tu pareja no
//      entra; con un as de patada la misma mano es `ace_high`
//    - en 9-7-2-2 el 97 es `top_pair`, no `two_pair`: con el board emparejado
//      la referencia no reparte two_pair
//    - y si el BOARD ya es color el solito, no da color a nadie ni carta alta a
//      nadie: todo es `nothing` menos la escalera de color, incluso el as del
//      palo. Eso es lo que hace, se comprobo a mano, y se copia tal cual.
//
//  Solo mano hecha. Los proyectos son otra cosa: dependen de las cartas que
//  faltan, no existen en el river, y llevan su propia logica.
// -----------------------------------------------------------------------------
enum MadeCat {
    MC_STRFLUSH = 0, MC_QUADS, MC_TOPFULL, MC_FULL, MC_FLUSH, MC_STRAIGHT,
    MC_SET, MC_TRIPS, MC_TWOPAIR,
    MC_OVERPAIR, MC_TOPPAIR, MC_UNDERPAIR, MC_SECONDPAIR, MC_THIRDPAIR,
    MC_LOWPAIR, MC_ACEHIGH, MC_KINGHIGH, MC_NOTHING, MC_COUNT
};

// Con guion BAJO en 2nd_pair y 3rd_pair, que es como los escribe su Range
// Explorer, que es la pantalla contra la que se compara esto. Su
// `show_category_names` los devuelve con guion normal, "2nd-pair", asi que ella
// mismo no se pone de acuerdo consigo mismo; se elige lo que el usuario VE.
static const char* const MC_NAME[MC_COUNT] = {
    "straight_flush", "quads", "top_fullhouse", "fullhouse", "flush", "straight",
    "set", "trips", "two_pair",
    "overpair", "top_pair", "underpair", "2nd_pair", "3rd_pair",
    "low_pair", "ace_high", "king_high", "nothing"
};

// La mejor mano de cinco entre `n` cartas (5, 6 o 7), donde las DOS PRIMERAS son
// las del jugador. De las que empatan devuelve la que use menos cartas suyas, y
// ese desempate es media regla: en K-K-7-7-5 con 5c2c las cinco son K,K,7,7,5 y
// el 5 que entra es el del board, no el tuyo -- por eso esa mano no es pareja de
// cincos, es nada. Deja las cinco elegidas en `sale`.
inline int best_five(const int* c, int n, int* sale) {
    int mejor = -1, mejor_mias = 3;
    for (int m = 0; m < (1 << n); ++m) {
        int k = 0, mias = 0, h[5];
        for (int i = 0; i < n; ++i)
            if (m & (1 << i)) {
                if (k == 5) { k = 6; break; }
                h[k++] = c[i];
                if (i < 2) ++mias;
            }
        if (k != 5) continue;
        const int v = eval5(h);
        if (v > mejor || (v == mejor && mias < mejor_mias)) {
            mejor = v;
            mejor_mias = mias;
            for (int i = 0; i < 5; ++i) sale[i] = h[i];
        }
    }
    return mejor;
}

// Full normal o full nuts. La referencia los separa, y solo en un sitio: cuando el BOARD
// ya es full el solito -- trio de un rango y pareja de otro, cinco cartas -- y
// tu carta sube el TRIO al rango de esa pareja.
//
// En 2-2-2-3-3 un tres te deja 3-3-3-2-2, que es el mejor full que hay en ese
// board, y la referencia lo llama top_fullhouse; un 44 te deja 2-2-2-4-4, que es full y
// nada mas. Y en 3-3-3-2-2 no existe el cajon: subir el trio al dos seria
// bajarlo, asi que ahi hasta el AA es fullhouse. Comprobado en 2-2-2-3-3,
// 2-2-2-A-A, 7-7-7-9-9 y K-K-K-A-A, y comprobado que NO sale con el trio arriba
// ni con el board a dos parejas ni con cuatro cartas.
//
// Este cajon estuvo dias sin aparecer: 652 boards al azar y ni uno, porque el
// azar no da trios con pareja mas alta. Salio al barrer FORMAS en vez de
// cartas.
inline int full_o_nuts(const std::vector<int>& board, const int* cuenta) {
    if (board.size() < 5) return MC_FULL;
    int cb[13] = { 0 };
    for (int b : board) ++cb[card_rank(b)];
    int trio_board = -1, par_board = -1;
    for (int r = 12; r >= 0; --r) {
        if (cb[r] >= 3 && trio_board < 0)      trio_board = r;
        else if (cb[r] >= 2 && par_board < 0)  par_board = r;
    }
    if (trio_board < 0 || par_board < 0) return MC_FULL;
    for (int r = 12; r >= 0; --r)
        if (cuenta[r] >= 3) return r > trio_board ? MC_TOPFULL : MC_FULL;
    return MC_FULL;
}

// La categoria de `c1 c2` sobre `board`. El board puede tener 3, 4 o 5 cartas.
inline int made_cat(int c1, int c2, const std::vector<int>& board) {
    int siete[7];
    siete[0] = c1;
    siete[1] = c2;
    for (size_t i = 0; i < board.size() && i < 5; ++i) siete[2 + i] = board[i];
    const int n = 2 + static_cast<int>(board.size());

    int cinco[5];
    const int cat = score_category(best_five(siete, n, cinco));

    // Que hay en esas cinco y cuanto de ello es mio.
    int cuenta[13] = { 0 }, mias_de[13] = { 0 };
    bool mia_dentro = false;
    int alta_mia = -1;
    for (int i = 0; i < 5; ++i) {
        const int r = card_rank(cinco[i]);
        ++cuenta[r];
        if (cinco[i] == c1 || cinco[i] == c2) {
            mia_dentro = true;
            ++mias_de[r];
            if (r > alta_mia) alta_mia = r;
        }
    }

    // El board que ya es color el solito: la referencia no reparte nada mas que la
    // escalera de color, ni siquiera carta alta.
    {
        int su[4] = { 0, 0, 0, 0 };
        for (int b : board) ++su[card_suit(b)];
        for (int s = 0; s < 4; ++s)
            if (su[s] >= 5)
                return (cat == HC_STRFLUSH && mia_dentro) ? MC_STRFLUSH : MC_NOTHING;
    }

    // Mi grupo mas alto dentro de las cinco: la pareja, el trio o el poker mas
    // alto que lleve una carta mia. `grupos_mios` cuenta todos, que es lo que
    // separa la doble pareja de la pareja sola.
    int r_mio = -1, mias_del_grupo = 0, grupos_mios = 0;
    for (int r = 12; r >= 0; --r)
        if (cuenta[r] >= 2 && mias_de[r] > 0) {
            ++grupos_mios;
            if (r_mio < 0) {
                r_mio = r;
                mias_del_grupo = mias_de[r];
            }
        }

    // Escalera, color y escalera de color son las cinco cartas enteras: basta
    // con que una sea mia. El poker y el full necesitan que la carta mia este en
    // el grupo, no de patada -- en 6-6-6-6-2 con un as las cinco son 6,6,6,6,A y
    // ese poker no es mio, es carta alta.
    switch (cat) {
        case HC_STRFLUSH: if (mia_dentro) return MC_STRFLUSH; break;
        case HC_QUADS:    if (r_mio >= 0)  return MC_QUADS;    break;
        case HC_FULL:     if (r_mio >= 0)  return full_o_nuts(board, cuenta);
                          break;
        case HC_FLUSH:    if (mia_dentro) return MC_FLUSH;     break;
        case HC_STRAIGHT: if (mia_dentro) return MC_STRAIGHT;  break;
        default: break;
    }

    // Los rangos distintos del board, de mayor a menor: la referencia contra la
    // que se mide la posicion.
    std::vector<int> br;
    bool board_emparejado = false;
    for (int b : board) {
        const int r = card_rank(b);
        bool ya = false;
        for (int x : br) if (x == r) ya = true;
        if (ya) board_emparejado = true;
        else    br.push_back(r);
    }
    std::sort(br.begin(), br.end(), std::greater<int>());

    if (r_mio >= 0) {
        // Trio: set si puse las dos cartas, trips si puse una.
        if (cuenta[r_mio] >= 3) return mias_del_grupo >= 2 ? MC_SET : MC_TRIPS;

        // Doble pareja, solo con el board sin emparejar.
        if (grupos_mios >= 2 && !board_emparejado) return MC_TWOPAIR;

        // Una pareja: si el rango esta en el board, por su posicion; si no, es
        // una pareja servida y se coloca por los rangos que le quedan encima.
        for (size_t i = 0; i < br.size(); ++i)
            if (br[i] == r_mio)
                return i == 0 ? MC_TOPPAIR : i == 1 ? MC_SECONDPAIR
                     : i == 2 ? MC_THIRDPAIR : MC_LOWPAIR;
        int arriba = 0;
        for (int x : br) if (x > r_mio) ++arriba;
        return arriba == 0 ? MC_OVERPAIR : arriba == 1 ? MC_UNDERPAIR
             : arriba == 2 ? MC_THIRDPAIR : MC_LOWPAIR;
    }

    // Nada mio en las cinco mas que la patada. El as y el rey van aparte:
    // bloquean, y ganan botes sin mejorar. Y si la jugada de las cinco no deja
    // sitio para una patada -- una escalera del board, por ejemplo -- entonces
    // ni eso, y es nada.
    if (alta_mia == 12) return MC_ACEHIGH;
    if (alta_mia == 11) return MC_KINGHIGH;
    return MC_NOTHING;
}

// =============================================================================
//  Proyectos: lo que la mano TODAVIA no es.
//
//  La otra mitad de la pregunta. Sin esto, en un board con dos del mismo palo,
//  un proyecto de color nut figura como "carta alta" al lado del 72o, y la fila
//  dice que un tercio del rango es aire cuando ese tercio apuesta el 78% de las
//  veces. Lo que se ve entonces no es informacion, es ruido con etiqueta.
//
//  Los cinco cajones y sus nombres son los estandar, y el suyo dice las salidas en
//  vez del mote: un proyecto de escalera de una sola carta son cuatro cartas y
//  el de dos son ocho. Y lo que la referencia hace, comprobado contra el:
//
//    - un proyecto tiene que ser DE LA MANO. La escalera cuenta solo si el rango
//      que la completa forma una escalera que pasa por una carta tuya que el
//      board no tiene. En Q-J-T-9 un A2 es 4out_straight_draw, porque el rey te
//      da A-K-Q-J-T: el ocho tambien da escalera, pero esa es de todos y no es
//      tuya.
//    - `combo_draw` es color MAS escalera, y le vale la tripa: en A-9-4 de
//      corazones un 3h2c es combo_draw, no flush_draw.
//    - con color ya hecho no hay proyecto de nada, ni de escalera. Con escalera
//      hecha si queda el proyecto de color.
//    - las salidas se cuentan en CARTAS, no en rangos, y con el board a cuatro
//      del mismo palo la carta de ese palo no cuenta: te liga la escalera y le
//      liga el color a otro. En Q-9-6-2 con 8-7 hay dos rangos de salida; con el
//      cuarto diamante en el board son seis cartas y no ocho, y la misma mano
//      pasa de 8out a 4out. En A-9-4-2 de un solo palo no queda ni un proyecto.
// =============================================================================
enum DrawCat {
    DR_NONE = 0, DR_GUT, DR_OESD, DR_FLUSH, DR_FLUSH_OESD, DR_COUNT
};

// Los nombres son los de la referencia, que se los pidio `show_category_names`.
static const char* const DR_NAME[DR_COUNT] = {
    "no_draw", "4out_straight_draw", "8out_straight_draw",
    "flush_draw", "combo_draw"
};

// Las diez ventanas de escalera como mascaras de rango, DE MENOR A MAYOR: la
// rueda (A-2-3-4-5) primero, que es la mas floja, y A-K-Q-J-T al final. El orden
// importa porque para saber si un proyecto es tuyo hay que mirar la mas alta.
inline const int* straight_windows() {
    static int w[10];
    static bool listo = false;
    if (!listo) {
        w[0] = (1 << 12) | (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
        for (int lo = 0; lo <= 8; ++lo) w[1 + lo] = 0x1F << lo;
        listo = true;
    }
    return w;
}

// Cuantos RANGOS completan una escalera QUE ES TUYA. La carta que la completa
// la completa para todos, asi que lo que decide es la MAS ALTA que quede hecha:
// si esa pasa por un rango que llevas tu y el board no tiene, el proyecto es
// tuyo; si no, la escalera que sale es del board y tu no ganas nada.
//
// En Q-J-T-9, con un A2 el rey da A-K-Q-J-T y es tuya: cuenta. Con un 72 el
// ocho da Q-J-T-9-8, que es de todos, y aunque a ti te deje 7-8-9-T-J eso ya no
// vale nada: no cuenta. Se cuentan rangos y no cartas porque lo que importa es
// de cuantos sitios puede venir; la referencia los nombra por las cartas, cuatro y ocho.
inline int straight_outs_mios(int m_board, int m_mano) {
    const int* w = straight_windows();
    const int m_all = m_board | m_mano;
    const int solo_mio = m_mano & ~m_board;
    int n = 0;
    for (int r = 0; r < 13; ++r) {
        if (m_all & (1 << r)) continue;
        const int con = m_all | (1 << r);
        int alta = -1;
        for (int i = 0; i < 10; ++i) if ((con & w[i]) == w[i]) alta = i;
        if (alta >= 0 && (w[alta] & solo_mio)) ++n;
    }
    return n;
}

inline int draw_cat(int c1, int c2, const std::vector<int>& board) {
    // En el river no queda carta por salir: lo que la mano no es, ya no sera.
    if (board.size() >= 5) return DR_NONE;

    int suit_all[4] = { 0, 0, 0, 0 }, suit_board[4] = { 0, 0, 0, 0 };
    int m_board = 0, m_mano = 0;
    for (int b : board) {
        ++suit_all[card_suit(b)];
        ++suit_board[card_suit(b)];
        m_board |= 1 << card_rank(b);
    }
    const int mano[2] = { c1, c2 };
    for (int k = 0; k < 2; ++k) {
        ++suit_all[card_suit(mano[k])];
        m_mano |= 1 << card_rank(mano[k]);
    }

    // Con color hecho o mejor no hay nada que esperar. Con la escalera hecha si
    // queda el color, y por eso el corte esta en el color y no en la escalera.
    const int hecho = made_cat(c1, c2, board);
    if (hecho <= MC_FLUSH) return DR_NONE;

    // Cuatro del mismo palo y al menos una puesta por la mano.
    bool color = false;
    for (int su = 0; su < 4; ++su)
        if (suit_all[su] == 4 && suit_board[su] < 4) color = true;

    // Las salidas, en CARTAS. Y si el board ya viene con cuatro del mismo palo,
    // la carta de ese palo que te liga la escalera le liga el color a alguien y
    // no cuenta: quedan tres por rango en vez de cuatro. De ahi que las mismas
    // dos salidas sean 8out en Q-9-6-2 con un treboles y 4out con el cuarto
    // diamante -- ocho cartas contra seis -- y que en A-9-4-2 de un solo palo no
    // quede un solo proyecto en pie.
    int por_rango = 4;
    for (int su = 0; su < 4; ++su) if (suit_board[su] >= 4) por_rango = 3;
    const int rangos = hecho == MC_STRAIGHT ? 0
                                            : straight_outs_mios(m_board, m_mano);
    const int cartas = rangos * por_rango;
    const bool abierta = cartas >= 8;
    const bool tripa   = cartas >= 4;

    if (color && (abierta || tripa)) return DR_FLUSH_OESD;
    if (color)                       return DR_FLUSH;
    if (abierta)                     return DR_OESD;
    if (tripa)                       return DR_GUT;
    return DR_NONE;
}

// Como se describe UN COMBO, no una familia. Las dos mitades sumadas
// ("top_pair + flush_draw") valen para decir que es esa mano concreta, y es lo
// que sale en la tabla de combos y en el recuadro del raton.
//
// Lo que NO vale es usar esto para agrupar: la referencia no tiene esa familia. Sus dos
// listas son independientes y "un set con proyecto de color" se pide con
// `set & flush_draw` en su lenguaje de filtros. El reparto por categorias va
// por agg_name(), en dos bloques que no se cruzan.
inline std::string made_draw_name(int hecho, int proyecto) {
    if (proyecto == DR_NONE || proyecto >= DR_COUNT) return MC_NAME[hecho];
    // Las dos mitades se suman y no se resumen, que es lo que hace la referencia: el
    // suyo da SIEMPRE los dos indices, uno de mano hecha y otro de proyecto, y
    // `nothing + flush_draw` es literalmente lo que contesta. Juntarlas en una
    // sola palabra perderia que un proyecto de color con as y otro sin el no
    // son la misma mano ni en equity ni en lo que se hace con ella.
    return std::string(MC_NAME[hecho]) + " + " + DR_NAME[proyecto];
}


// Human-readable description of a made hand, e.g. "two pair, aces and sevens".
inline std::string score_str(int score) {
    const int cat = score_category(score);
    const int k0 = (score >> 16) & 0xF, k1 = (score >> 12) & 0xF, k2 = (score >> 8) & 0xF;
    std::string s = HC_NAME[cat];
    switch (cat) {
        case HC_STRFLUSH:
        case HC_STRAIGHT: s += std::string(", ") + RANK_CH[k0] + " high"; break;
        case HC_QUADS:    s += std::string(", ") + RANK_CH[k0] + "s"; break;
        case HC_FULL:     s += std::string(", ") + RANK_CH[k0] + "s full of " + RANK_CH[k1] + "s"; break;
        case HC_FLUSH:    s += std::string(", ") + RANK_CH[k0] + " high"; break;
        case HC_TRIPS:    s += std::string(", ") + RANK_CH[k0] + "s"; break;
        case HC_TWOPAIR:  s += std::string(", ") + RANK_CH[k0] + "s and " + RANK_CH[k1] + "s"; break;
        case HC_PAIR:     s += std::string(" of ") + RANK_CH[k0] + "s"; break;
        default:          s += std::string(", ") + RANK_CH[k0] + RANK_CH[k1] + RANK_CH[k2] + " high"; break;
    }
    return s;
}

// -----------------------------------------------------------------------------
//  169 starting-hand classes, laid out as the usual 13x13 grid.
//  Row/column 0 is aces; suited combos live above the diagonal, offsuit below.
// -----------------------------------------------------------------------------
inline int class_index(int c1, int c2) {
    const int r1 = card_rank(c1), r2 = card_rank(c2);
    const int hi = std::max(r1, r2), lo = std::min(r1, r2);
    const int i = 12 - hi, j = 12 - lo;            // 0 = ace
    if (r1 == r2) return i * 13 + j;               // pair, on the diagonal
    return (card_suit(c1) == card_suit(c2)) ? (i * 13 + j) : (j * 13 + i);
}

inline std::string class_name(int idx) {
    const int i = idx / 13, j = idx % 13;
    const int r1 = 12 - i, r2 = 12 - j;
    if (i == j) return std::string(1, RANK_CH[r1]) + RANK_CH[r1];
    if (i < j)  return std::string(1, RANK_CH[r1]) + RANK_CH[r2] + "s";
    return std::string(1, RANK_CH[r2]) + RANK_CH[r1] + "o";
}

// How many concrete combos a class holds before any card removal.
inline int class_combos(int idx) {
    const int i = idx / 13, j = idx % 13;
    if (i == j) return 6;    // pair
    if (i < j)  return 4;    // suited
    return 12;               // offsuit
}
