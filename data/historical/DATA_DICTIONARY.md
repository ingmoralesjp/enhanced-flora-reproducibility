# Platform-paper run-level data, version 1

This export belongs to *Enhanced FLoRa: An Audited Fidelity and Reproducibility Extension for LoRaWAN Simulation*. It contains existing results only; it does not launch simulations. Run `python3 export_paper_data.py` to reproduce the two run-level tables. Run `python3 export_device_summary.py` only if the optional per-device CSV needs to be regenerated from the much larger scalar archive.

## Files and coverage

| File | Unit of observation | Coverage |
| --- | --- | --- |
| `enhanced-runs.csv` | one Enhanced run | 900/900 accepted: F0–F5 × three populations × 100–1000 devices in steps of 100 × five repetitions |
| `enhanced-downlink-counters.csv` | one Enhanced run | same 900 runs; gateway MAC counters only |
| `enhanced-device-summary.csv` | one end device in one accepted Enhanced run | 495,000 devices; scalar summaries, **not** time-stamped trajectories |
| `official-flora-1.1-runs.csv` | one official reference attempt | 100 attempts: 92 accepted, eight failed; stationary only, ten repetitions per density |
| `export-manifest.json` | export metadata | input campaign-manifest hashes and SHA-256 of generated CSVs |

The Enhanced pairs are formed by the same `population`, `end_devices`, and `repetition` across F0–F5; `pair_id` records this tuple. There are 150 complete six-stage pairs and 180 stage/population/density cells of five runs each. `seed` is the OMNeT++ **seed-set index**, not a raw random-number-generator seed and not the sole pairing identifier. `run_id` is unique within this export. `accepted=true` means that the campaign marked a complete scalar run as accepted and the matching receipt was found; the exporter checks each receipt's configuration, run number, and scalar filename. It also checks the recorded 900/900 status, design grid, positive count/energy values, and recomputed PDR. The official 92/8 classification comes from the separate accepted and failure receipts and its survey status. This export does not independently re-run either simulator.

## Main Enhanced columns

| Column | Definition |
| --- | --- |
| `run_id`, `pair_id`, `campaign` | unique run key, six-stage pairing key, and fixed campaign identifier |
| `source_hash` | SHA-256 of the **executed `libflora.so` binary** recorded in the frozen campaign manifest; it is not claimed to hash the entire source tree |
| `source_manifest_hash` | SHA-256 of `state/campaign-manifest.json`; that manifest records the source-file, runner, and binary hashes |
| `config_hash`, `included_config_hash` | recorded SHA-256 values of the entry `fidelity-ablation.ini` and included `Analisis-fidelity-ablation.ini`, respectively; use both plus the manifest to identify the executed configuration |
| `population` | `stationary`, `random_waypoint`, or `mixed`; `population_original` retains the source labels `static`, `mobile`, `mixed` |
| `control_policy` | `lorawanReference` for every F0–F5 run; this is the configured modified legacy/reference ADR, not BADR or a mixed-policy comparison |
| `end_devices`, `end_device_count` | configured number of end devices, duplicated under the requested count field |
| `stage`, `repetition`, `seed`, `seed_kind` | cumulative fidelity stage, within-density repetition (0–4), seed-set index, and its interpretation |
| `accepted`, `failure_reason` | all 900 Enhanced runs are accepted; failure reason is empty, never an imputed metric |
| `simulated_days`, `warmup_days` | 12-day configured horizon and two-day measurement warm-up |
| `unique_uplinks_sent`, `unique_uplinks_received` | source summary's `sent_data` and `received_data`: network-server `counterOfSentPacketsFromNodes` and `counterUniqueReceivedPackets` in the aligned measurement window |
| `end_device_energy_mJ` | sum of end-device `measurementEnergyConsumed` scalar values, converted from J to mJ; it is measurement-window energy, not gateway/server energy |
| `pdr_recomputed` | unique received / unique sent, calculated from the exported counts |
| `energy_per_delivery_mJ_recomputed` | total end-device mJ / unique received; this is a cost per successful delivery, not mean per-node energy |
| `scalar_path`, `receipt_path` | paths relative to `/home/juan/omnetpp-6.0pre11` for audit, not portable file URIs |

F0 is an **internal Enhanced-source control**, not the unmodified official FLoRa 1.1. All six stages share the corrected MAC, application, measurement code, and ADR policy. F1 adds thermal-noise calculation; F2 adds SF-specific packet-SNIR reception requirements; F3 uses actual physical-frame bytes in time-on-air; F4 applies downlink-airtime-based 1% gateway duty holdoff; F5 corrects the transmit-power/current lookup. The treatments are cumulative. These labels follow the paper's stage map.

## Downlink counters

`enhanced-downlink-counters.csv` joins to the main table on `run_id`. `gateway_forwarded_to_phy` is the measured gateway MAC `GW_forwardedDown` count; `gateway_rejected_duty` is measured `GW_droppedDC`. `gateway_attempts_derived` is their arithmetic sum. Forwarded means passed toward the radio path; it does **not** establish successful RF transmission or reception. The campaign does not provide trustworthy separate `downlinks_scheduled`, `downlinks_transmitted`, or `downlink_received` run-level fields, so those names are intentionally absent. These counters support a descriptive, not causal-by-themselves, interpretation of F4.

## Per-device summary

`enhanced-device-summary.csv` joins on `run_id` and identifies each node by zero-based `device_id`. `energy_mJ` is that device's measured energy, converted from J; its sum is checked against the run-level total. `initial_SF`, `final_SF`, `initial_tx_power_dBm`, `final_tx_power_dBm`, `SF_changes`, `tx_power_changes`, and `ADR_commands_received` are scalar summaries. The scalar campaign does not provide the requested timestamp/SNR/SNIR/ADR-event sequence, so this file **cannot** establish adaptation trajectories or the timing of a downlink.

## Separate official FLoRa 1.1 reference

`official-flora-1.1-runs.csv` is **not** part of the 900 Enhanced runs or their 150 pairs. It uses OMNeT++ 6.0, INET 4.4, and official FLoRa 1.1. Its `source_hash` is the recorded executed official `libflora.so` SHA-256, `source_manifest_hash` identifies the official campaign manifest, and `config_hash` is the recorded `experiment/official-legacy.ini` SHA-256. The official table records all 100 attempts, with `accepted=false` and the preserved `failure_reason` for eight failures. Counts, PDR, energy, and scalar path remain **blank** for failed attempts. `native_end_device_energy_mJ` is the native official energy sum converted from J, **not** a harmonized counterpart to Enhanced measurement-window energy. Cross-stack differences in propagation, pacing, MAC behavior, reception, duty cycle, energy accounting, and simulator versions preclude treating the official comparison as a paired or single-cause ADR estimate. Accepted-only official summaries are conditional on completion.

Both campaigns use a single gateway. This export does not add device duty-cycle experiments or urban simulations.
