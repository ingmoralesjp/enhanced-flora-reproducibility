# Item 6 — preregistered bounded sensitivity

This design is frozen before additional performance simulations. It diagnoses
the same implementation used by item 3, including its documented MAC limitation;
it is not a performance evaluation of the separately corrected item-4 candidate.

One gateway, stationary 500-ED deployment, 12 days, ADR enabled. Within every
condition and seed, the four A/G cells are paired for deployment and offered
traffic. Application traffic has its own RNG stream, distinct from deployment,
path loss and network delay. The instrumentation adds no random draws.

Baseline plus half/double offered traffic, gamma 2.0/2.7 versus 2.32 at fixed
sigma=7.08 dB and the same 1-km Friis reference, and warm-up 0/1/3 days versus
2 days. Gamma 2.0 brackets the free-space distance exponent and 2.7 tests a
larger loss slope; these are model-parameter stress bounds, not fitted urban
or suburban calibration claims. Vary one scenario parameter at a time. Initial
and subsequent interpacket means change together in the traffic conditions.

The primary contrast is G with ADR enabled, averaged over A. Also report A and
the AG interaction for PDR, measured energy/node/day and energy/delivery.
Warm-up comparisons use their actual 12/11/10/9-day measurement windows;
energy/node/day is normalized to avoid confusing window length with power.
The generation cohort defines the PDR denominator; queued packets remain pending.

Use the five delivered baseline repetitions to estimate precision; a useful
target is 1 pp PDR half-width. Fix 10 repetitions for every condition **before**
the new runs. The estimate may require more than the authorized 10, in which
case report achieved precision and do not extend or stop on significance.
Reuse the 20 accepted baseline A/G runs at 500 EDs and seeds 0–4; run only
the 300 missing cells with at most 8 simultaneous native processes.

All intervals are pointwise paired Student-t intervals with 9 df. Direction
stability is descriptive unless the interval resolves the sign. Differences
between scenario contrasts are themselves paired by seed. No hardware,
multi-gateway or mobility-causation claim is introduced.
