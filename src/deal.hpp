#pragma once
// =============================================================================
//  Deals across streets.
//
//  The board length picks the depth of the game:
//      5 cards -> river only
//      4 cards -> turn, then river
//      3 cards -> flop, then turn, then river
//
//  Every reach vector in the solver is indexed over the combos that survive the
//  BASE board, so one indexing serves the whole tree. When a chance node deals a
//  card, the combos using it simply get zero reach from that point down.
//
//  Hand strength depends on the final five-card board, so it is precomputed once
//  per runout: the score of every combo plus the combos sorted by it, which is
//  what the O(N) showdown sweep walks.
//
//  SUIT ISOMORPHISM
//    Two runout cards are strategically identical when some permutation of the
//    suits maps the board onto itself and one card onto the other -- and the
//    ranges are invariant under it too, which is checked rather than assumed.
//    Only one card per orbit is solved and stored; the others are read back
//    through the permutation.
//
//    The permutations fixing a board are those that permute suits carrying
//    identical rank sets, so the win comes from the suits ABSENT from the board:
//    three free suits on a monotone flop (group of 6), two on a two-tone (group
//    of 2), one on a rainbow (group of 1 -- no saving at all).
// =============================================================================

#include "msg.hpp"
#include "cards.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

struct Combo {
    int c1 = -1;   // hole cards, c1 < c2
    int c2 = -1;
    int cls = 0;   // 0..168, index into the 13x13 grid
};

enum Street { ST_FLOP = 0, ST_TURN = 1, ST_RIVER = 2 };

static const char* const STREET_NAME[3] = { "flop", "turn", "river" };

// One equivalence class of runout cards, plus the permutation carrying the
// representative onto each member.
struct Orbit {
    int              rep_slot = -1;
    std::vector<int> member_slot;
    std::vector<int> member_perm;   // perm[i] maps deck[rep_slot] -> deck[member_slot[i]]
};

struct Deal {
    std::vector<int>   board;     // the base board: 3, 4 or 5 cards
    int                start = ST_RIVER;
    std::vector<Combo> combos;    // every combo compatible with the base board
    std::vector<int>   deck;      // cards not on the base board
    std::vector<int>   combo_at;  // 52*52 -> combo index, or -1

    // ---- per-runout hand strength ---------------------------------------
    int                num_runouts = 1;
    std::vector<int>   scores;    // [runout * nh + combo]
    std::vector<int>   order;     // [runout * nh + k] -> combo, weakest first
    std::vector<int>   pair_id;   // [i*D + j] -> runout id, for flop starts
    std::vector<int>   runout_slot;  // [runout*2] -> the two deck slots it deals
    std::vector<std::vector<int>> with_card;   // card -> combos that use it

    // ---- suit permutations ----------------------------------------------
    int  perm_suit[24][4];
    int  perm_card[24][52];
    int  perm_inv[24];
    int  perm_mul[24][24];                     // perm_mul[a][b] is "a after b"
    std::vector<std::vector<int>> perm_combo;  // [perm][combo] -> combo
    int  slot_of[52];                          // card -> deck slot, -1 if on the board

    // ---- isomorphic runout structure (rebuilt when the ranges change) -----
    bool                            iso_on = false;
    std::vector<int>                base_group;   // permutations fixing the base board
    std::vector<int>                use_group;    // the part of it the ranges survive
    std::vector<Orbit>              lvl1;         // first chance node
    std::vector<std::vector<Orbit>> lvl2;         // second chance node, per lvl1 orbit
    std::vector<long long>          lvl2_base;    // packing offsets for level-2 instances
    std::vector<int>                lvl1_order;   // orbits, costliest first
    long long                       inst[3] = { 1, 1, 1 };
    std::vector<int>                runout_by_inst;  // last-level instance -> runout id

    int num()      const { return static_cast<int>(combos.size()); }

    // What the precomputed tables weigh. Two ints per combo per runout for the
    // scores and the strength order is nearly all of it: on a flop that is
    // 2352 runouts by 1176 combos, 22 MB, and it is there before a single
    // regret has been allocated.
    long long table_bytes() const {
        const long long n = num(), R = num_runouts;
        long long b = 0;
        b += 2 * R * n * static_cast<long long>(sizeof(int));          // scores, order
        b += n * static_cast<long long>(sizeof(Combo));               // combos
        b += 52LL * 52LL * static_cast<long long>(sizeof(int));       // combo_at
        b += 2 * n * static_cast<long long>(sizeof(int));             // with_card
        b += static_cast<long long>(perm_combo.size()) * n *
             static_cast<long long>(sizeof(int));                     // perm_combo
        return b;
    }
    int deckN()    const { return static_cast<int>(deck.size()); }
    int identity() const { return identity_; }

    int index_of(int a, int b) const {
        if (a < 0 || b < 0 || a >= 52 || b >= 52) return -1;
        return combo_at[static_cast<size_t>(a) * 52 + b];
    }
    bool blocks(int h, int c) const {
        const Combo& k = combos[static_cast<size_t>(h)];
        return k.c1 == c || k.c2 == c;
    }

    bool build(const std::vector<int>& board_cards, std::string& err) {
        if (board_cards.size() < 3 || board_cards.size() > 5) {
            err = "the board needs 3 (flop), 4 (turn) or 5 (river) cards";
            return false;
        }
        board = board_cards;
        start = (board.size() == 5) ? ST_RIVER : (board.size() == 4 ? ST_TURN : ST_FLOP);

        bool dead[52] = { false };
        for (int c : board) {
            if (c < 0 || c >= 52) { err = "bad board card"; return false; }
            if (dead[c]) { err = M("el board repite la carta ", "duplicated board card ") + card_str(c); return false; }
            dead[c] = true;
        }

        deck.clear();
        for (int c = 0; c < 52; ++c) if (!dead[c]) deck.push_back(c);

        combos.clear();
        combo_at.assign(52 * 52, -1);
        with_card.assign(52, std::vector<int>());
        for (size_t i = 0; i < deck.size(); ++i)
            for (size_t j = i + 1; j < deck.size(); ++j) {
                Combo k;
                k.c1  = deck[i];
                k.c2  = deck[j];
                k.cls = class_index(k.c1, k.c2);
                const int id = static_cast<int>(combos.size());
                combo_at[static_cast<size_t>(k.c1) * 52 + k.c2] = id;
                combo_at[static_cast<size_t>(k.c2) * 52 + k.c1] = id;
                with_card[static_cast<size_t>(k.c1)].push_back(id);
                with_card[static_cast<size_t>(k.c2)].push_back(id);
                combos.push_back(k);
            }

        build_perms();
        build_strength();
        build_orbits(std::vector<double>(), std::vector<double>(), false);
        return true;
    }

    // The extra cards a runout deals on top of the base board.
    void runout_cards(int runout, int& t, int& r) const {
        t = -1; r = -1;
        if (start == ST_RIVER) return;
        if (start == ST_TURN) { r = deck[static_cast<size_t>(runout)]; return; }
        t = deck[static_cast<size_t>(runout_slot[static_cast<size_t>(runout) * 2])];
        r = deck[static_cast<size_t>(runout_slot[static_cast<size_t>(runout) * 2 + 1])];
    }

    std::string runout_str(int runout) const {
        int t, r;
        runout_cards(runout, t, r);
        std::string s;
        if (t >= 0) s += card_str(t);
        if (r >= 0) { if (!s.empty()) s += " "; s += card_str(r); }
        return s;
    }

    // ---- suit isomorphism -------------------------------------------------
    // Permutations mapping the given card set onto itself.
    std::vector<int> stabilizer(const std::vector<int>& cards) const {
        std::vector<int> out;
        for (int p = 0; p < 24; ++p) {
            bool good = true;
            for (int c : cards) {
                const int pc = perm_card[p][c];
                bool found = false;
                for (int d : cards) if (d == pc) { found = true; break; }
                if (!found) { good = false; break; }
            }
            if (good) out.push_back(p);
        }
        return out;
    }

    // Does this one permutation leave a per-combo vector where it found it?
    bool perm_keeps(int p, const std::vector<double>& r) const {
        if (p == identity_) return true;
        if (static_cast<int>(r.size()) != num()) return false;
        const std::vector<int>& pc = perm_combo[static_cast<size_t>(p)];
        for (int h = 0; h < num(); ++h) {
            const double a = r[static_cast<size_t>(h)];
            const double b = r[static_cast<size_t>(pc[static_cast<size_t>(h)])];
            if (a - b > 1e-9 || b - a > 1e-9) return false;
        }
        return true;
    }

    // The part of a group that both ranges survive.
    //
    // Filtering a group by a condition closed under composition and inverse
    // leaves a subgroup, so this is one, and collapsing runouts by it is legal
    // by exactly the same argument as collapsing by the whole group: every
    // permutation in it fixes the board and moves neither range.
    std::vector<int> keep_part(const std::vector<int>& group,
                               const std::vector<double>& r) const {
        std::vector<int> out;
        for (int p : group) if (perm_keeps(p, r)) out.push_back(p);
        return out;
    }
    std::vector<int> range_part(const std::vector<int>& group,
                                const std::vector<double>& r0,
                                const std::vector<double>& r1,
                                const std::vector<std::vector<double>>* extra = nullptr) const {
        std::vector<int> out = keep_part(keep_part(group, r0), r1);
        // One at a time, never merged into a single mask. Two locks that cover
        // suit twins but pin them to different actions have a symmetric union
        // and are not symmetric at all, and a merged mask cannot tell.
        if (extra) for (const std::vector<double>& m : *extra) out = keep_part(out, m);
        return out;
    }

    // What build_orbits() would settle on, worked out without building it. The
    // session needs this to answer "would rebuilding change what is collapsed?"
    // before it decides to throw a solve away.
    std::vector<int> group_for(const std::vector<double>& r0,
                               const std::vector<double>& r1,
                               const std::vector<std::vector<double>>* extra,
                               bool want_iso) const {
        if (!want_iso) return std::vector<int>(1, identity_);
        std::vector<int> g = range_part(stabilizer(board), r0, r1, extra);
        if (g.size() < 2) return std::vector<int>(1, identity_);
        return g;
    }

    // A range only qualifies for the whole group if nothing was filtered out:
    // "AKs" does, a hand-picked "AsKd:0.5" does not.
    bool range_symmetric(const std::vector<int>& group, const std::vector<double>& r) const {
        if (static_cast<int>(r.size()) != num()) return false;
        for (int p : group) if (!perm_keeps(p, r)) return false;
        return true;
    }

    // Splits available deck slots into orbits under `group`.
    std::vector<Orbit> orbits_of(const std::vector<int>& avail,
                                 const std::vector<int>& group) const {
        std::vector<uint8_t> is_avail(static_cast<size_t>(deckN()), 0u);
        for (int s : avail) is_avail[static_cast<size_t>(s)] = 1u;
        std::vector<uint8_t> seen(static_cast<size_t>(deckN()), 0u);
        std::vector<Orbit> out;
        for (int s : avail) {
            if (seen[static_cast<size_t>(s)]) continue;
            Orbit o;
            o.rep_slot = s;
            for (int p : group) {
                const int c2 = perm_card[p][deck[static_cast<size_t>(s)]];
                const int s2 = slot_of[c2];
                if (s2 < 0 || !is_avail[static_cast<size_t>(s2)]) continue;
                if (seen[static_cast<size_t>(s2)]) continue;
                seen[static_cast<size_t>(s2)] = 1u;
                o.member_slot.push_back(s2);
                o.member_perm.push_back(p);
            }
            out.push_back(o);
        }
        return out;
    }

    // Rebuilds the runout structure.
    //
    // Two runouts may be collapsed only under a permutation that fixes the
    // board AND moves neither range. This used to be all-or-nothing: if any
    // permutation of the board's group failed the range test, collapsing was
    // switched off entirely. That threw away symmetry the range did survive --
    // on `Ks 7s 2s` with a range holding no diamonds it cost 3.5x the nodes and
    // 2.3x the time, for a group that still had a legal half. So take the part
    // of the group the ranges survive, and collapse by that.
    void build_orbits(const std::vector<double>& r0, const std::vector<double>& r1,
                      bool want_iso,
                      const std::vector<std::vector<double>>* extra = nullptr) {
        base_group = stabilizer(board);
        use_group  = group_for(r0, r1, extra, want_iso);
        iso_on = use_group.size() > 1;
        const std::vector<int>& grp = use_group;

        lvl1.clear();
        lvl2.clear();
        lvl2_base.clear();
        runout_by_inst.clear();
        inst[0] = 1; inst[1] = 1; inst[2] = 1;

        if (start == ST_RIVER) { runout_by_inst.assign(1, 0); return; }

        std::vector<int> all;
        for (int s = 0; s < deckN(); ++s) all.push_back(s);
        lvl1 = orbits_of(all, grp);
        inst[1] = static_cast<long long>(lvl1.size());

        lvl1_order.resize(lvl1.size());
        for (size_t i = 0; i < lvl1.size(); ++i) lvl1_order[i] = static_cast<int>(i);

        if (start == ST_TURN) {
            runout_by_inst.resize(lvl1.size());
            for (size_t i = 0; i < lvl1.size(); ++i)
                runout_by_inst[i] = lvl1[i].rep_slot;   // the river slot IS the runout id
            return;
        }

        // Flop start: once the turn representative is fixed, the group shrinks
        // to whatever still fixes the four-card board.
        lvl2.resize(lvl1.size());
        lvl2_base.assign(lvl1.size(), 0);
        long long acc = 0;
        for (size_t t = 0; t < lvl1.size(); ++t) {
            const int ts = lvl1[t].rep_slot;
            std::vector<int> b4 = board;
            b4.push_back(deck[static_cast<size_t>(ts)]);
            // The group of the FOUR-card board, which is not a subgroup of the
            // board's: `Ks Kh 2s` with a `2h` turn admits the swap (s h), which
            // the flop alone does not. That makes it a real symmetry of the
            // river -- only the five-card set matters there -- but it is one the
            // ranges never had to survive, and collapsing by it read a range's
            // spades back as its hearts. On a hearts-free range that put the
            // mirror of the range at every spade river, moved the game value by
            // 0.09 chips and drove the reported exploitability NEGATIVE, which
            // is the shape of an impossible answer. So filter here too.
            const std::vector<int> g2 = iso_on
                    ? range_part(stabilizer(b4), r0, r1, extra)
                    : std::vector<int>(1, identity_);
            std::vector<int> avail;
            for (int s = 0; s < deckN(); ++s) if (s != ts) avail.push_back(s);
            lvl2[t] = orbits_of(avail, g2);
            lvl2_base[t] = acc;
            acc += static_cast<long long>(lvl2[t].size());
        }
        inst[2] = acc;

        // Hand the fan-out to the threads costliest first. On a monotone flop a
        // heart turn leaves 22 river orbits and a non-heart 35, so starting a
        // 35 last would leave the whole pool waiting on one thread.
        std::sort(lvl1_order.begin(), lvl1_order.end(),
                  [this](int a, int b) {
                      return lvl2[static_cast<size_t>(a)].size() >
                             lvl2[static_cast<size_t>(b)].size();
                  });

        runout_by_inst.assign(static_cast<size_t>(acc), 0);
        for (size_t t = 0; t < lvl1.size(); ++t)
            for (size_t k = 0; k < lvl2[t].size(); ++k)
                runout_by_inst[static_cast<size_t>(lvl2_base[t]) + k] =
                    pair_id[static_cast<size_t>(lvl1[t].rep_slot) * deckN() +
                            lvl2[t][k].rep_slot];
    }

    // The reverse of locate(): given a stored instance, which instance does each
    // context above it live in, and which deck slot was dealt on the way.
    //
    // Reading a node has to walk the reaches down from the root, zeroing the
    // combos that use each dealt card. Doing that needs the dealt slots, and
    // the obvious way to get them -- treating the instance index as a base-deckN
    // number, which is how it is packed with isomorphism off -- is simply wrong
    // once orbits are collapsed, because then the index counts orbits instead of
    // cards. Everything here is in the STORED frame, which is the frame the walk
    // runs in; the caller rotates to the requested runout afterwards.
    //  `levels` is how many cards were dealt to get there, which is what says
    //  whether the index counts first-level orbits or second-level ones. A turn
    //  node inside a flop solve is one level deep even though the solve is two,
    //  and reading it as a second-level index lands on an unrelated runout.
    bool decompose(long long final_inst, int levels,
                   std::vector<long long>& insts, std::vector<int>& slots) const {
        insts.clear();
        slots.clear();
        insts.push_back(0);                       // the starting street
        if (levels <= 0) return final_inst == 0;

        if (levels == 1) {
            if (final_inst < 0 || final_inst >= static_cast<long long>(lvl1.size()))
                return false;
            slots.push_back(lvl1[static_cast<size_t>(final_inst)].rep_slot);
            insts.push_back(final_inst);
            return true;
        }

        if (levels != 2 || lvl2.empty()) return false;
        if (final_inst < 0 || final_inst >= inst[2]) return false;
        for (size_t t = 0; t < lvl1.size(); ++t) {
            const long long lo = lvl2_base[t];
            const long long hi = lo + static_cast<long long>(lvl2[t].size());
            if (final_inst < lo || final_inst >= hi) continue;
            slots.push_back(lvl1[t].rep_slot);
            insts.push_back(static_cast<long long>(t));
            slots.push_back(lvl2[t][static_cast<size_t>(final_inst - lo)].rep_slot);
            insts.push_back(final_inst);
            return true;
        }
        return false;
    }

    // How many real runouts a stored instance stands for. One with suits not
    // collapsed; with them collapsed, the size of its orbit -- and at the second
    // level, the product, because each member of the first orbit carries the
    // whole second-level structure with it. Anything averaging "across the
    // board" has to weight by this, or a monotone flop, where orbits differ in
    // size by more than three to one, comes out skewed.
    long long orbit_weight(long long stored, int levels) const {
        if (levels <= 0) return 1;
        if (levels == 1) {
            if (stored < 0 || stored >= static_cast<long long>(lvl1.size())) return 0;
            return static_cast<long long>(lvl1[static_cast<size_t>(stored)].member_slot.size());
        }
        if (levels != 2 || lvl2.empty()) return 0;
        for (size_t t = 0; t < lvl1.size(); ++t) {
            const long long lo = lvl2_base[t];
            const long long hi = lo + static_cast<long long>(lvl2[t].size());
            if (stored < lo || stored >= hi) continue;
            return static_cast<long long>(lvl1[t].member_slot.size()) *
                   static_cast<long long>(lvl2[t][static_cast<size_t>(stored - lo)]
                                              .member_slot.size());
        }
        return 0;
    }

    // The permutations carrying a stored instance's frame onto each real runout
    // it stands for -- one per runout, so the count matches orbit_weight().
    // Averaging across the board needs these and not just the count: a member's
    // strategy is the representative's with the combo indices moved, so adding
    // the representative's numbers raw would mix two different frames.
    void orbit_members(long long stored, int levels, std::vector<int>& perms) const {
        perms.clear();
        if (levels <= 0) { perms.push_back(identity_); return; }
        if (levels == 1) {
            if (stored < 0 || stored >= static_cast<long long>(lvl1.size())) return;
            perms = lvl1[static_cast<size_t>(stored)].member_perm;
            return;
        }
        if (levels != 2 || lvl2.empty()) return;
        for (size_t t = 0; t < lvl1.size(); ++t) {
            const long long lo = lvl2_base[t];
            const long long hi = lo + static_cast<long long>(lvl2[t].size());
            if (stored < lo || stored >= hi) continue;
            const Orbit& o2 = lvl2[t][static_cast<size_t>(stored - lo)];
            for (int p1 : lvl1[t].member_perm)
                for (int p2 : o2.member_perm)
                    perms.push_back(perm_mul[p1][p2]);
            return;
        }
    }

    // Maps a concrete runout (deck slots, in dealing order) onto the instance
    // that is actually stored, plus the permutation carrying the stored frame
    // onto the requested one.
    bool locate(const std::vector<int>& slots, long long& out_inst, int& out_perm) const {
        out_inst = 0;
        out_perm = identity_;
        if (slots.empty()) return true;

        int t = -1, p1 = identity_;
        for (size_t i = 0; i < lvl1.size() && t < 0; ++i)
            for (size_t m = 0; m < lvl1[i].member_slot.size(); ++m)
                if (lvl1[i].member_slot[m] == slots[0]) {
                    t = static_cast<int>(i);
                    p1 = lvl1[i].member_perm[m];
                    break;
                }
        if (t < 0) return false;
        out_inst = t;
        out_perm = p1;
        if (slots.size() == 1) return true;
        if (lvl2.empty()) return false;

        // The second card as seen from inside the representative's frame.
        const int want = perm_card[perm_inv[p1]][deck[static_cast<size_t>(slots[1])]];
        const int ws = slot_of[want];
        const std::vector<Orbit>& L2 = lvl2[static_cast<size_t>(t)];
        int k = -1, p2 = identity_;
        for (size_t i = 0; i < L2.size() && k < 0; ++i)
            for (size_t m = 0; m < L2[i].member_slot.size(); ++m)
                if (L2[i].member_slot[m] == ws) {
                    k = static_cast<int>(i);
                    p2 = L2[i].member_perm[m];
                    break;
                }
        if (k < 0) return false;
        out_inst = lvl2_base[static_cast<size_t>(t)] + k;
        out_perm = perm_mul[p1][p2];
        return true;
    }

private:
    int identity_ = 0;

    void build_perms() {
        int n = 0;
        for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 4; ++b) { if (b == a) continue;
        for (int c = 0; c < 4; ++c) { if (c == a || c == b) continue;
        for (int d = 0; d < 4; ++d) { if (d == a || d == b || d == c) continue;
            perm_suit[n][0] = a; perm_suit[n][1] = b;
            perm_suit[n][2] = c; perm_suit[n][3] = d;
            ++n;
        }}}
        for (int p = 0; p < 24; ++p) {
            for (int c = 0; c < 52; ++c)
                perm_card[p][c] = card_of(card_rank(c), perm_suit[p][card_suit(c)]);
            if (perm_suit[p][0] == 0 && perm_suit[p][1] == 1 &&
                perm_suit[p][2] == 2 && perm_suit[p][3] == 3) identity_ = p;
        }
        for (int a = 0; a < 24; ++a)
            for (int b = 0; b < 24; ++b) {
                int want[4];
                for (int s = 0; s < 4; ++s) want[s] = perm_suit[a][perm_suit[b][s]];
                for (int q = 0; q < 24; ++q)
                    if (perm_suit[q][0] == want[0] && perm_suit[q][1] == want[1] &&
                        perm_suit[q][2] == want[2] && perm_suit[q][3] == want[3]) {
                        perm_mul[a][b] = q;
                        break;
                    }
            }
        for (int p = 0; p < 24; ++p)
            for (int q = 0; q < 24; ++q)
                if (perm_mul[p][q] == identity_) { perm_inv[p] = q; break; }

        for (int c = 0; c < 52; ++c) slot_of[c] = -1;
        for (size_t i = 0; i < deck.size(); ++i) slot_of[deck[i]] = static_cast<int>(i);

        perm_combo.assign(24, std::vector<int>(static_cast<size_t>(num()), 0));
        for (int p = 0; p < 24; ++p)
            for (int h = 0; h < num(); ++h) {
                const Combo& k = combos[static_cast<size_t>(h)];
                const int i = index_of(perm_card[p][k.c1], perm_card[p][k.c2]);
                perm_combo[static_cast<size_t>(p)][static_cast<size_t>(h)] = (i >= 0) ? i : h;
            }
    }

    void build_strength() {
        const int n = num();
        const int D = deckN();

        if (start == ST_RIVER) {
            num_runouts = 1;
        } else if (start == ST_TURN) {
            num_runouts = D;
        } else {
            pair_id.assign(static_cast<size_t>(D) * D, -1);
            runout_slot.clear();
            int id = 0;
            for (int i = 0; i < D; ++i)
                for (int j = i + 1; j < D; ++j) {
                    pair_id[static_cast<size_t>(i) * D + j] = id;
                    pair_id[static_cast<size_t>(j) * D + i] = id;
                    runout_slot.push_back(i);
                    runout_slot.push_back(j);
                    ++id;
                }
            num_runouts = id;
        }

        scores.assign(static_cast<size_t>(num_runouts) * n, 0);
        order.assign(static_cast<size_t>(num_runouts) * n, 0);

        int seven[7];
        for (size_t i = 0; i < board.size(); ++i) seven[i + 2] = board[i];
        std::vector<int> idx(static_cast<size_t>(n));

        for (int ro = 0; ro < num_runouts; ++ro) {
            int t, r;
            runout_cards(ro, t, r);
            int nb = static_cast<int>(board.size());
            if (t >= 0) seven[2 + nb++] = t;
            if (r >= 0) seven[2 + nb++] = r;

            int* sc = &scores[static_cast<size_t>(ro) * n];
            for (int h = 0; h < n; ++h) {
                const Combo& k = combos[static_cast<size_t>(h)];
                // A combo using a runout card cannot reach this board; its reach
                // is zero everywhere below, so any finite score will do.
                seven[0] = k.c1;
                seven[1] = k.c2;
                sc[h] = eval7(seven);
                idx[static_cast<size_t>(h)] = h;
            }
            std::sort(idx.begin(), idx.end(), [sc](int a, int b) { return sc[a] < sc[b]; });
            int* od = &order[static_cast<size_t>(ro) * n];
            for (int h = 0; h < n; ++h) od[h] = idx[static_cast<size_t>(h)];
        }
    }
};

// -----------------------------------------------------------------------------
//  For every combo, how much of `r` is compatible with holding it: the total
//  minus everything sharing a card. Combos sharing ONE card drop out through the
//  per-card sums; the single combo sharing BOTH is the hero's own holding, so it
//  is subtracted twice and added back once. O(N) rather than a pairwise scan.
// -----------------------------------------------------------------------------
inline void compat_counts(const Deal& d, const double* r, int n, std::vector<double>& out) {
    double card[52];
    for (int c = 0; c < 52; ++c) card[c] = 0.0;
    double tot = 0.0;
    for (int i = 0; i < n; ++i) {
        const double w = r[i];
        if (w == 0.0) continue;
        tot += w;
        card[d.combos[static_cast<size_t>(i)].c1] += w;
        card[d.combos[static_cast<size_t>(i)].c2] += w;
    }
    out.assign(static_cast<size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        const Combo& k = d.combos[static_cast<size_t>(i)];
        out[static_cast<size_t>(i)] = tot - card[k.c1] - card[k.c2] + r[i];
    }
}

// -----------------------------------------------------------------------------
//  Showdown sweep on one runout: how much opponent reach every combo beats and
//  how much beats it, both blocker-exact.
//
//  Combos sharing one card fall out of the per-card running sums. The one combo
//  sharing both cards is the hero's own holding: it scores identically, so it
//  lands in neither the strictly-weaker nor the strictly-stronger set and needs
//  no correction. O(N) given the precomputed ordering.
// -----------------------------------------------------------------------------
//  `want` marks the hands whose win/lose are actually going to be read (the
//  traverser's range); everything else is skipped on the write side. The
//  accumulation side skips opponents with no reach, which on a real range is
//  most of the board -- adding a zero is exact, so nothing moves.
//  `od` is a strength-ordered list of `m` combo indices and `sc` maps a combo
//  index to its score on this runout. The caller may hand in a subsequence of
//  the full order -- the combos neither player can hold contribute nothing to
//  the running sums and are never asked about, so leaving them out of the walk
//  gives exactly the same answer for a fraction of the steps.
// One step of the sweep, laid out so the walk is a straight read. The score,
// the combo index and the two cards used to live in three different arrays and
// were reached through the order array, so every step of what is the hottest
// loop in the solver did three dependent random loads. They are the same three
// numbers for the same runout every iteration, so they are packed once.
struct SdEntry {
    int           score;
    unsigned short combo;
    unsigned char  c1, c2;
};

// Packs one runout's strength order for the sweep. The solver does this once
// per solve for every runout; equity does it a runout at a time into a buffer
// it reuses, because it walks each runout exactly once.
inline void pack_sweep(const Deal& d, const int* od, const int* sc, int m,
                       const unsigned char* keep, std::vector<SdEntry>& out) {
    out.clear();
    for (int p = 0; p < m; ++p) {
        const int h = od[p];
        if (keep && !keep[h]) continue;
        SdEntry e;
        e.score = sc[h];
        e.combo = static_cast<unsigned short>(h);
        e.c1 = static_cast<unsigned char>(d.combos[static_cast<size_t>(h)].c1);
        e.c2 = static_cast<unsigned char>(d.combos[static_cast<size_t>(h)].c2);
        out.push_back(e);
    }
}

inline void showdown_split(const SdEntry* pk, int m,
                           const double* ro, double* win, double* lose,
                           const unsigned char* want) {
    const int n = m;
    double card[52];

    for (int c = 0; c < 52; ++c) card[c] = 0.0;
    double run = 0.0;
    int j = 0;
    for (int p = 0; p < n; ++p) {
        const SdEntry& e = pk[p];
        while (j < n) {
            const SdEntry& f = pk[j];
            if (f.score >= e.score) break;
            // No test for zero. Skipping the three adds when a hand has
            // no reach looks free and is not: whether a hand is live
            // varies with the node, so the branch is unpredictable, and
            // the mispredictions cost more than the adds. Measured on
            // this loop, which is 43% of an iteration: 11.33 ms with the
            // test, 10.56 without, every run of one beating every run of
            // the other. Adding zero is harmless.
            const double w = ro[f.combo];
            run += w;
            card[f.c1] += w;
            card[f.c2] += w;
            ++j;
        }
        if (!want[e.combo]) continue;
        win[e.combo] = run - card[e.c1] - card[e.c2];
    }

    for (int c = 0; c < 52; ++c) card[c] = 0.0;
    run = 0.0;
    j = n;
    for (int p = n - 1; p >= 0; --p) {
        const SdEntry& e = pk[p];
        while (j > 0) {
            const SdEntry& f = pk[j - 1];
            if (f.score <= e.score) break;
            // Same as the forward sweep: no test for zero.
            const double w = ro[f.combo];
            run += w;
            card[f.c1] += w;
            card[f.c2] += w;
            --j;
        }
        if (!want[e.combo]) continue;
        lose[e.combo] = run - card[e.c1] - card[e.c2];
    }
}

