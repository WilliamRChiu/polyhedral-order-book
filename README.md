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
(paper §3.3) solves a dual LP to find a fair price vector `y`, then
matches against exactly the resting orders that price `y` deems
competitive. This repeats until the incoming order is filled or the
book is no longer crossed.

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

First-time configure (downloads Catch2 via FetchContent into `build/_deps/`):

```
cmake -B build -S .
```

Build everything:

```
cmake --build build -j
```

Run all tests:

```
ctest --test-dir build --output-on-failure
```

Combined one-liner for incremental dev (configure → build → test):

```
cmake -B build -S . && cmake --build build -j && ctest --test-dir build --output-on-failure
```

Targets C++20, no external runtime deps. Tests use Catch2 v3 (fetched
via CMake FetchContent).

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