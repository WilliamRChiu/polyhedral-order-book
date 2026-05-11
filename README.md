# Polyhedral Order Book

A C++ implementation of the polyhedral order book described in Jacob
Jackson's 2023 paper [`polyhedral.pdf`](./polyhedral.pdf). A traditional
central limit order book matches orders for a single pair of assets; a
polyhedral order book lets each order span any number of assets and
matches non-negative combinations of orders whose net inventory is
non-negative in every asset. The CLOB is the N=2 special case.

## Motivation

The paper identifies five reasons polyhedral matching matters:

1. **Tighter spreads via correlation hedging.** A market maker in
   ACME/USD bears both ACME-specific risk and broad-market risk. If
   they could quote ACME against SPY, market-wide moves cancel out and
   spreads can shrink.
2. **Solving the cold-start problem.** New pairs (ACME/SPY) start with
   zero volume on a CLOB, so spreads stay wide, so volume never
   arrives. A polyhedral book *implies* the new pair's liquidity from
   existing pairs (ACME/USD + SPY/USD), bootstrapping it on day one.
3. **Generalizing spread markets.** Calendar and inter-commodity
   spreads (currently hand-curated by exchanges) fall out automatically
   — market makers quote whatever spread they want.
4. **Replacing the ETF wrapper for basket exposure.** Market makers can
   quote `ACME+BACA+DADA` against USD directly, with no fund wrapper or
   management fee.
5. **Lowering market-making barriers to entry.** Hedged baskets carry
   far less residual risk, so non-HFT participants can quote
   competitively.

## How matching works

The book state is a matrix `A ∈ ℝ^{N×M}` where columns are resting
orders and rows are assets. The entry `A[i,j]` is the change in asset
*i* the counterparty would receive by filling order *j*. The book is
**crossed** iff there exists a nonzero `x` with `Ax ≥ 0` and
`0 ≤ x ≤ 1`; deciding this is a linear program (the *matching
problem*).

When an incoming order arrives, the **pricing-matching algorithm**
(paper §3.3) solves the **fair price problem** — a modified dual of
the matching LP — to find a price vector `y` that values the incoming
order under the cheapest prices that don't cross any resting order. If
`zᵀy < 0`, the order rests (no cross). Otherwise, it matches against
exactly the resting orders for which `(yᵀA)_i = 0` — the orders that
break even at price `y`. This repeats until the incoming order is filled
or the book is no longer crossed.

The fair price problem is *not* the §3.2 dual itself: §3.2 is pure
feasibility (`yᵀA < 0`, no objective), while §3.3 adds an objective
(`min zᵀy`), an L1 normalization (`‖y‖₁ = 1`), and excludes the
incoming column from the constraints. We solve §3.3 in code; §3.2 is
the theoretical scaffolding that proves the cross test is sound.

## Scope (v0)

- One book over N assets (no per-pair separation).
- Limit + market orders, IOC and FOK, post-only.
- Matching via the pricing-matching algorithm of paper §3.3.
- Maker fee = 0; all fees on the taker order (paper Remark 6 — maker
  rebates break under multi-leg matches).
- Deterministic replay from an event log.

Out of scope for v0: networking, persistence, multi-venue routing,
market data dissemination, tick sizes, and price-time priority — the
paper itself defers these to future work (§5).

## Background

A fundamental issue an exchange may face is that at any given moment, there may exist both buyers and sellers, but they may not show up at the same time, agree on the same price, or want the same quantity of a stock/good.

What a traditional order book does is create a live marketplace: it is a live list showing "buy" and "sell" offers organized by price.  An example is Bob the farmer sells apples at $1.02 per apple but Joe the buyer wants to only pay $1.00 per apple.  The delta is called the bid-ask spread.  Here, a trade will only occur if someone budges (ex: seller comes down or buyer comes up in price).  Using this tool, the exchange's job is to maintain this list in real time and match buyers with sellers automatically.

In other words, a order book plays two essential roles:
1) Finding matches of buyers + sellers
2) Giving a live look at what the current value of an asset based on market movement (i.e. what people are willing to pay)

The order book beats out systems like a "call auction" (where you wait a period of time to collect orders then match to a single clearing price -> This causes latency which is mega bad for fast trades) or "dealer market" (Which is dependent on a singular dealer for pricing -> slow and opaque on process)

Now looking at a central limit order book, it tracks singular pairs (n = 2).  An example is Stock AAA/USD.  Given that every order has a side (bid for buy, ask/offer for sell), a quantity, and a limit price (worst price TRADER will accept), best bid is the highest offer from a buyer, best ask is the lowest offer from a seller.  Midpoint is the rough "market price".  When a limit order (an order to buy at price X or better) comes in, if it crosses spread, it continously buys until the coverage goes to 0 or order is fully filled.  If there is no coverage yet there still exists quantity on the order, the order sits on the book at its limit price.  Note that orders are typically matched using price-time priority (first price, second time). Therefore, given two orders at same price, the one who came first gets served first.  Note this is not universal, some CLOBs may use pro-rata for instance.

If a order comes in and sits on the book, the offer is "resting" and therefore the orderer is the "maker".  The trader who fills the order (i.e. seller) is the "taker".  Maker puts a job on the board, taker fills the job essentially.  The idea of maker vs taker is important for the concept of a polyhedral order book since exchanges usually charge the two sides different fees (exchanges want more resting orders as it's more attractive so they charge lower fees for makers and higher fees for takers) which gets complicated.

The mindset used for this program is that a order book's job is to determine if a cross has occurred, and to resolve these crosses as quickly as possible by executing matches.  Here, crossed is defined as best bid >= best ask.

The core idea of the polyhedral order book is that instead of only analyzing paired asset trades, what if we analyzed trades involving 3+ assets?  An example is person A wants to give 10 dollars for a share of stock AAA, person B wants to give 1 share of AAA for 1 share of BBB, and person C wants to give 1 share of BBB for 10 dollars.  As we can see, there is no sequential trade that can be made here (no pairwise trade can occur).  However, if we create a table and look at the trade possibilities:

|                        | Cash   | AAA   | BBB   |
| ---------------------- | ------ | ----- | ----- |
| A gives $10, gets AAA  | −$10   | +1    | —     |
| B gives AAA, gets BBB  | —      | −1    | +1    |
| C gives BBB, gets $10  | +$10   | —     | −1    |
| **Net**                | **0**  | **0** | **0** |

In other words, "there exists a non-negative combination of orders whose net inventory is non-negative in every asset."  In example above, we found a hidden three way cycle that no pair matching algorithm could fill otherwise.

This is important in the real world. Consider what happens if we want to start a market for a whole new pair (ex: AAA/BBB).  Right now, almost every stock is traded against the USD, and there exists a "cold start" problem: you can't attract traders without liquidity, and you can't build liquidity without traders.  We can't get enough liquidity to make the market self-sustaining.

Why would we want to create an AAA/BBB market in the first place, instead of just trading AAA/USD?  Suppose AAA is a tech stock and BBB is a whole stock market index.  These two stocks have correlation, so if BBB goes up, AAA will probably go up as well.

Now if you are a market maker of AAA, you are posting resting orders on both sides to earn the spread, and your risk is that the price moves while you're holding your position.  If AAA is priced in dollars, AAA is exposed to a lot of volatility from market crashes, tech selloffs, etc.  However, if AAA is priced in terms of BBB, then if the whole market drops, AAA and BBB drop together — their ratio stays roughly the same.  You only expose yourself to AAA doing something weird relative to BBB, not every market-wide move.

### Simple Numbers Example

| Event              | AAA price | BBB price | AAA/USD | AAA/BBB ratio    |
| ------------------ | --------- | --------- | ------- | ---------------- |
| Normal day         | $100      | $500      | 100     | 0.20             |
| Market crashes 10% | $90       | $450      | 90 ↓    | 0.20 ← same!     |
| AAA bad news       | $80       | $500      | 80 ↓    | 0.16 ↓           |

Since we can't manage to do this using a regular CLOB, many market makers are forced to widen spreads to protect against **adverse selection** — a fast trader racing to lift their stale AAA quotes when BBB jumps.

## Build

Configure (cross-platform; downloads Catch2 + HiGHS into `build-ninja/_deps/` on first run):
run whenever cmake files edited
```
cmake -B build-ninja -G Ninja -S .
```

Build everything:
run whenver a cpp/hpp is edited
```
cmake --build build-ninja -j
```

Run all tests:

```
ctest --test-dir build-ninja --output-on-failure
```

Combined one-liner for incremental dev (configure → build → test):

```
cmake -B build-ninja -G Ninja -S . && cmake --build build-ninja -j && ctest --test-dir build-ninja --output-on-failure
```

Targets C++20, no external runtime deps. Catch2 v3 (tests) and HiGHS
(LP solver) are fetched via CMake FetchContent on first configure. The
Ninja generator drops `build-ninja/compile_commands.json`, which clangd
reads for inline diagnostics, jump-to-def, and clang-tidy.

### Toolchain prerequisites

- **macOS / Linux**: install Ninja (`brew install ninja` on macOS,
  `sudo apt install ninja-build` on Debian/Ubuntu) plus CMake ≥ 3.20
  and a C++20-capable compiler.
- **Windows**: install **Build Tools for Visual Studio** (or full VS) —
  this bundles MSVC, the Windows SDK, and Ninja. Run the commands above
  from a **Developer PowerShell for VS** (Start menu) so `cl.exe` and
  `ninja.exe` are on `PATH`.

`-G Ninja` is pinned because the default Visual Studio multi-config
generator on Windows does not emit `compile_commands.json`, which
breaks clangd. Mac and Linux usually default to a generator that does
emit it, but pinning Ninja keeps behavior identical across platforms.

## Layout

```
include/pob/      public headers
src/              implementation
tests/            unit + property tests
bench/            microbenchmarks
docs/             design notes, including the polyhedral model spec
```

## Status

Pre-alpha. Interfaces will change without notice.


## Decision Choices Logic
for CLOB, chose map as underlying container instead of maxHeap/minHeap since although maxHeap/minHeap better for cache hitting due to the underlying container of pqueue typically being a vector, the order book has better time complexities finding or eliminating by a certain key (heaps only good at getting top element but are O(N) for all other element searches), and have better time complexities for a ordered walk of the data

I chose a list as the sub type of the map since we want to have a price-time priority engine.  


## Definitions
Aggressor -> for CLOB, it is the trader who put in the order second and crossed into the existing book
Rester -> For CLOB, it is the trader who put in order first and had their order logged onto book before getting matched

Note that for CLOB, the aggressor always gets the price improvement (aka the spread)

## N=2 worked example

Convention: asset 0 = USD, asset 1 = ACME. A column of A is the trade as a vector of asset deltas to the counterparty.

Setup constraints. A match vector x must satisfy x ≥ 0 (a negative fill would mean reversing the order, which isn't a real trade) and Ax ≥ 0 (no asset can end up negative — counterparties can't be left short). The trivial solution x = 0 always satisfies these but represents "no trade happens," so we don't count it; we want a nonzero x.

### Case 1: Buy 10 ACME @ $50, Sell 10 ACME @ $50

Columns: z_buy = [+500, -10]ᵀ, z_sell = [-500, +10]ᵀ. With x = (1, 1), Ax = [0, 0]. Both orders fully fill, zero surplus — perfect match.

### Case 2: Buy 10 ACME @ $51, Sell 10 ACME @ $50 (crossed book)

Columns: z_buy = [+510, -10]ᵀ, z_sell = [-500, +10]ᵀ. With x = (1, 1), Ax = [+10, 0]. Both fill, with $10 surplus — that's the price improvement, awarded to the aggressor.

### Case 3: Buy 10 ACME @ $49, Sell 10 ACME @ $50 (not crossed)

Columns: z_buy = [+490, -10]ᵀ, z_sell = [-500, +10]ᵀ. Any x = (a, b) gives Ax = [490a − 500b, −10a + 10b]; the constraints force b ≥ a and a ≥ 1.02b, which collapses to a = b = 0.

Note: this is a proof of infeasibility. The only x satisfying Ax ≥ 0, x ≥ 0 is the trivial x = 0, which we exclude. So no nonzero match vector exists and the matching LP has no solution — the book is not crossed and no trade occurs. Dually, any y with y₀ = 1 and 49 < y₁ < 50 (e.g. y = [1, 49.5]) gives yᵀA < 0, certifying non-crossedness from the pricing side.


### Matrix labels

Columns are orders (one column per order in book)
Rows are assets (one row per share of stock/currency in system)

Visual layout for a 3-asset, 3-order book:

```
                  Order 1   Order 2   Order 3
Asset 0 (USD)   [  +25000    -500       0    ]
Asset 1 (SPY)   [    0        +5      -10    ]
Asset 2 (ACME)  [  -500       0       +20    ]
```

Reading by **column**: "Order 1 wants to buy 500 ACME for $25k" — the counterparty receives +$25k, gives -500 ACME. The whole order is described as one vector of asset deltas.

Reading by **row**: "Across all current orders, asset 1 (SPY) has potential exposures of [0, +5, -10]." Useful for asking "what happens to my SPY balance if I fill some combination of these orders?"

**Why this orientation matters.** The matching condition `Ax ≥ 0`:

- `x` has length M (one weight per order — the fraction filled).
- `Ax` has length N (one balance per asset).
- `(Ax)[i] = Σⱼ A[i,j] · x[j]` — the net flow into asset *i* across all weighted orders.

Requiring every entry of `Ax ≥ 0` means **no asset ends up net-negative** when you apply the weighted combination of orders — the exchange isn't left short on anything. This works precisely because rows = assets (the thing that must balance) and columns = orders (the things you're combining). Flipping the convention would force you to compute `xᵀA` instead, and the code's indexing would feel inverted from the paper.



We aim to maximize the fraction of orders filled in the primal equation determining if we are crossed or not.  By bounding the x between 0 and 1, it ensures that if a x exists s.t. Ax>=0, we cant just scale it up to infinity

if Ax >= 0 && Ax != 0, the pass over is net inventory and is added onto A for the next calculation in Ax + inventory >= 0

Therefore, if i can find a y such that yA < 0, then book can never be crossed as yAx < 0 since x >=0

but if y DNE, then we know the book is crossed.  This is derived from strong duality which states primal is bounded iff dual is feasible.  However, if only solution to primal is x = 0, then it means dual is feasible, yet dual is infeasible, therefore by strong duality, we can never have the trivial solution as only solution if y DNE, therefore we have a non zero x >=0 s.t. Ax >= 0, therefore it is crossed

When we get a new order and recalculate if a cross exists or not, we can do a simple check first instead of resolving dual problem.  We just check if yz < 0 as A' = A U {z}.  If true, the book still not crossed as yA'< 0 => yA'x<0

If false, then need to recalculate dual to see if a cross exists (i.e. if yz>=0 then recalculate dual for a new y').  To do this, we solve the Fair Price Problem to minimize z^ty such that y >=0 and y * 1 = 1 and yA <=0 for all A columns that are not z.  The logic for this is that we want to not use the trivial solution so y * 1 = 1.  If the optimal solution is less than 0, then we have all the columns of A

The fair price problem doesnt only just check yes or no, it finds the best possible y to first satisfy all old orders and then check what it says about z.  if the optimal y' says z * y' < 0 then y'A<=0 therefore y'Ax <=0 so no cross as only solution is x = 0 which also means not crossed.  If z * y' >=0 then there is no better y that I can multiply with z to get a negative value, therefore the dual problem is infeasible (y DNE) so by strong duality, book is crossed


Now, since we multipled y' to A, the only possible ways we can have crossing is for the y'A rows where they have a strict inequality.  Therefore, we just need to solve x for the rows where y'A = 0 (i.e. solve initial primal only for the Ax>=0 and y'A = 0) by complementary slackness theorem (x_i for all y'_iA<0 are 0; all other x_i can be any number >=0 so just solve)