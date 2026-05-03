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
