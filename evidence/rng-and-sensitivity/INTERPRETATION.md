# Item 6 — interpretation of completed sensitivity

All 320 preregistered cells and 80 within-condition/seed blocks passed; 300 new native runs and 20 accepted reused controls. Ten repetitions were fixed before additional simulation. The implementation is the archived factorial testbed, not the corrected MAC candidate.

## Direction, magnitude and precision

| Condition | G PDR effect (pp), 95% CI | G energy/node/day (J), 95% CI | G energy/delivery (mJ), 95% CI |
|---|---|---|---|
| baseline | -5.7485 [-7.1588, -4.3382] | -0.3987 [-0.4974, -0.3000] | 0.2238 [0.1043, 0.3433] |
| gamma_higher | -5.2489 [-6.2415, -4.2563] | -0.3912 [-0.4646, -0.3178] | 0.1833 [-0.0379, 0.4045] |
| gamma_lower | -5.8905 [-7.8083, -3.9726] | -0.3999 [-0.5291, -0.2707] | 0.1935 [0.0903, 0.2968] |
| load_higher | -3.9809 [-4.9805, -2.9814] | -0.5498 [-0.6875, -0.4120] | 0.2596 [0.1629, 0.3563] |
| load_lower | -5.2815 [-6.0570, -4.5060] | -0.1824 [-0.2105, -0.1544] | 0.2310 [0.1391, 0.3230] |
| warmup_0d | -5.5405 [-6.8934, -4.1877] | -0.3630 [-0.4566, -0.2695] | 2.0419 [1.6864, 2.3974] |
| warmup_1d | -5.7485 [-7.1547, -4.3424] | -0.3981 [-0.4966, -0.2996] | 0.2451 [0.1224, 0.3678] |
| warmup_3d | -5.7500 [-7.1592, -4.3407] | -0.3988 [-0.4973, -0.3002] | 0.2271 [0.1038, 0.3504] |

The gateway PDR effect ranges from -5.8905 to -3.9809 pp, with every pointwise interval below zero. The airtime PDR effect is also negative in all eight conditions. This is bounded robustness at one stationary density and one ADR configuration; the conditions share seeds and are not eight independent replications.

Gateway energy/node/day decreases in all eight conditions, with negative pointwise intervals. Energy per successful delivery has a positive point estimate throughout; the gamma-higher interval crosses zero, so its increase is unresolved there. Lower total expenditure must not be described as improved delivery efficiency.

The 1-pp half-width target is met in three conditions and missed in five. Achieved G/PDR half-widths range from 0.7755 to 1.9178 pp; the reference half-width is 1.4103 pp. The ten-repetition ceiling is respected; no significance-based stopping or further extension is used.

## Changes relative to the reference

A higher offered load changes the airtime effect by +1.6306 pp, CI [0.1960,3.0652], whereas a lower load changes it by -2.3248 pp, CI [-3.9312,-0.7184]. Thus airtime-effect magnitude depends on traffic in these tested ranges. Gateway-effect differences between the traffic/propagation alternatives and the reference have intervals spanning zero. Their changed point estimates alone do not demonstrate changed effects or equivalence.

Warm-up one versus two days changes the gateway PDR contrast by less than 0.001 pp (CI about [-0.0080,0.0080]); three versus two days gives -0.0014 pp, CI [-0.0075,0.0047]. Including the initial two days changes it by +0.2080 pp, CI [0.1486,0.2674], without reversing its negative sign.

The gateway energy/delivery contrast changes by +1.8181 mJ for zero versus two-day warm-up, CI [1.5112,2.1249]. The one-day change is +0.0213 mJ, CI [0.0045,0.0382]; the three-day change is +0.0033 mJ, CI [-0.0241,0.0307]. Initialization is therefore material to this energy contrast, while one-to-three-day window choices leave the primary PDR contrast nearly unchanged. These comparisons use their actual window durations and paired differences; raw total energy over unequal windows is not compared.

## Evidence limits

Intervals are pointwise and exploratory, not multiplicity-adjusted discovery tests. Gamma alternatives are model-parameter stress bounds, not fitted physical environments. Existing daily probes show nearly constant post-two-day output rates while blocked queues can continue growing; rate stability does not prove queue equilibrium. The known MAC progress and scheduler-header limitations remain explicit. No result is generalized to another density, algorithm, toolchain or radio hardware without its own evidence.
