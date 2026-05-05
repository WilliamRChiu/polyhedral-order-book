# Polyhedral Order Book — Build Plan

## Thesis

Portfolio project, not a real exchange. The win is a working engine that
demonstrably matches trades a CLOB cannot, with the engineering depth of
a quant-dev hire visible in the code: numerical discipline, incremental
algorithms, deterministic replay, defensible benchmarks.

## Build order

1. **Reference CLOB.** Price-time priority, limit/market/IOC/FOK/post-only,
   cancels. Acts as the correctness oracle (N=2 polyhedral ≡ CLOB) and the
   performance baseline.
2. **Order/asset model + sparse matrix store.** Asset registry, columns as
   sparse vectors, CSC layout for `A`.
3. **Cold-solve pricing-matching pipeline.** Wire HiGHS for the fair-price
   LP and the restricted matching LP. Get correctness end-to-end.
4. **Property tests vs CLOB at N=2.** Same event stream, identical fills.
5. **Custom warm-startable simplex.** Column-level basis surgery — the
   real performance lever.
6. **`y`-cache state machine.** Maintain the price vector; only re-solve
   when the cache is invalidated.
7. **Benchmark harness.** Fast-path hit rate, p50/p99 latency, ops/sec
   vs N and M.
8. **CLOB-compat pair view.** Implied-liquidity level-2 feed for any
   `(i,j)` — the killer demo.

## Core engineering problems

- **Incremental warm-starting.** Column add (new order), column drop
  (cancel/full fill), column scale (partial fill). HiGHS hot-restart
  isn't column-surgical; a custom revised dual simplex with explicit
  basis management gets few-pivot updates instead of fresh solves.
- **`y`-cache invariants.** When is the cached `y` still optimal?
  - New order with `yᵀz ≥ 0`: `y` still optimal.
  - Cancel of an inactive column (`(yᵀA)ᵢ < 0`): `y` still optimal.
  - Cancel of an active column (`(yᵀA)ᵢ = 0`): `y` still feasible, may
    not be optimal — re-solve from `y`.
  - Partial fill scales a column: similar, re-solve from `y`.
  Worth a transition table with proofs in the design doc.
- **Alternative optima in the matching LP.** The restricted match LP has
  a polytope of optimal `x` in general. Paper punts on tie-breaking.
  Pick a deterministic rule (lexicographic, FIFO via secondary objective,
  age-weighted) and justify it.
- **Sparse + incremental linear algebra.** CSC for `A`, sparse `yᵀA`
  maintained incrementally, explicit "active set" structure for
  `(yᵀA)ᵢ = 0` columns.
- **Determinism under floating point.** LP results aren't bitwise stable
  across solvers/platforms. Use integer prices/sizes (real exchanges do),
  pin the solver, and document tolerance discipline.

## Open research bits (paper leaves these as future work)

- **Price-time priority within polyhedral matches** (paper §5). Define
  it, defend it, implement as the matching-LP tie-breaker.
- **Fee design surviving multi-maker matches** (Remark 6). Paper waves
  at "maker fee = 0." A scheme that scales with surplus or charges per
  edge instead of per fill is its own contribution.
- **CLOB-compat implied-liquidity feed** (Remark 5). Project the matching
  polytope onto a chosen `(i,j)` plane; produce a level-2 view including
  implied quotes. Computing this efficiently for live dissemination is
  non-trivial.

## Benchmark targets

Single-threaded, N=10–20 assets, M≈10k resting orders, replayed mixed event stream.

| Path | Target ops/sec |
| --- | --- |
| Fast path (cached `yᵀz` test) | ~10⁶ |
| Slow path with column-level warm starts | ~10⁴–10⁵ |
| Cold-solve baseline (HiGHS from scratch) | ~10²–10³ |

Headline metric: **fast-path hit rate × p50/p99 latency** under realistic load.

## Explicitly out of scope (v0)

Networking, persistence, multi-venue routing, fee engine beyond `maker_fee = 0`,
authentication, market-data feeds, GUI beyond a TUI/SVG, lock-free
concurrency, multi-threading.
