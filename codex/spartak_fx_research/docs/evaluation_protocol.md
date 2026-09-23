# Spartak FX evaluation protocol

## Status of the trading rules

The executable Spartak entry and exit rules are **source pending**. No indicator
period, threshold, stop distance, target, filter, session, or risk percentage is
assumed by this code. `StrategyParameters::source_pending` defaults to `true`,
and the backtest refuses to run until a reviewed rule revision explicitly locks
it to `false`.

Every implemented rule should eventually carry a stable rule ID and a source
reference containing the video/document name, timestamp or page, and a short
interpretation note. Ambiguous source statements must become competing,
explicitly named variants rather than an undocumented implementation choice.

## Deterministic information and execution timing

1. A strategy receives only bars through the current bar close. Future bars are
   not present in the span passed to `on_bar_close`.
2. A signal produced at close of bar `t` is eligible to enter only at the open
   of bar `t+1`. A signal on the final bar is not requested.
3. Only one position may be open. After an intrabar exit, a new close signal may
   be created for the following bar.
4. Signal stop-loss and take-profit values are executable exit-quote levels:
   bid levels for a long and ask levels for a short. A next-open gap that makes
   those levels invalid rejects the signal rather than silently rewriting it.

## Bid/ask and transaction costs

Input OHLC values are mid prices. A bar-specific spread overrides the configured
default spread. Quotes are reconstructed as `mid ± spread/2`:

- long entry buys at ask; long exit sells at bid;
- short entry sells at bid; short exit buys at ask.

Slippage is always adverse and is applied to every entry and exit. Fees contain
an explicit fixed amount per order plus basis points of fill notional. There are
no hidden cost defaults: a zero in the experiment configuration means the
researcher intentionally requested zero for that component.

## Risk sizing

At entry, the risk budget is current realized equity multiplied by the explicit
per-trade risk fraction. Quantity is solved so that a normal stop exit, including
adverse exit slippage and both fixed and proportional fees, consumes that budget.
Quantity is then capped by gross leverage and rounded down to the configured
quantity step.

This is not a guarantee against gap loss. A stop-market gap is filled at the
first available exit quote plus adverse slippage and may lose more than the risk
budget. Experiments must report such events separately when production reporting
is added.

## Intrabar assumptions

OHLC bars do not reveal the path between high and low. If both stop and target
are reachable in one bar, the engine selects the stop. This pessimistic rule also
applies to the entry bar. Stop gaps use the worse opening quote; favorable target
gaps are conservatively filled at the target rather than receiving improvement.

The current drawdown statistic uses equity after closed trades. It is not an
intrabar mark-to-market drawdown and must be labelled accordingly.

## Chronological partitions

Each instrument must be divided once, in timestamp order, into:

1. **IS** — strategy development and broad parameter exploration;
2. **validation** — parameter choice and cross-instrument ranking;
3. **blind OOS** — a final audit only after the leaderboard is locked.

The splitter accepts explicit counts or caller-supplied fractions; it contains no
assumed research ratio. Optional equal embargoes separate IS from validation and
validation from OOS. All three partitions must be non-empty. Parser cleaning,
timezone normalization, duplicate removal, and resampling rules must be fixed
before splitting and applied identically without looking across a protected
boundary.

Indicator warm-up must be handled inside each partition or by a predeclared
read-only warm-up prefix whose P&L is excluded. It must never import labels,
signals, positions, or fitted state from the later partition.

## Grid optimization and the top-50 lock

The grid is supplied explicitly as named axes; the library invents no strategy
values. For every instrument and parameter combination, IS and validation are
run independently from the same initial-equity convention. Candidates below the
declared minimum trade counts are excluded.

The optimizer retains one validation winner per instrument, sorts those winners
by the caller-supplied validation objective, applies deterministic tie breaks,
and truncates to `top_k` (50 for the requested study). The resulting immutable
`LockedLeaderboard` contains a deterministic lock ID derived from instrument,
rank, parameters, and IS/validation scores.

Blind OOS data is not an argument to optimization. It can enter only through
`audit_locked_oos`, which requires the locked leaderboard. Audit rows preserve
validation rank and are never sorted by OOS result. OOS must not be used to:

- change parameters, filters, costs, universe, or data cleaning;
- drop an instrument;
- reorder the top 50;
- choose a different objective;
- decide which earlier experiment to publish.

Any such change creates a new research cycle and requires a genuinely untouched
future period. Report all locked OOS rows, including failures.

## Minimum audit record

Persist the following beside every result:

- raw-file identity/checksum and parser version;
- instrument type (spot FX, metal, crypto, or other), timezone, bar interval,
  and available history range;
- split boundaries and embargo size;
- rule revision and source references;
- complete parameter grid and validation objective;
- spread, fee, slippage, leverage, quantity-step, and initial-equity settings;
- rejected-signal count, trades, P&L, closed-equity drawdown, and lock ID;
- build/compiler version and deterministic run identifier.

Until the source-derived strategy replaces `SourcePendingStrategy`, any test
strategy in `tests/` is only an execution-engine fixture and is not a claim about
the Spartak method.
