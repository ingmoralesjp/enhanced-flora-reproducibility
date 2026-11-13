# Interpretation of the paired factorial results

The results below are computed from the 120-run, five-repetition factorial
matrix in this directory. They describe the archived implementation with its
known MAC progress path; they are not a ranking of competing ADR algorithms
or a claim of hardware fidelity. Confidence intervals are pointwise 95%
Student-t intervals of five within-seed contrasts (df=4).

## Gateway scheduling depends on adaptation

In fixed-SF/TP controls, no ADR downlinks are generated and the conditional
gateway effect is zero. With ADR enabled, the actual-ToA gateway holdoff
changes PDR by -2.876 pp at 100 EDs (CI -7.128 to 1.375), -6.658 pp at
500 EDs (CI -8.976 to -4.341), and -4.748 pp at 1000 EDs (CI -5.752 to
-3.743), averaging over the two airtime treatments within each seed.
The 100-ED interval spans zero. The two larger-density intervals resolve a
negative conditional effect in this experiment.

Because the D=0 conditional effect is zero, the gateway-by-adaptation
interaction equals that D=1 effect under the declared difference-of-
differences scaling. At 1000 EDs, the marginal gateway main effect is
-2.374 pp, half the conditional ADR-enabled effect; reporting only that
marginal average would conceal its dependence on adaptation.

## Airtime also interacts with adaptation

The airtime main effects are -4.832 pp at 100 EDs (CI -6.927 to -2.737),
-4.084 pp at 500 EDs (CI -5.391 to -2.778), and -3.436 pp at 1000 EDs
(CI -5.241 to -1.632). The airtime-by-adaptation interactions are -4.671,
-6.811 and -5.819 pp respectively, with the three pointwise intervals
excluding zero. Thus the timing change is not an additive offset independent
of adaptation in the tested implementation.

The airtime-by-gateway and three-factor PDR intervals span zero at each
evaluated density. This is unresolved evidence with five repetitions,
not proof of no interaction. No multiplicity-controlled discovery claim
is made from these many exploratory contrasts.

## Total energy and energy per delivery answer different questions

At 1000 EDs with ADR enabled, changing gateway scheduling lowers mean energy
by 3.299 J per node (CI -3.980 to -2.618), while raising energy per delivery
by 0.651 mJ (CI 0.241 to 1.060). Fewer active transmissions/RX windows can
therefore reduce the total while successful deliveries become more costly.
The packet and queue outcomes must accompany these energy figures.

Enabling network ADR against the control that freezes the same initial
per-device SF/TP increases PDR in this particular matrix. Its marginal effect
is +57.575 pp at 100 EDs, +25.562 pp at 500 EDs and +15.783 pp at 1000 EDs.
This comparison changes whether adaptation exists at all; it is not evidence
that the reference ADR is superior to another ADR algorithm. Initial radio
assignments, deployment and offered-traffic fingerprints match within every
seed/density block.

## Implementation and uncertainty boundaries

The item-2 replay demonstrates that received-frame timing can consume window
timeouts and leave long queues in WAIT_DELAY_2. Retaining that implementation
is necessary for this controlled factorial diagnosis, but the effects must
not be described as pure channel/PHY losses. A separately versioned MAC
correction and regression tests are required before extending conclusions to
a liveness-corrected release.

Two diagnostic loss counters use Packet-pointer suppression sets and are
allocation-sensitive. Their pilot differences are preserved; they are not
primary outcomes or unqualified loss probabilities. Stable-identity counting
belongs to the component verification/correction work in item 4.

Five repetitions give wide intervals for some metrics. For example, the
1000-ED airtime main effect on energy per delivery is +31.018 mJ with CI
-8.330 to 70.365. This result does not resolve its sign. Item 6 must use the
observed variation to define a bounded precision/sensitivity design, rather
than treating every point estimate as established.
