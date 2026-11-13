# Item 2 event and measurement dictionary

The tested instrumented source is a separate copy of frozen rc11. No RNG
calls, scheduled events, packets or tags were added by instrumentation. In ten
short configurations, every scalar exactly matched the historical library
with tracing both off and on. This is a tested equivalence statement for
those paths, not a proof for every possible event sequence.

## Identifiers and windows

`packet_tree` is the native OMNeT++ packet tree ID, preserved on packet
duplication and through the existing forwarding path. Uplinks additionally
carry the native MAC `sequence`; application generation uses the monotonic
application index, which is checked against the same packet tree. `device_id`
is the ED vector index, with an explicit address-to-index mapping learned at
MAC admission. MAC addresses are not assumed to be sequential integers.
ADR commands have separate packet trees and record their triggering uplink
sequence when issued. `attempt` increments at actual PHY transmission start;
it is zero before that start and may exceed one if a native packet is sent
more than once.

All events from day 0 to 12 are retained. `in_measurement_window` uses the
simulator's exact time comparison against warm-up (day 2), avoiding rounding
ambiguity in CSV decimal timestamps. Primary packet comparisons use the
cohort generated during days 2--12; reception-time counts are also retained
separately. Packets generated before day 2 and active afterward, and packets
still pending at day 12, are reported separately. Pending does not imply a
radio loss.

`packet_bytes` is the packet length at the recorded layer. It is 20 B at
application generation and includes the MAC header at the PHY; the simulator
preamble chunk is excluded from PHY payload length. `PHY_CREATED` also records
the exact calculator input in `value2`. Start/end times refer to effective
transmission intervals for `PHY_TX_START`/`PHY_TX_END`, and to the associated
source transmission for reception events. Propagation arrival time is the
reception event timestamp; it differs from source start time.

## Events

| Event | Meaning | Extra fields |
| --- | --- | --- |
| `APP_GENERATED`, `APP_HANDOFF_MAC` | Application packet creation and delivery to MAC gate | SF/TP/channel at generation; monotonic sequence |
| `MAC_ADMITTED` | Existing MAC queue accepted the packet | `reason`: current FSM state; `value1`: queue depth |
| `MAC_SUBMIT_PHY`, `MAC_TX_FINISHED` | MAC submission and completion of its transmission | Native sequence/tree; completion precedes deletion of the current frame |
| `MAC_PENDING_CURRENT_AT_END`, `MAC_PENDING_QUEUE_AT_END` | Snapshot of frames remaining at simulation end | Boundary-pending state, not a failure classification |
| `PHY_CREATED`, `PHY_TX_START`, `PHY_TX_END` | ToA construction and actual RF start/end callbacks | Start/end seconds; PHY creation gives ToA and physical payload bytes |
| `PHY_DROP_WRONG_MODE`, `GW_RADIO_DROP_BUSY`, `MAC_DROP_DOWNLINK` | Explicit existing drop branches | Recorded radio/MAC condition |
| `RX_GATE_SENSITIVITY_PASS/FAIL` | Native sensitivity query outcome | Received W, sensitivity W, signal part |
| `RX_GATE_COLLISION_FAIL` | Native collision/capture rule rejected reception | Signal part; repeated queries are retained |
| `RX_GATE_SNIR_PASS/FAIL` | Native SF-specific SNIR query | Measured minimum SNIR and required SNIR, dB |
| `GW_RX_START`, `GW_RX_INTERVAL_END` | Gateway arrival interval callbacks | Associated packet tree and source TX interval |
| `GW_RX_SUCCESS/FAILED`, `GW_RX_IGNORED_TX`, `GW_RX_ABORTED` | Gateway decode, unavailability or abort branch | Recorded decision cause, where available |
| `GW_FORWARD_UP`, `SERVER_ARRIVAL`, `SERVER_PROCESSED`, `SERVER_UNIQUE`, `SERVER_DROP_OLD` | Forwarding and server processing/deduplication | Forwarder records SNIR/RSSI; processing records generation timestamp and accounting eligibility |
| `ADR_ISSUED` | Server creates a concrete command | `value1..4`: old SF, requested SF, old TP, requested TP; `sequence`: triggering uplink |
| `GW_DOWNLINK_REQUEST`, `GW_DOWNLINK_SCHEDULED`, `GW_DOWNLINK_REJECT_DUTY` | Gateway arrival, admission or duty rejection | Scheduling gives holdoff and ToA seconds |
| `ED_RX_SUCCESS/FAILED`, `ADR_RECEIVED`, `ADR_APPLIED` | ED radio result and command arrival/application | State events give SF before/after and TP before/after |
| `ADR_LOCAL_RECOVERY` | Existing local no-feedback recovery | SF/TP before/after; no command tree |

Raw reception queries are not disjoint packet-loss counters. The ledger groups
them by packet tree, retains all observed flags and documents a precedence
rule for a countable summary. Unresolved cases are reported as a residue.
Initial/end SF and TP remain available in the ordinary per-device scalars.

## Energy intervals

`events.csv.energy.csv` logs each interval integrated by the existing energy
consumer. Fields are ED index, interval start/end, start clipped at warm-up,
measured duration, and TX/RX/idle/sleep/other power in W. Energy is W times
the recorded measured seconds. These five groups match the paper's native
accumulators; receiving and busy share the RX group and the same configured
9.7 mA current. Their energy is not claimed to distinguish those two receiver
substates. Active seconds are reported per group; simultaneous power groups,
if present, must not be summed as exclusive elapsed time.

The analyzer reconciles each ED's components and total against its scalars
with absolute tolerance 1e-8 J and relative tolerance 1e-10 (floating-point
accumulation order). It checks continuous interval coverage from zero through
the configured horizon. Gateway and server energy are excluded.

## Interpretation boundary

The nine probes have one repetition per cell and describe execution mechanisms.
They cannot supply five-repetition confidence intervals or identify a causal
mediation effect. Seed-set pairing alone does not guarantee identical traffic
trajectories if shared RNG consumption changes. The factorial experiment in
item 3 is the next gate for separating airtime, gateway and adaptation effects.
