# Changelog

All notable changes to this project are documented here. Dates are ISO-8601.

The format follows [Keep a Changelog](https://keepachangelog.com/) loosely and
the project uses [semantic versioning](https://semver.org/): until 1.0.0, the
minor number moves when something user-visible changes.

## [0.1.1] — 2026-09-21

The evening-before-launch pass: looking at the screen the way someone who has
never seen it will look at it.

- **Keyboard at the table**: `F` `X` `C` for fold, check and call, `1` `2` … for
  the bet and raise sizes in order, `N` next hand, `R` replay it, `A` the
  advice. Each key is printed in the corner of its own button, and nothing fires
  while you are typing in a field.
- A **refresh mid-hand** puts you back at the table instead of the study screen.
- **Fixed**: the disk note stayed in Spanish on the English screen — two pieces
  of text ended up in one node and the key stopped matching.
- **Fixed**: text clipped at 430px wide, in the nodelock dialog and the setup
  grid.
- The table is bigger where there is room, the turn and river slots are visible,
  the strategy grid takes the width it has, and the action buttons are big
  enough to hit.
- Two **screenshots** in the README, and the release notes are written in proper
  Spanish.
- **Fixed**: six places in the text console were still in Spanish -- the memory
  breakdown, the DCFR line, the made-hand table headers and the saves listing --
  inside an interface that says of itself that it is English throughout. The
  suite now runs the console and looks for words that do not exist in English.

## [0.1.0] — 2026-09-21

First public release. The engine and the interface have been in daily use for a
while; this is the point at which they get a version number and a download.

### Engine

- Multi-street **Discounted CFR** (α 1.5, β 0.0, γ 2.0) over the full 52-card
  deck, solving from the **flop, turn or river** with complete ranges.
- Betting trees with per-player, per-street bet and raise sizes, **donk bets**,
  bare **all-in** branches, a "don't 3-bet" switch for IP, rake with a cap, and
  near-duplicate size merging at a measured 10% threshold.
- **Suit isomorphism** on runouts, validated against range symmetry rather than
  assumed — a 6× saving on a monotone flop, 2× on a two-tone.
- **Nodelocking** at the exact node including the dealt card, combo by combo,
  with the whole tree above re-solving around it.
- **Accuracy target** as a stopping rule (the average of the two best responses,
  as a percentage of the pot) plus an iteration cap and a wall-clock timeout.
- Memory reported **before** allocating, with a limit defaulting to 75% of the
  machine's RAM.
- Persistent thread pool, work handed out by an atomic counter so uneven chance
  fan-out still balances.
- Save and load **ranges** (both sides), **configs** and full **trees**
  (resumable).

### Playing the tree

- **Trainer**: play the solved tree hand by hand against **DCFR Bot**, which
  plays the solution — at every one of its nodes it rolls a die with the
  frequencies of its own hand.
- The **advice** (frequency and EV of every action, for your exact hand) can be
  shown or hidden; hidden means it is not sent to the browser at all.
- **Scored by EV, not by frequency**: each decision costs
  `EV(best) - EV(yours)` in chips for the hand you held, so mixing where the
  solver mixes is never punished.
- A **review** at the end of every hand, and a running grade as a percentage of
  the pot lost per decision.
- **Any node can be the starting point**, with the hand dealt from the range
  that reaches it; the runout is re-dealt every hand unless you fix it.
- Every hand carries a **seed** and can be replayed exactly.
- **Keyboard**: `F` `X` `C` for fold, check and call, `1` `2` … for the bet and
  raise sizes, `N` next hand, `R` replay, `A` advice. The key is printed on the
  button and nothing fires while you are typing in a field.
- A refresh mid-hand puts you back at the table instead of the study screen.

### Interface

- A single self-contained page served over `127.0.0.1` from a hand-written HTTP
  server: 13×13 range grids, tree browser, strategy grid coloured by action with
  equity and EV painted into the cells, made-hand and draw report with draggable
  category bars, nodelock dialog with per-suit combo painting.
- **English and Spanish**, switched from the gear, remembered per browser. The
  engine's own messages answer in the language the page asks for.
- Weight of every hand on hover in the setup grids.
- **Many boards…**: generates a script that solves a list of boards unattended,
  with per-board accuracy and time caps and a timestamped log.
- Seven starter ranges shipped in the binary, written out only when no range of
  your own exists.

### Fair play

- **It will not run next to a poker room.** On start it looks at the running
  processes and refuses to open if it finds a client for PokerStars, GGPoker,
  Winamax, 888poker, CoinPoker or iPoker; open one while it is running and it
  closes itself within three seconds, saying why. It matches brands, never the
  bare word "poker", so study tools like PokerTracker keep working.

### Correctness

- `solver --check`: 1,480+ assertions covering hand evaluation, tree building,
  the maths, persistence, the console and the interface. Exit code 1 if anything
  moved.
- Every check in the suite is verified by breaking the thing it guards.
- Engine values cross-checked against an independent commercial solver on
  identical trees, node by node.

[0.1.1]: https://github.com/danidealmeria-alt/dcfr-solver/releases/tag/v0.1.1
[0.1.0]: https://github.com/danidealmeria-alt/dcfr-solver/releases/tag/v0.1.0
