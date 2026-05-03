# Polyhedral Order Book

A C++ limit order book that represents resting liquidity as a polyhedron in
(price, size, time) space rather than as flat per-level queues. Matching,
sweeps, and risk queries become geometric operations — half-space
intersections, projections, and volume integrals — over the live book.

## Why polyhedral

Conventional order books store each side as a price-indexed map of FIFO
queues. That layout is fast for top-of-book and single-level walks but
awkward for the questions that actually matter to a market maker:

- *How much notional can I sweep before slipping past price P?*
- *What is the convex envelope of resting bids inside ±k bps?*
- *Which orders dominate, in the Pareto sense, on (price, age, size)?*

Each of those is a polyhedral query. Modeling the book as a set of
half-spaces lets us answer them with one geometric primitive instead of
hand-rolled loops per question.

## Scope (v0)

- Single-symbol, single-venue, in-memory book.
- Limit + market orders, IOC and FOK, post-only.
- Price-time priority preserved; the polyhedral layer is an *index over*
  the canonical book, not a replacement for it.
- Deterministic replay from an event log.

Out of scope for v0: multi-venue routing, persistence, networking, fees.

## Build

```
cmake -B build -S .
cmake --build build -j
ctest --test-dir build
```

Targets C++20, no external runtime deps. Tests use Catch2 (fetched via
CMake FetchContent).

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


# Background Knowledge

A fundamental issue an exchange may face is that at any given moment, there may exists both buyers and sellers, but they may not show up at the same time, agree on the same price, or want the same quantity of a stock/good.

What a traditional order book does is create a live marketplace: it is a live list showing "buy" and "sell" offers organized by price.  An example is Bob the farmer sells apples at $1.02 per apple but Joe the buyer wants to only pay $1.00 per apple.  The delta is called the buy-ask spread.  Here, a trade will only occur if someone budges (ex: seller comes down or buyer comes up in price).  Using this tool, the exchange's job is to maintain this list in real time and match buyers with sellers automatically.

In other words, a order book plays two essential roles:
1) Finding matches of buyers + sellers
2) Giving a live look at what the current value of an asset based on market movement (i.e. what people are willing to pay)

The order book beats out systems like a "call auction" (where you wait a period of time to collect orders then match to a single clearing price -> This causes latency which is mega bad for fast trades) or "dealer market" (Which is dependent on a singular dealer for pricing -> slow and opaque on process)

Now looking at a central limit order book, it tracks singular pairs (n = 2).  An example is Stock AAA/USD.  Given that every order has a side (bid for buy, ask/offer for sell), a quantity, and a limit price (worst price TRADER will accept), best bid is the highest offer from a buyer, best ask is the lowest offer from a seller.  Midpoint is the rough "market price".  When a limit order (order saying buy me at X or lower price) comes in, if it crosses spread, it continously buys until the coverage goes to 0 or order is fully filled.  If there is no coverage yet there still exists quantity on the order, the order sits on the book at it's limit price.  Note that orders are typically matched using price-time priority (first price, second time). Therefore, given two orders at same price, the one who came first gets served first.  Note this is not universal, some CLOBs may use pro-rata for instance.

If a order comes in and sits on the book, the offer is "resting" and therefore the orderer is the "maker".  The trader who fills the order (i.e. seller) is the "taker".  Maker puts a job on the board, taker fills the job essentially.  The idea of maker vs taker is important for the concept of a polyhedral order book since exchanges usually charge the two sides different fees (exchanges want more resting orders as it's more attractive so they charge lower fees for makers and higher fees for takers) which gets complicated.

The mindset used for this program is that a order book's job is to determine if a cross has occurred, and to resolve these crosses as quickly as possible by executing matches.  Here, crossed is defined as best bid >= best ask.  

The core idea of the polyhedral order book is that instead of only analyzing paired asset trades, what if we analyzed trades involving 3+ assets?  An example is person A wants to give 10 dollars for a share of stock AAA, person B wants to give 1 share of AAA for 1 share of BBB, and person C wants to give 1 share of BBB for 10 dollars.  As we can see, there is no sequential trade that can be made here (no pairwise trade can occur).  However, if we create a table and look at the trade possibilites:

|                        | Cash   | AAA   | BBB   |
| ---------------------- | ------ | ----- | ----- |
| A gives $10, gets AAA  | −$10   | +1    | —     |
| B gives AAA, gets BBB  | —      | −1    | +1    |
| C gives BBB, gets $10  | +$10   | —     | −1    |
| **Net**                | **0**  | **0** | **0** |

In other words, "there exists a non-negative combination of orders whose net inventory is non-negative in every asset."  In example above, we found a hidden three way cycle that no pair matching algorithm could fill otherwise.

This is important in the real world since what if we want to start a market for a whole new pair (ex: AAA/BBB).  Right now, almost every stock is traded against the USD and since there exists a cold start principle in that for a new market you can't attract traders without liquidity, and you can't build liquidity without traders, we have a "zero to flywheel" type problem (can't get enough liquidity to make market become self sustaining in the sense of new traders coming in with orders to put on the books).  The reason we would want to create a market like AAA/BBB instead of just trading AAA/USD like the status quo is that say AAA is a tech stock and BBB is a whole stock market index.  These two stocks have correlation, so if BBB goes up, AAA will probably go up as well.  Now if you are a market maker of AAA, you are posting resting orders on both sides to earn the spread and your risk is that the price moves while holding your position.  If AAA is priced using dollars, AAA is exposed to a lot of volatility from market crashes, tech selloff, etc.  However, if AAA is in terms of BBB, if whole market drops, since AAA is correlated to BBB, both will drop roughly together ensuring that their ratio stays roughly the same.  Therefore, you only expose yourself to AAA doing something weird relative to BBB, not every market wide move.

### Simple Numbers Example

| Event              | AAA price | BBB price | AAA/USD | AAA/BBB ratio    |
| ------------------ | --------- | --------- | ------- | ---------------- |
| Normal day         | $100      | $500      | 100     | 0.20             |
| Market crashes 10% | $90       | $450      | 90 ↓    | 0.20 ← same!     |
| AAA bad news       | $80       | $500      | 80 ↓    | 0.16 ↓           |


Since we cant manage to do this using regular CLOB, many market makers are stuck with creating larger spreads on their buy/sell prices just to ensure that if a large positive change happens in lets say BBB, a fast trader doesnt race to buy the market makers AAA shares at its stale price (since AAA will probably go up due to its correlation with BBB)