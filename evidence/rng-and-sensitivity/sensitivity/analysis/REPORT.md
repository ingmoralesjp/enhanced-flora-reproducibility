# Item 6 — sensitivity and precision

All 320 preregistered cells are accepted and paired, including 20 reused baseline runs and 300 additional native runs. No sample size was chosen from significance.

| Condition | G effect on PDR (pp) | 95% CI (pp) | Half-width (pp) | Paired n |
|---|---:|---|---:|---:|
| baseline | -5.7485 | [-7.1588, -4.3382] | 1.4103 | 10 |
| gamma_higher | -5.2489 | [-6.2415, -4.2563] | 0.9926 | 10 |
| gamma_lower | -5.8905 | [-7.8083, -3.9726] | 1.9178 | 10 |
| load_higher | -3.9809 | [-4.9805, -2.9814] | 0.9995 | 10 |
| load_lower | -5.2815 | [-6.0570, -4.5060] | 0.7755 | 10 |
| warmup_0d | -5.5405 | [-6.8934, -4.1877] | 1.3528 | 10 |
| warmup_1d | -5.7485 | [-7.1547, -4.3424] | 1.4062 | 10 |
| warmup_3d | -5.7500 | [-7.1592, -4.3407] | 1.4093 | 10 |

The G effect averages gateway treatment differences across the two airtime conditions with ADR enabled. A averages the airtime effect across G; AG is a difference of differences. Scenario-minus-baseline changes are paired by seed; their own intervals are supplied rather than inferred from overlap between separate intervals.

Energy/node/day uses the actual 12/11/10/9-day measurement window for warm-up 0/1/2/3 days. Energy/delivery and PDR use the same generated-packet cohort/window conventions as the original experiment. Pending MAC packets are not silently dropped from the denominator.

This study retains the item-3 MAC progress behavior and the scheduler header assumption identified by items 2/5. It tests robustness of those implementation-conditional effects, not the performance of the separately corrected MAC candidate. Gamma alternatives are stress bounds at fixed sigma/reference, not empirical calibration. Warm-up sensitivity does not itself prove stationarity.

The prior five-seed SD predicted a 1.335-pp half-width at n=10 and a normal-approximation requirement of 14 for a 1-pp target. The user ceiling is 10; achieved precision is reported even when the target is missed. Pointwise intervals are exploratory, with no multiplicity-controlled discovery claim. No failed run is assigned PDR and undefined energy/delivery remains undefined.
