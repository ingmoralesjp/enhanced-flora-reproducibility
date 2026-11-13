# Observation 1 — parameters recovered from the executed package

Scope: the 900-run Enhanced FLoRa F0--F5 campaign, not the official FLoRa 1.1
reference or later bridge tests. Paths below are relative to the root of
`enhanced-flora-0.2.0-dev-rc11.tar.gz`. Values are from the packaged INI, NED,
XML and C++ files; an INI value is not automatically proof that the C++ module
uses it. The original files, not this summary, are authoritative.

## Configuration chain and design

`simulations/examples/fidelity-ablation.ini` includes
`Analisis-fidelity-ablation.ini`, which includes
`Analisis-mi-PROPOSAL-ADR.ini`. The base INI references
`simulations/energyConsumptionParameters.xml` and `simulations/cloudDelays.xml`.
The ablation INI overrides the inherited `thesisLegacy` ADR choice with
`lorawanReference`; the base's `useAllEu868Channels=true` and six-hour
heartbeat are likewise overridden to `false` and `0s`. Do not interpret the
inherited values as the executed values.

| Item | Executed value | Primary location |
| --- | --- | --- |
| Topology | One GW; 100, 200, ..., 1000 end devices | `Analisis-mi-PROPOSAL-ADR.ini`; `Analisis-fidelity-ablation.ini` |
| Population | Static, RandomWaypoint mobile, and 50/50 mixed (first half mobile) | `Analisis-fidelity-ablation.ini`; `simulations/package.ned` |
| Duration and sampling | 12 simulated days; first 2 days warm-up; five repetitions per cell; 900 runs in total | `Analisis-fidelity-ablation.ini` |
| Application offered load | 20 B application payload; exponential(1000 s) first and subsequent packet intervals; no heartbeat | Both included INIs; `src/LoRaApp/SimpleLoRaApp.cc` |
| Node uplink duty limit | No separate per-node duty-cycle regulator in this campaign; the exponential traffic generator is not a 1% node-duty implementation | `src/LoRaApp/SimpleLoRaApp.cc`, `src/LoRa/LoRaGWMac.cc` |
| Gateway downlink duty | Enabled, 0.01 fraction; F0--F3 use inherited SF constants and F4--F5 use actual downlink ToA / 0.01 start-to-start holdoff | `Analisis-fidelity-ablation.ini`; `src/LoRa/LoRaGWMac.cc` |
| EU868 channels | 868.1, 868.3, 868.5 MHz; initial channel randomized among these three; not all available channels | `Analisis-fidelity-ablation.ini`; `src/LoRaApp/SimpleLoRaApp.cc` |
| Initial radio | Random initial SF 7--12 and power from 2, 4, 6, 8, 10, 12, 14 dBm; 125 kHz; code redundancy 4, i.e. coding rate 4/8 | `Analisis-mi-PROPOSAL-ADR.ini`; `src/LoRaApp/SimpleLoRaApp.cc` |
| ADR | `lorawanReference`, representative maximum of the last 20 received quality samples, 15 dB device margin, 2.5 dB step divisor, no CSI notification input | `Analisis-fidelity-ablation.ini`; `src/LoRa/NetworkServerApp.cc` |
| Propagation | `LoRaLogNormalShadowing` with Friis reference at d0=1000 m, gamma=2.32, sigma=7.08 dB, 868 MHz, 0 dB system loss, transmitter and receiver gains of 1 dBi each (2 dB combined; not linear-unity gains) | `Analisis-mi-PROPOSAL-ADR.ini`; `src/LoRaPhy/LoRaLogNormalShadowing.cc` |
| Deployment | GW at (15000,15000) m; static devices in a 15 km-radius circle; mobile initial positions/destinations in a 0--30000 m square, **not** confined to that circle | `Analisis-mi-PROPOSAL-ADR.ini`; `src/LoraNode/LoRaNode.ned`; INET `RandomWaypointMobility.ned` |
| Mobility | Speed uniform(0.5,3) m/s; pause uniform(0,60) s; 1 s update | `Analisis-mi-PROPOSAL-ADR.ini` |

The sampling choice of two warm-up days and five repetitions is a recorded
design parameter. No stabilization or precision analysis was located in the
frozen package, so the values should not be described as empirically
justified by this handoff.

## PHY and physical bytes

| Item | Value / effective rule | Primary location |
| --- | --- | --- |
| Application versus physical bytes | `dataSize=20B` is the application payload. `includePosition` defaults to false, so no 8-B position field is added in this campaign. The uplink MAC header is inherited from INET `CsmaCaMac.ned` (17 B in the pinned INET 4.3.2 tree), giving 37 B at the LoRa PHY payload input for an ordinary uplink. F0--F2 nevertheless use fixed-byte legacy ToA, not a 37-B duration. | Base INI; `src/LoRaApp/SimpleLoRaApp.ned`; `src/LoRa/LoRaMac.ned`; `src/LoRa/LoRaMac.cc`; INET `CsmaCaMac.ned` |
| Corrected ToA | F3--F5 use actual PHY payload length, eight preamble symbols plus 4.25, explicit header, CRC enabled, and low-data-rate optimization when symbol duration is at least 16 ms (SF11/12 at 125 kHz). `CR=4` means 4/8. | `src/LoRaPhy/LoRaAirtimeCalculator.h`; `src/LoRaPhy/LoRaTransmitter.cc` |
| Legacy ToA | F0--F2 recover the inherited 20-B ED/15-B GW fixed-payload calculation with DE=0; it is an ablation treatment, not the physical byte count of the packet. | `src/LoRaPhy/LoRaAirtimeCalculator.h` |
| Noise | F1--F5: -174 dBm/Hz thermal-noise density, 6 dB noise figure, fallback 125 kHz bandwidth. F0: inherited sensitivity-as-background-noise rule. There is no explicit temperature parameter; -174 dBm/Hz is a configured density, not a recorded temperature measurement. | `src/LoRaPhy/LoRaAnalogModel.ned`, `.cc` |
| Reception and capture | SF-specific sensitivity and required-SNIR gates plus a cross-SF interference/capture matrix are implemented in `LoRaReceiver`. F2--F5 enable the required-SNIR gate; F0--F1 disable it. The full matrix and branch order should be read from source, not reduced to a single capture threshold. | `src/LoRaPhy/LoRaReceiver.h`, `.cc`; ablation INI |
| Detection caveat | Radio NED sets `receiver.energyDetection=-90dBm`, but the packaged `LoRaReceiver.cc::initialize()` does not load this NED parameter into its member. Thus -90 dBm is a declared configuration value, **not a verified effective receive-energy threshold** in the 900 runs. | `src/LoRa/LoRaRadio.ned`; `src/LoRa/LoRaGWRadio.ned`; `src/LoRaPhy/LoRaReceiver.cc`, `.h` |

## Energy and ADR implementation details

`simulations/energyConsumptionParameters.xml` parameterizes a generic ED radio
at 3.3 V. It does not identify a calibrated board. Receiving and busy states
draw 9.7 mA; idle and sleep draw 0.0001 mA. The transmit-current calibration
is (dBm:mA) `2:24, 3:24, 4:24, 5:25, 6:25, 7:25, 8:25, 9:26,
10:31, 11:32, 12:34, 13:35, 14:44`. F5 selects the nearest available
integer-dBm entry (ties to lower); F0--F4 retain `legacyExactWatts`.
`src/LoRaEnergyModules/LoRaEnergyConsumer.cc` integrates state power in W
times elapsed seconds, clips accounting to the post-warm-up window, and
records J by state and in total. Gateway and server energy are not part of
end-device energy.

`src/LoRa/NetworkServerApp.cc` holds at most 20 received samples and uses
the maximum quality value. It computes integer adaptation steps with a
C++ cast of margin / 2.5, which truncates toward zero, then reduces SF toward
7 before reducing transmit power in 3-dB steps. Power is bounded at 2--14 dBm;
negative steps raise power toward 14 dBm. The node declares
`ADR_ACK_LIMIT=64` and `ADR_ACK_DELAY=32` in
`src/LoRaApp/SimpleLoRaApp.h`; its recovery logic first raises power then SF
up to 12 after missed feedback. The server reads `frame->getSNIR()` and converts it from a linear ratio
to dB (`NetworkServerApp.cc:282`); the `bestSnr` name therefore denotes SNIR
in this implementation. It updates after each group of 20 eligible received
frames, or earlier if ADRACKReq is set; a no-change command is suppressed
unless a response is forced.

## Verification status

This document extracts values; it does not constitute an independent
numerical validation of formulas or a warm-up/replication precision study.
The delivery includes the existing MAC and integration-test scripts, original
outputs, historical MAC-regression build logs, and a fresh clean-build check.
Independent F1--F5 numerical sweeps are assigned to reviewer request 4; the
original campaign compiler stdout/stderr was not recovered. See `README.md`
and `RESPONSE_TO_REVIEWER_1.md` for the exact provenance limits.
