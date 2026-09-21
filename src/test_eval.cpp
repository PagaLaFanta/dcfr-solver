// Evaluator sanity checks.  g++ -std=c++17 -O2 -static -o test_eval src/test_eval.cpp
#include "cards.hpp"
#include <cstdio>
#include <string>
#include <vector>

static int fails = 0;

static std::vector<int> cards(const std::string& s) {
    std::vector<int> v;
    std::string e;
    if (!parse_board(s, v, e)) { std::printf("PARSE FAIL %s: %s\n", s.c_str(), e.c_str()); ++fails; }
    return v;
}

static int ev(const std::string& s) {
    std::vector<int> v = cards(s);
    if (v.size() == 5) return eval5(v.data());
    if (v.size() == 7) return eval7(v.data());
    std::printf("BAD SIZE %s\n", s.c_str());
    ++fails;
    return -1;
}

static void cat_is(const std::string& h, HandCat want) {
    const int s = ev(h);
    if (score_category(s) != static_cast<int>(want)) {
        std::printf("FAIL cat %-16s got %-16s want %s\n", h.c_str(),
                    HC_NAME[score_category(s)], HC_NAME[want]);
        ++fails;
    }
}

static void gt(const std::string& a, const std::string& b) {
    const int x = ev(a), y = ev(b);
    if (!(x > y)) {
        std::printf("FAIL %s (%s) should beat %s (%s)\n", a.c_str(), score_str(x).c_str(),
                    b.c_str(), score_str(y).c_str());
        ++fails;
    }
}

static void eq(const std::string& a, const std::string& b) {
    const int x = ev(a), y = ev(b);
    if (x != y) {
        std::printf("FAIL %s (%s) should chop with %s (%s)\n", a.c_str(), score_str(x).c_str(),
                    b.c_str(), score_str(y).c_str());
        ++fails;
    }
}

int main() {
    // ---- categories --------------------------------------------------------
    cat_is("AsKsQsJsTs", HC_STRFLUSH);
    cat_is("5s4s3s2sAs", HC_STRFLUSH);   // steel wheel
    cat_is("7c7d7h7sKc", HC_QUADS);
    cat_is("7c7d7hKsKc", HC_FULL);
    cat_is("As9s7s4s2s", HC_FLUSH);
    cat_is("9c8d7h6s5c", HC_STRAIGHT);
    cat_is("5c4d3h2sAc", HC_STRAIGHT);   // wheel, not ace-high anything
    cat_is("7c7d7hKs2c", HC_TRIPS);
    cat_is("7c7dKhKs2c", HC_TWOPAIR);
    cat_is("7c7dKh9s2c", HC_PAIR);
    cat_is("AcKd9h7s2c", HC_HIGH);

    // ---- category ordering -------------------------------------------------
    gt("5s4s3s2sAs", "7c7d7h7sKc");      // steel wheel > quads
    gt("7c7d7h7sKc", "7c7d7hKsKc");      // quads > boat
    gt("7c7d7hKsKc", "As9s7s4s2s");      // boat > flush
    gt("As9s7s4s2s", "9c8d7h6s5c");      // flush > straight
    gt("9c8d7h6s5c", "7c7d7hKs2c");      // straight > trips
    gt("7c7d7hKs2c", "7c7dKhKs2c");      // trips > two pair
    gt("7c7dKhKs2c", "7c7dKh9s2c");      // two pair > pair
    gt("7c7dKh9s2c", "AcKd9h7s2c");      // pair > high card

    // ---- within-category ordering -----------------------------------------
    gt("9c8d7h6s5c", "5c4d3h2sAc");      // 9-high straight > wheel
    gt("6c5d4h3s2c", "5c4d3h2sAc");      // 6-high straight > wheel
    gt("As9s7s4s2s", "Ks9s7s4s2s");      // A-high flush > K-high flush
    gt("As9s7s4s3s", "As9s7s4s2s");      // last kicker decides a flush
    gt("AcAdKhQs2c", "AcAdKhJs2c");      // pair kicker
    gt("AcAdKhKs2c", "AcAdQhQs2c");      // higher two pair
    gt("AcAdKhKsQc", "AcAdKhKs2c");      // two-pair kicker
    gt("KcKdKhAsQc", "QcQdQhAsKc");      // higher trips beats higher kickers
    gt("AcAdAhKsKc", "KcKdKhAsAc");      // aces full > kings full
    gt("AcKd9h7s3c", "AcKd9h7s2c");      // fifth card decides high card

    // ---- chops -------------------------------------------------------------
    eq("AcKd9h7s2c", "AsKh9c7d2s");      // same ranks, different suits
    eq("AsKsQsJsTs", "AcKcQcJcTc");      // royal vs royal

    // ---- seven-card selection ---------------------------------------------
    // Board Ah Kd 7c 2s 9h.  AsKs makes two pair, 77 makes trips.
    gt("7d7h" "AhKd7c2s9h", "AsKs" "AhKd7c2s9h");
    // A player holding 5c3c on a 4h6d2c board with 8s Ts makes nothing;
    // holding 5c3c on 4h6d2cKsQs makes a 6-high straight.
    cat_is("5c3c" "4h6d2cKsQs", HC_STRAIGHT);
    // Four to a flush on board: any spade beats a non-spade holding.
    gt("2s3d" "AsKsQs9s4h", "AdAc" "AsKsQs9s4h");
    // Board plays: both players chop when neither improves the board.
    eq("2c3d" "AsAdAcKsKd", "2h3h" "AsAdAcKsKd");

    std::printf("\n%s  (%d failure%s)\n", fails ? "FAILED" : "ALL EVALUATOR TESTS PASSED",
                fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
