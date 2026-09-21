#pragma once
// =============================================================================
//  Runtime configuration. Everything here is settable from the console or the
//  web UI; nothing sizes an array, so nothing needs to be constexpr except the
//  branching-factor bound.
// =============================================================================

#include <cstdio>

namespace cfg {

// La version, en un solo sitio.
//
// Hace falta en cuanto esto sale de una maquina: cuando alguien dice "me falla
// esto" hay que saber con QUE build le falla, y sin un numero a la vista la
// respuesta es "actualiza y prueba", que no es una respuesta. Sale en --help,
// en la barra de arriba de la interfaz y en el titulo de la pestana.
inline const char* VERSION = "0.1.3";

// ---- structural -------------------------------------------------------------
constexpr int MAX_ACTIONS = 8;    // upper bound on a node's branching factor

// How deep a single betting round may go before the builder gives up. There is
// no user-facing cap on bets and raises, and there does not need to be: every
// raise is at least as large as the one it answers, so the stack runs out, and
// the all-in threshold ends the chain earlier still. Measured on a 20-chip pot,
// the deepest a round actually goes is 3 levels at 100bb and 10 at 2000bb --
// well under this. The bound is here for the sizing that does NOT terminate in
// any useful sense, a bet of a thousandth of the pot min-raised to the ceiling,
// which would otherwise recurse thousands deep and run out of memory somewhere
// with nothing to read.
constexpr int MAX_RAISE_LEVELS = 24;

// All-in threshold, measured the way everyone measures it: a bet or raise that
// would commit MORE than this fraction of the STARTING effective stack becomes
// a clean all-in instead. 0.67 is the usual default and the reason is pot odds --
// with two thirds of your stack in the middle you are committed, so the third
// left behind buys no decision, only branches. 1.0 turns the rule off (you
// cannot commit more than everything); 0 makes every bet an all-in.
//
// This used to be measured against the resulting POT instead of the stack, and
// pinned. Both readings snap the same lines most of the time, but the stack one
// is what every solver and every player means by the phrase, and it is a single
// number for the hand rather than something that drifts street by street.
inline double ALLIN_THRESH = 0.67;

// Two sizings within this fraction of the larger one are the same bet with
// extra steps: the smaller wins and the branch is not duplicated. The reference has no
// field for it either, and measuring says why -- it is a constant, not a
// choice. The list "0.5,3x" of raises over a 12 bet in a 20 pot lands on 34 and
// on 36, six per cent apart and the same bet in every practical sense:
//
//   merge      0      0.05    0.10    0.20    0.30
//   nodes      164    164     112     112     112
//   memory     0.245  0.245   0.197   0.197   0.197 GB
//
// So 0.10 sits exactly on the knee. Below it the near-duplicate survives and
// costs a third of the tree for nothing; above it, on a two-bet-size tree, 0.20
// starts eating raises that really are different (747 nodes down to 466). The
// price of pinning it is that two sizings closer than a tenth of each other can
// no longer both exist -- which is a distinction nobody plays.
inline double MERGE_PCT = 0.10;

// Reporting sink. Every report writes here rather than straight to stdout, so a
// full report can be dumped to a file (or captured for the web UI) without any
// freopen() trickery. Interactive chatter keeps using stdout directly.
inline std::FILE* out = stdout;

// ---- spot geometry ----------------------------------------------------------
inline double POT0       = 20.0;  // dead money on the river
inline double STACK      = 100.0; // effective stack behind, per player

// ---- rake -------------------------------------------------------------------
//  Off by default, because a solve without rake is the right model for a
//  tournament and for pure theory. For cash it is not a missing feature but a
//  wrong one: rake makes marginal calls worse and shifts continuation
//  thresholds, and small pots feel it most.
//
//  RAKE_PCT is a fraction of the raked pot; RAKE_CAP is a ceiling in chips and
//  is ignored when it is not positive. Rake is off entirely at PCT <= 0.
//
//  The raked pot is the MATCHED pot: an uncalled bet goes back to the bettor
//  before the house takes anything, exactly as it does at a real table.
inline double RAKE_PCT = 0.0;
inline double RAKE_CAP = 0.0;

inline double rake_on_pot(double pot) {
    if (RAKE_PCT <= 0.0 || pot <= 0.0) return 0.0;
    const double r = RAKE_PCT * pot;
    return (RAKE_CAP > 0.0 && r > RAKE_CAP) ? RAKE_CAP : r;
}

// ---- Discounted CFR hyper-parameters (Brown & Sandholm 2019 defaults) -------
inline double DCFR_ALPHA = 1.5;   // discount on POSITIVE cumulative regret
inline double DCFR_BETA  = 0.0;   // discount on NEGATIVE cumulative regret
inline double DCFR_GAMMA = 2.0;   // discount on the average strategy

// Worker threads for the solve. 0 means "ask the machine". Lowering it leaves
// cores for whatever else you are doing; the solve otherwise takes all of them.
inline int THREADS = 0;

// The most memory a solve may claim, in GB. A tree is easy to describe and
// impossible to hold: three bet sizes and a cap of three on a rainbow flop
// estimates at 84 GB. Asking for it used to mean the allocation either threw
// somewhere unhelpful or, on Windows with a big pagefile, quietly succeeded and
// then thrashed the machine to a standstill -- which is worse, because there is
// nothing to read and nothing to cancel. Raise it if you have the RAM.
inline double MAX_MEM_GB = 8.0;

// Collapse runout cards that are equivalent under a suit permutation fixing the
// board. Only ever applied when the ranges are symmetric under that same group.
inline bool ISO = true;

} // namespace cfg
