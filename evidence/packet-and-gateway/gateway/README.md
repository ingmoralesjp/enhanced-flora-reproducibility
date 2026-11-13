# Observation 5 — actual gateway RF occupancy

This delivery follows the verified item-4 delivery. Its controlled testbed is a
separate copy of the item-4 candidate; it does not change that source, the
historical 900-run campaign or the 120-run factorial. A small injection adapter
passes downlinks through the **native MAC, transmitter, gateway radio and RF
event timers**. It does not replace the PHY with a sink. The sole existing NED
assignment made overridable is the gateway MAC typename; its default stays
LoRaGWMac. Raw source, configuration, commands, receipts and traces are supplied.

## Definition and measurement window

RF occupancy is the duration of the union of effective gateway transmission
intervals, clipped to the measurement window, divided by window length. Both
native PHY start and end records are checked. A carry-in transmission is clipped
at warm-up rather than omitted; an interval extending beyond the horizon is
clipped at the horizon. Scheduled admissions are joined to RF transmissions by
stable packet-tree identity. Duplicate scheduling identities, unmatched native
transmissions, missing completed-RF end records or admission balance failures
prevent acceptance.

The controlled runs last 3600 seconds and measure [600,3600), with one gateway,
one inactive ED and no uplink/ADR traffic. Downlink requests cycle SF7–12,
physical lengths 16/24/32 B, CR4 (4/8), BW125kHz, 14 dBm and two frequencies.
Low load requests every 400 seconds; high load every 0.5 seconds, stopping at
3500 seconds. Explicit and implicit header cases are separate. These are
deterministic component-load controls, not confidence-interval estimates.

The additional audit reuses the untouched 120 factorial traces, measuring
[172800,1036800) seconds (days 2–12). It does not rerun those simulations or
combine their MAC implementation with the corrected candidate. Each trace is
verified against its original accepted receipt. The native GW forwarded/rejected
scalars count the **full horizon**, so they reconcile against full-horizon
events, separately from the measurement-window table.

## Resource and boundary contract

`waitingForDC` and one `dutyCycleTimer` govern the whole gateway. This is a
global gateway resource across channels, not independent channel or subband
budgets. Alternating frequencies does not create additional budget. The
corrected G1 treatment schedules each next release from its calculated
downlink duration /0.01. Release slack and the slack against actual RF
duration /0.01 are checked separately for every consecutive start, including
warm-up. A 1e-8-second tolerance exceeds double conversion error at 12-day
timestamps while remaining negligible compared with LoRa symbols. Native
SimTime itself remains picosecond based.

This verifies the implemented start-spacing contract, not a claim of legal
regional compliance or an exact bound on every arbitrary finite observation
window. Window boundary effects must be distinguished from spacing violations.
Legacy G0 intentionally uses recovered constants and can violate spacing
relative to a 1% actual-RF budget even when its long-window occupancy is low.

## Header-duration discrepancy

The scheduler calls the calculator with `useHeader=true`. The physical gateway
radio takes the frame's `LoRaUseHeader` value, whose default is false for the
observed ADR downlinks. Therefore a shared function does not, by itself,
guarantee identical input assumptions. In the controlled implicit-header cases,
scheduled duration exceeds RF duration and the wait is conservative; explicit
header cases align within the numerical tolerance. The same discrepancy occurs
in the corrected-airtime factorial traces. It is retained and measured, not
silently corrected or pooled into the published campaign.

Consequently, scheduled airtime/holdoff equals 1% by construction but is neither
actual RF airtime/holdoff nor RF occupancy. `REPORT.md` presents all three.
No inference about PDR or device energy follows from this gateway-only test.

## Recompute and provenance

Run `analyze_gateway_occupancy.py --campaign data/controlled --output NEW_DIR
--kind controlled` and likewise `--campaign data/factorial --kind factorial`.
The supplied factorial counter extracts contain only the original full-horizon
GW scalar lines. They are explicitly named `gateway-scalars.sca`, not original
run files. Their hashes and the original full-SCA hash are linked by
`source-sca-provenance.json` to the unchanged accepted receipt. The complete
original SCAs remain in the item-3 archive (SHA-256
`2a2cfd3dc2c0c45eb0374cfd5e33c5a0d28f69346149e6cf70423a1af6541737`).
This keeps the occupancy delivery self-contained for its numerical analysis
without duplicating 2.2 GiB of unrelated scalar records.

Controlled native reruns use the item-1 dependency versions and the supplied
`testbed/flora` sources. Build with `make makefiles` and
`make MODE=release INET_PROJ=/your/inet -j8`, then supply OMNeT++ and INET paths
to the runner. `controlled-v1` records the preserved adapter-configuration setup
failure; all four `controlled-v2` native executions are accepted.
