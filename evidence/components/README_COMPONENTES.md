# Observation 4 — quantitative component verification

This is a separate, dated test/correction candidate. It does not replace the
sources, binary or results of the historical 900 runs, item-2 probes or item-3
factorial. `testbed/flora/component-changes.patch` identifies every production
change; `parent-source-hashes.json` identifies the immediate parent files.

## Native implementation and independent expected values

`ComponentProbe.cc` invokes actual C++ component methods in an initialized
OMNeT++/INET network. `component_oracle.py` computes expected values separately;
it does not call those C++ methods or import their outputs as expected values.
The outputs are per-case CSVs with inputs, expected and observed values, error,
tolerance and acceptance. A component adapter replaces the gateway's external
PHY sink for the controlled scheduling test, while executing the production
`LoRaGWMac::handleUpperMessage` and actual event timers. This tests scheduling,
not successful RF reception. Item 5 measures actual RF intervals separately.

| Correction | Independent reference and test | Acceptance |
|---|---|---|
| F1 noise | Linear noise-density × bandwidth × noise-figure calculation, versus native logarithmic implementation; BW 125/250/500 kHz, NF 0/3/6/9 dB | Relative error ≤1e-12 in W; the model uses the rounded convention −174 dBm/Hz, not a specified temperature-dependent radio |
| F2 sensitivity | Inherited upstream FLoRa sensitivity table, independently transcribed from its official source; native power sweeps ±1 and ±0.01 dB for SF7–12 | Exact decisions outside equality; table values within 1e-10 dB |
| F2 SNIR | Specified SF thresholds −7.5 to −20 dB in 2.5 dB steps; controlled linear SINR calculation, and independent Boolean combination of sensitivity, capture and SNIR gates | Exact decisions away from the threshold; computed linear SNIR relative tolerance 1e-10, absolute floor 1e-12 |
| F2 capture/inter-SF | Independently transcribed 6×6 design threshold matrix, all 36 SF pairs, six desired/interferer ratios each | Exact capture decisions for the selected non-boundary ratios; also verify that every co-channel interfering SF contributes power to analog noise and to the joint reception decision |
| F3 ToA | Semtech SX1276/77/78/79 Rev.4 §4.1.1.7, p.31; independent rational arithmetic counts coded symbols | ≤2 ps total duration, covering separate rounding of the three native SimTime parts |
| F4 scheduling | Independent chronological recurrence of admission and ToA/0.01 deadlines, accounting for FIFO ordering of equal-time events | Exact admission/rejection, ≤2 ps airtime, ≤0.2 ns holdoff |
| F5 TX lookup and energy | Archived XML table as input data; independent integer rounding/clamping and E=VIΔt in SI units | Current ≤1e-12 mA; energy ≤1e-12 J |

Primary references: [Semtech datasheet](https://cdn-shop.adafruit.com/product-files/3179/sx1276_77_78_79.pdf),
[Semtech AN1200.22](https://www.mouser.com/pdfdocs/an120022.pdf),
and [Croce et al., DOI 10.1109/LCOMM.2018.2797057](https://doi.org/10.1109/LCOMM.2018.2797057).
The Croce paper supports imperfect SF orthogonality as a mechanism; this package
does **not** claim that a design-matrix conformance test validates a physical
receiver or reproduces that paper's hardware data. The local upstream source is
hashed in the provenance report as the independent implementation contract for
the inherited sensitivity/capture table.

The 12 exact-equality sensitivity/SNIR cases are boundary observations: logarithm
and exponentiation can round one ULP to either side of an inclusive dB threshold.
Their decisions are preserved, with a binary tolerance of one; they are not
counted as evidence of precise boundary fidelity. The 48 below/above decisions
require exact agreement. No PER curve or empirical radio claim is inferred.

An additional isolated adapter, `testbed/energy-boundary`, verifies the complete
radiated-watts → dBm → selected-current → supply-power path at 73 powers from
−1 to 17 dBm in 0.25 dB steps, including clamping beyond the campaign's allowed
2–14 dBm. Four state integrals verify TX/RX/idle/sleep clipping at warm-up.
All 77 comparisons pass in `results/native-energy-v1`. This adapter supplies
controlled states to the initialized production consumer; it leaves the active
MAC regression library unchanged. `prepare_energy_boundary.py` and
`energy_boundary_oracle.py` preserve its generation and independent calculation.

## Scope of the airtime sweep

The archived calculator exposes SF7–12, bandwidth, physical bytes, CR=1–4 and
explicit/implicit header. It fixes CRC on, preamble 8 symbols and automatic DE
when symbol duration ≥16 ms. The archived function is copied with only its
namespace/header guard renamed into `ArchivedAirtimeCalculator.h`; its 1008
default cases are tested against the independent oracle. The candidate adds
optional CRC and DE arguments to the calculator, preserving all original call
defaults. Its 6048 cases additionally test CRC off/on and DE automatic/off/on.
These overrides are component test capabilities; they were **not** options of
the transmitter in the historical campaign. A 37-byte ordinary PHY frame is
20 application bytes plus 17 inherited MAC bytes.

## Why F5 did not change the campaign energy

`results/F5-campaign-audit.json` checks every F4–F5 pair (150), including total
and component measurement energy, generated/delivered packets and PDR. All are
equal. The 165000 device summaries in those stages have observed initial/final
TX powers in {2,4,6,8,10,12,14} dBm. Code initializes integer powers and ADR
changes them by integer steps with integer limits. Both lookup paths select the
same XML current at every integer from 2 through 14; all 13 values are exercised
in the component test. F5 therefore exercises a different, robust lookup path
without changing the current on the powers used. It is not an energy-saving
effect. Initial/final summaries alone are not complete event trajectories.

## Separately versioned diagnostic corrections

The candidate uses `Packet::getTreeId()` for once-per-identity diagnostic sets.
The regression deliberately constructs 100 distinct packets at the *same*
memory address and calls the sensitivity gate twice each: the expected count is
100, not 200 and not 1. These sets guard counters only, not reception decisions.
The identity-only build retains the original MAC, to check all common primary
scalars against its parent on ten native short configurations. Differences in
the two allocator-sensitive diagnostics remain recorded separately.

The MAC candidate adds no-event deadline catch-up transitions after returning
from reception to a state whose timeout already fired. It introduces no timer,
packet or new deadline. Three direct native FSM cases cover late RX1 drop,
late RX2 drop and the consumed-RX1/RX2 timeout path. Native replay additionally
uses the observed item-2 seed, and full-horizon controls cover 100 and 1000 EDs.
The old queue-blocking result is retained as a reproducer; changed primary
results from the candidate are new results, never replacements for the 900.

The uncorrected native component adapter retains four expected failures in
`original-component-repro-v1`: the same-address count adds zero in that run
(the address was already present from earlier test packets), and the three
expired-deadline paths end in LISTENING_1, WAIT_DELAY_2 and LISTENING_2.
The corrected adapter adds exactly 100 identities and ends all three in IDLE.
All other component comparisons pass in the uncorrected adapter. These expected
failures are regression evidence and are not counted as accepted network runs.

## Reproduction

Use the OMNeT++/INET versions and dependency instructions delivered with item 1.
Set up OMNeT++ on PATH and supply INET as a user-selected directory. No source
changes to INET are needed by these tests. The generator and runner locate this
project relative to their own files. In the supplied `observation-4` directory,
choose your own installed dependency locations:

```bash
export FLORA_TEST_OMNET=/path/to/omnetpp
export FLORA_TEST_INET=/path/to/inet
python3 run_component_suite.py --omnet-dir "$FLORA_TEST_OMNET" \
  --inet-dir "$FLORA_TEST_INET" --output results/my-component-check --build
```

The source snapshot is already supplied. To regenerate it instead, use
`prepare_components.py --output /a/fresh/output/directory`; no existing source
is overwritten. Native regression reproduction additionally builds the
`identity-only` and parent `../observation-3/testbed/flora` sources using the
same `make makefiles` and `make MODE=release INET_PROJ=... -j8` commands.
Then run `run_regressions.py --omnet-dir ... --inet-dir ... --parent-library ...
--output /a/fresh/regression/directory --jobs 8`. The extended 1000-ED run is
computationally expensive. Do not relaunch this runner while it is active.

The archived raw commands and test inputs are evidence of the executed tests.
`run_component_suite.py` provides a portable invocation with explicit OMNeT++
and INET directories and fresh output paths. Failed setup attempts and the first
scheduler-oracle failure are retained: the latter exposed FIFO rejection at an
exactly equal release timestamp, resolved in the documented independent event
ordering contract rather than by changing the scheduler or deleting a case.
