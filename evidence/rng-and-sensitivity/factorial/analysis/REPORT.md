# Item 3 — paired factorial experiment

Accepted and analyzed: 120/120. Five repetition blocks per density when complete.

The seven factorial effects and two conditional gateway effects are computed within each seed block before estimating confidence intervals. They are not contrasts between eight independently estimated means.

Main effects average over the other two factors. Two-factor interactions are differences of differences averaged over the third factor. The three-factor interaction is the difference between two-factor interactions. Intervals are pointwise 95% Student-t with four degrees of freedom; they are not multiplicity-adjusted discovery claims.

Primary results use the days 2–12 measurement window. PDR refers to the application-generation cohort. PHY transmissions and energy are event/interval quantities in that window. Terminal pending counts identify generated packets still held at the MAC; they are not automatically RF losses.

The known MAC timeout path is deliberately retained for this controlled diagnosis. The two pointer-deduplicated loss counters are excluded from primary outcomes and their pilot differences are preserved. No hardware-validation claim follows from this factorial experiment.

## Completed-cell summaries

| Nodes | A | G | D | Runs | Mean PDR (%) | Mean ED energy/node (J) | Mean pending/node |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 0 | 0 | 0 | 5 | 10.2450 | 13.527450 | 774.1580 |
| 100 | 0 | 0 | 1 | 5 | 71.5083 | 48.754775 | 219.8600 |
| 100 | 0 | 1 | 0 | 5 | 10.2450 | 13.527450 | 774.1580 |
| 100 | 0 | 1 | 1 | 5 | 68.8039 | 46.955000 | 244.0680 |
| 100 | 1 | 0 | 0 | 5 | 7.7486 | 12.959731 | 795.7640 |
| 100 | 1 | 0 | 1 | 5 | 64.5125 | 46.653612 | 283.0140 |
| 100 | 1 | 1 | 0 | 5 | 7.7486 | 12.959731 | 795.7640 |
| 100 | 1 | 1 | 1 | 5 | 61.4639 | 44.472615 | 310.4000 |
| 500 | 0 | 0 | 0 | 5 | 2.6692 | 3.762550 | 840.9036 |
| 500 | 0 | 0 | 1 | 5 | 35.5109 | 24.354371 | 543.4160 |
| 500 | 0 | 1 | 0 | 5 | 2.6692 | 3.762550 | 840.9036 |
| 500 | 0 | 1 | 1 | 5 | 27.7636 | 19.085855 | 613.8228 |
| 500 | 1 | 0 | 0 | 5 | 1.9907 | 3.425323 | 846.8304 |
| 500 | 1 | 0 | 1 | 5 | 26.9318 | 19.612185 | 621.4600 |
| 500 | 1 | 1 | 0 | 5 | 1.9907 | 3.425323 | 846.8304 |
| 500 | 1 | 1 | 1 | 5 | 21.3627 | 15.622487 | 671.8564 |
| 1000 | 0 | 0 | 0 | 5 | 1.1940 | 2.048498 | 853.3630 |
| 1000 | 0 | 0 | 1 | 5 | 22.4844 | 15.497389 | 660.6762 |
| 1000 | 0 | 1 | 0 | 5 | 1.1940 | 2.048498 | 853.3630 |
| 1000 | 0 | 1 | 1 | 5 | 17.2882 | 11.988377 | 707.9468 |
| 1000 | 1 | 0 | 0 | 5 | 0.6673 | 1.473054 | 857.9454 |
| 1000 | 1 | 0 | 1 | 5 | 15.6901 | 11.545927 | 722.1172 |
| 1000 | 1 | 1 | 0 | 5 | 0.6673 | 1.473054 | 857.9454 |
| 1000 | 1 | 1 | 1 | 5 | 11.3908 | 8.457632 | 761.1774 |

## PDR effects and interactions

| Nodes | Contrast | Effect (pp) | 95% CI (pp) |
| ---: | --- | ---: | --- |
| 100 | A | -4.8322 | [-6.9274, -2.7369] |
| 100 | G | -1.4382 | [-3.5640, 0.6875] |
| 100 | D | 57.5754 | [54.1106, 61.0402] |
| 100 | AG | -0.1721 | [-1.0638, 0.7195] |
| 100 | AD | -4.6714 | [-8.8538, -0.4891] |
| 100 | GD | -2.8765 | [-7.1279, 1.3749] |
| 100 | AGD | -0.3443 | [-2.1275, 1.4390] |
| 100 | G_given_D0 | 0.0000 | [0.0000, 0.0000] |
| 100 | G_given_D1 | -2.8765 | [-7.1279, 1.3749] |
| 500 | A | -4.0843 | [-5.3908, -2.7778] |
| 500 | G | -3.3291 | [-4.4878, -2.1704] |
| 500 | D | 25.5623 | [22.8997, 28.2249] |
| 500 | AG | 1.0891 | [-0.9644, 3.1426] |
| 500 | AD | -6.8114 | [-10.5608, -3.0621] |
| 500 | GD | -6.6582 | [-8.9756, -4.3409] |
| 500 | AGD | 2.1782 | [-1.9288, 6.2853] |
| 500 | G_given_D0 | 0.0000 | [0.0000, 0.0000] |
| 500 | G_given_D1 | -6.6582 | [-8.9756, -4.3409] |
| 1000 | A | -3.4362 | [-5.2406, -1.6319] |
| 1000 | G | -2.3739 | [-2.8761, -1.8716] |
| 1000 | D | 15.7827 | [12.4281, 19.1373] |
| 1000 | AG | 0.4485 | [-1.4127, 2.3097] |
| 1000 | AD | -5.8192 | [-8.5061, -3.1323] |
| 1000 | GD | -4.7477 | [-5.7523, -3.7432] |
| 1000 | AGD | 0.8969 | [-2.8254, 4.6193] |
| 1000 | G_given_D0 | 0.0000 | [0.0000, 0.0000] |
| 1000 | G_given_D1 | -4.7477 | [-5.7523, -3.7432] |

The gateway effect at D=0 is the negative control: no ADR command or downlink is generated. Its zero effect, where verified, is an implementation control rather than evidence that duty scheduling never matters. The conditional D=1 effect and the GD interaction quantify dependence on adaptation. Queue and transmission contrasts must be read together with PDR and energy before assigning a mechanism.

All run-level outcomes, per-device distributions and paired seed contrasts are supplied as CSV. Undefined energy per delivery is retained as undefined; no failed execution is imputed.
