# Frequently asked questions

> 🇪🇸 **[Estas preguntas en español](FAQ.es.md)**

## The basics

**Does it play preflop?**

No. You give it a flop, a turn or a river that is already dealt, plus the ranges
the two players arrive there with. There is no preflop solver and no built-in
opening ranges.

**How much memory does it really need?**

It depends on the tree, and it depends a lot:

| spot | rough memory |
|---|---|
| river, one bet size | tens of MB |
| turn, two sizes | hundreds of MB |
| flop, two sizes + raises + all-in | **2 to 3 GB** |
| flop with many sizes | over 8 GB |

That is not a defect: a flop solver replicates the river subtree across 1,176
runouts. The interface tells you how much it will take **before** it builds
anything, and if you go over the limit it tells you what to cut. Drop bet sizes
before you drop ranges — every extra size multiplies.

**Does it work on Mac or Linux?**

The code is written for both (there is a POSIX branch alongside the Windows
one) and there is a Makefile and a CMakeLists. It is developed and tested daily
on Windows; if you build it elsewhere and something breaks, that is useful
information and it is welcome.

**Does it need an internet connection?**

No. It runs entirely on your machine and talks to nobody. The interface is a web
page served by the program itself on `127.0.0.1`, which is the address of your
own computer and of nobody else.

---

## Accuracy and results

**Is it as accurate as a commercial solver?**

In everything that has been compared, yes. Same tree, same ranges, same spot:
the game values agree to 3-4 decimal places across eight different spots, and
the strategies agree combo by combo. See the validation section of the README.

The differences that remain always fall on **indifferent** hands — the ones
whose actions are worth the same to within a hundredth of a chip — where any
mix is an equilibrium and both solvers are right.

**Why is my 33% bet 33 chips and not 33.4?**

Because you cannot bet half a chip at a table. Sizes are **truncated to whole
chips**, as in every other solver: 25% of a 250 pot is 62.5 and the tree builds
62.

It truncates rather than rounds for two reasons: it is what the others do, and
this way it never bets **more** than you asked for.

The **all-in is left alone**: the cap is the stack you set. If you have 250.5
behind, the all-in is 250.5.

One side effect worth knowing: a size that comes out to **less than one chip**
is no longer a bet, and the solver rejects it and says so instead of letting it
through. With a pot of 20, the smallest that fits is 5%.

**The equity I see does not match another solver's.**

Check which **node** you are on. Equity is against the range the opponent
arrives **there** with, and that range changes with every action: whoever calls
a bet arrives with more hand, so your equity drops.

On `Ah9h4h`, AA is worth:

| where | equity |
|---|---|
| against the whole range | **85.0%** |
| after the opponent checks | 86.7% |
| facing their bet | **74.4%** |

All three numbers are correct and all three are from different places. The 85%
is verified three independent ways: our own calculation, a brute-force
enumeration of every turn and river (`tools/eqcheck.cpp`), and a commercial
solver, which gives 0.8500.

**My exploitability does not match another solver's for the same solution.**

Watch the convention, because there is a factor of two in it. What is shown
elsewhere is the **average** of the two best responses; internally this program
computes the **sum**. The number in the interface and the accuracy target are
already converted to the usual convention, so they should agree. If you are
comparing raw numbers from the console, divide by two.

**Why does exploitability sometimes go up when I give it more iterations?**

Because that is how it behaves, and it is not a bug. The average strategy
weights recent iterations more heavily (the gamma exponent of DCFR), so it
follows the current strategy closely; when the current strategy takes an
excursion — because an action it was not using becomes profitable, say — the
average follows it.

Measured here: from 0.0042 at 1,810 iterations to 0.0618 at 1,880, and another
two thousand iterations to recover. It has been checked that this is what it is
and not corruption: the bump lands on the same iterations with gamma 1, 2 and 3,
and its size grows with gamma.

**The practical consequence**: set an accuracy target, not a number of
iterations. A counter does not tell you where you landed.

**What are alpha, beta and gamma?**

The three discounts of Discounted CFR, which is the algorithm behind this. They
are **deliberately not exposed in the interface**: they are fixed at the values
from the original paper, and changing them only alters how fast it converges,
not what the answer is. They are explained here because you will see them named
in the literature.

- **alpha (1.5)** — how much **positive** regret is discounted. High means what
  was learned long ago weighs almost as much as what was learned now.
- **beta (0)** — the same for **negative** regret. At 0 it is halved every
  iteration: an action that was going badly stops dragging its history around
  and can be tried again if the opponent changes.
- **gamma (2)** — how much recent iterations weigh in the **average strategy**,
  which is the one you are shown. Squared: what is happening now dominates.

If you want to experiment with them, they are in the console (`set alpha`,
`set beta`, `set gamma`).

---

## Using it

**Can I use the ranges I already have?**

Yes — that is the format used here: hands separated by commas, pure ones with no
weight and partial ones with `:0.5`.

```
AA,KK,QQ,AKs,A8o:0.5,KQ,KJ,K8o:0.5,Q5s:0.5,J9o:0.5
```

Paste it straight into the *Range text* box and the grid paints itself. And the
other way round: whatever you paint on the grid comes out written in that same
format, ready to paste wherever you want or to save where you keep the rest.

The usual shorthands are accepted too — `22+`, `A2s+`, `KTo+`, `55-88`, `AsKd`
for one specific combo, `random` for everything — and the ten as either `T` or
`10`.

**If I lock a river, does it only apply to that card?**

Yes: the lock belongs to the **exact node you are on, with that card already
dealt**. Locking the 3s does not lock the 3h. Put yourself on the runout you
care about (`cd 3s`, or by clicking the card in the tree) before you lock.

The two rules that come with it, both measured:

- **Everything above is recomputed.** Freezing a river changes what betting the
  turn is worth, and the turn finds out. Locking one river out of 48 moves the
  turn bet by 0.019 on average; locking all 48 moves it by 0.190 — ten times
  more, with 48 times more future touched.
- **Nothing below is inherited.** Locking the turn leaves the rivers free: they
  keep solving, each against the range that reaches it from the frozen turn.

A lock on a turn or a flop does not have this distinction, because that decision
is taken **once, before** any card falls.

**What is nodelocking and how does it work here?**

It is fixing what part of a range does at a node, to see how the rest of the
tree adapts. The flow is the usual one:

1. Solve.
2. Navigate to the node you care about and look at the strategy.
3. Open **Nodelock**, pick the action and **paint** the hands you want to fix.
   It paints combo by combo and with the weight you choose, not class by class.
4. Solve again: what you painted stays put and the rest re-adapts.

**Suit collapsing is switched off, or trimmed, if the lock names specific
cards** (`AsKs`), because at that point suits stop being interchangeable. The
interface tells you so. If you fix by ranges (`QQ+`) you lose nothing.

**Why does it sometimes say "suits collapsed x6" and other times x2 or nothing?**

(Collapsing is always on; it is not an option. What varies is how much of it can
be exploited.)

Two runouts that differ only by a swap of suits are the same decision, so one is
solved and the other is read off by permuting it. How much this saves depends on
the board: a monotone flop leaves three suits free (a group of 6), a two-tone
leaves two (a group of 2), a rainbow leaves none.

And it depends on your ranges: it only collapses by the permutations **your
ranges also survive**. A range with no diamonds in it trims the group.

**Can I save a range so I do not have to repaint it every time?**

Yes. Under the grid there is a name and the **Save** / **Load** / **Delete**
buttons. It saves whichever side you have open in the tabs (OOP or IP).

A range is saved **without the player inside it**, on purpose: one saved from
OOP can be loaded into IP, which is exactly what you want when building the same
spot from the other seat. They are tiny text files in `saves/ranges/`, so you
can edit them in a text editor or pass them to somebody else.

From the console: `save range <name> oop|ip`, `load range <name> oop|ip`,
`delete range <name>`, and `saves` lists them.

**Why does the program close when I close the browser tab?**

Because otherwise it did not close at all. This is a server: the window you look
at is a page it serves, and closing that page used to leave the process running
with the whole tree still in memory — measured at 551 MB and 19 threads for the
flop it opens with — with no window, and nothing to stop it but the Task
Manager. Open the program three times in a week and that is a gigabyte and a
half held for nothing.

Every tab tells the server it is still there once every ten seconds, and says
goodbye when it goes. When the last one leaves, the program waits a few seconds
and exits, and it says so in the console before it does.

Three things it will **not** do:

- **Close in the middle of a solve.** Shutting the tab on a forty-minute flop
  and losing it would be worse than the leak this fixes. It waits for the solve
  to finish, and goes only if nobody has come back by then.
- **Close if no browser ever connected.** `solver --gui --no-open` stays up
  until you open the page yourself. "The page left" and "the page never came"
  are not the same thing.
- **Close because the tab was in the background.** Chrome throttles a hidden
  tab's timers to one beat a minute; the server waits two and a half minutes
  before it gives a tab up for dead.

If it starts counting down and that was not what you wanted, just open the page
again — the countdown cancels.

**Can I save a solved tree?**

Yes, with the *Save tree* button. It takes real space — a solved flop is tens of
megabytes — because it stores the accumulated regrets and strategy, which is
what is needed to carry on solving where you left off. Configurations are saved
separately and weigh half a kilobyte.

---

**Why will it not open while I have a poker room open?**

Because every room's terms forbid using real-time assistance while you play, and
this is real-time assistance if you have it next to the table. The program looks
at the running processes when it starts and refuses to open; if you open a room
afterwards, it closes itself.

It covers PokerStars, GGPoker, Winamax, 888poker, CoinPoker and iPoker. It
matches brand names and not the word "poker", so PokerTracker, Hold'em Manager,
Flopzilla and the rest keep working — the suite checks that.

It prevents the accident, which is how this actually happens. It does not stop
anyone determined to get around it, and it does not pretend to.

## Playing against the solution

**How does the trainer score me?**

By **EV lost**, not by frequency. At each of your decisions it looks at what
each action is worth **for the exact hand you are holding** — the same numbers
the grid paints in EV mode — and compares the one you took with the best one.
The difference, in chips, is what it cost you.

This matters more than it looks. With a hand the solver bets 70% of the time,
**checking is not a mistake** if checking is worth the same: actions that get
mixed are mixed *because* they are worth the same. Scoring against the frequency
would punish what the theory itself calls indifferent, which is the fastest way
to learn superstitions.

The session grade is that EV lost per decision, as a percentage of the pot:
under 0.5% is flawless, over 5% is a leak.

**The bot called me with nothing and took the pot. Is it broken?**

No. The bot plays the solution: at each of its nodes it rolls a die with the
frequencies of **its** hand. It does not know what you have and it is not
playing to punish you, so sometimes it calls with the worst hand in its range
and hits. The result of a single hand says nothing — which is why the scoreboard
does not score it — and EV lost does not depend on how the cards fall.

**Why do I get different hands when I start at a node inside the tree?**

Because the hand is dealt from the range that **reaches that node**, not from
the starting range. If you pick the node after calling a bet, you will be dealt
hands that call. Dealing from the starting range would be training a spot that
is never played: half the hands would not be there.

The cards still missing on the way there (the turn, the river) are dealt at
random every hand, so you train the node and not one particular card. If you
want a fixed card, solve that board.

**If I close it and come back, is my score kept?**

The scoreboard lives as long as the program is open and goes back to zero with
*Start over*. It is not written to disk: it is for a study session, not a
history.

**Can I replay the hand I just destroyed?**

Yes, and it is the most useful thing in the trainer. Every hand carries a
**seed** in plain sight and *Replay this hand* deals the whole thing again: your
same cards, the bot's same cards, the same runout. Play the other line and
compare what each one cost.

## Licence and contributing

**Can I sell it, or sell it built into something else?**

It is under the GPL v3. You can use it and modify it freely, commercially
included, but if you **distribute** a modified version you have to publish your
code under the same licence. You cannot close it.

**How do I contribute?**

Read the README first, which is written for that, and the comments at the top of
the headers: every rule in the engine has, right next to it, why it is that one
and not another, with the measurement that decided it. It saves repeating dead
ends.

And one habit of the project worth respecting: **nothing is assumed to be better
without measuring it**, and negative results are written down with their numbers
instead of being deleted.

`solver --check` has to stay green.
