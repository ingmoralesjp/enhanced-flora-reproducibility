#!/usr/bin/env python3
"""Export supplementary, run-level mechanism scalars without conflating windows."""

import csv
import hashlib
import json
from collections import Counter, defaultdict
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[6]
HERE = Path(__file__).resolve().parent
BASE = HERE.parent / "platform-paper-run-level-v1"
CAMPAIGN = ROOT / "samples/flora/experiments/enhanced-flora-platform/results/fidelity-ablation-corrected-v0.2.0-dev"
PREFIX_NODE = "scalar LoRaNetworkTest.loRaNodes["
PREFIX_SERVER = "scalar LoRaNetworkTest.networkServer.app[0] "
PREFIX_GATEWAY = "scalar LoRaNetworkTest.loRaGW[0]."
VERSION = "platform-paper-mechanisms-v1"


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def integer(value):
    number = Decimal(str(value))
    assert number == number.to_integral_value(), value
    return int(number)


def rows(path):
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def scalar_audit(path, expected_nodes, expected_mean):
    counters = Counter()
    air = Decimal(0)
    node_means = Decimal(0)
    server_code = []
    gateway = {}
    with path.open(encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith(PREFIX_NODE):
                if ".SimpleLoRaApp clientAdrModeCode " in line:
                    counters["node_modes"] += 1
                    counters["node_mode_sum"] += integer(line.rsplit(" ", 1)[-1])
                elif ".LoRaNic.radio.transmitter " in line:
                    if " transmissionCount " in line:
                        counters["phy_uplinks"] += integer(line.rsplit(" ", 1)[-1])
                        counters["tx_node_count"] += 1
                    elif " totalTimeOnAir " in line:
                        air += Decimal(line.rsplit(" ", 1)[-1])
                        counters["air_node_count"] += 1
                    elif " meanTimeOnAir " in line:
                        node_means += Decimal(line.rsplit(" ", 1)[-1])
                        counters["mean_node_count"] += 1
            elif line.startswith(PREFIX_SERVER):
                if " adrAlgorithmCode " in line:
                    server_code.append(integer(line.rsplit(" ", 1)[-1]))
                elif ' "Send ADR for node" ' in line:
                    counters["server_commands_10d"] += integer(line.rsplit(" ", 1)[-1])
                elif ' "adaptiveNoOpSuppressed node ' in line:
                    counters["suppressed_noop_12d"] += integer(line.rsplit(" ", 1)[-1])
            elif line.startswith(PREFIX_GATEWAY):
                if ".packetForwarder forwardedDownlinks " in line:
                    gateway["forwarder_downlinks_12d"] = integer(line.rsplit(" ", 1)[-1])
                elif ".LoRaGWNic.radio.transmitter transmissionCount " in line:
                    gateway["phy_downlink_transmissions_12d"] = integer(line.rsplit(" ", 1)[-1])
                elif ".LoRaGWNic.radio.receiver " in line:
                    for name in ("numCollisionsBelowSens", "numCollisionsAboveSens", "Total_rcvBelowSensitivity"):
                        if f" {name} " in line:
                            gateway[name] = integer(line.rsplit(" ", 1)[-1])
    assert server_code == [1], (path, server_code)
    assert counters["node_modes"] == expected_nodes and counters["node_mode_sum"] == 0, path
    assert all(counters[key] == expected_nodes for key in ("tx_node_count", "air_node_count", "mean_node_count")), path
    assert counters["phy_uplinks"] > 0 and air > 0
    assert abs(node_means / expected_nodes - expected_mean) < Decimal("0.00000001"), path
    assert set(gateway) == {"forwarder_downlinks_12d", "phy_downlink_transmissions_12d", "numCollisionsBelowSens", "numCollisionsAboveSens", "Total_rcvBelowSensitivity"}, path
    return counters, air, gateway


def main():
    base_path = BASE / "enhanced-runs.csv"
    devices_path = BASE / "enhanced-device-summary.csv"
    source_path = CAMPAIGN / "analysis/run-metrics.csv"
    base_rows = rows(base_path)
    original_rows = {row["file"]: row for row in rows(source_path)}
    assert len(base_rows) == len(original_rows) == 900
    device_downlinks = Counter()
    with devices_path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            device_downlinks[row["run_id"]] += integer(row["ADR_commands_received"])
    assert len(device_downlinks) == 900
    result = []
    for idx, base in enumerate(base_rows, 1):
        source = original_rows[base["scalar_path"]]
        assert base["accepted"] == "true"
        assert base["stage"] == source["stage"]
        assert base["population_original"] == source["population"]
        assert integer(base["end_devices"]) == integer(source["nodes"])
        assert integer(base["repetition"]) == integer(source["repetition"])
        assert integer(base["seed"]) == integer(source["seed_set"])
        assert integer(base["unique_uplinks_sent"]) == integer(source["sent_data"])
        assert integer(base["unique_uplinks_received"]) == integer(source["received_data"])
        nodes = integer(base["end_devices"])
        metrics, total_air, gateway = scalar_audit(ROOT / base["scalar_path"], nodes, Decimal(source["mean_time_on_air_s"]))
        forwarded = integer(source["gateway_downlinks_forwarded"])
        rejected = integer(source["gateway_downlinks_dropped_dc"])
        attempts = forwarded + rejected
        assert gateway["forwarder_downlinks_12d"] == attempts, base["run_id"]
        assert gateway["phy_downlink_transmissions_12d"] == forwarded, base["run_id"]
        assert abs(Decimal(source["gateway_downlink_drop_rate"]) - (Decimal(rejected) / attempts if attempts else 0)) < Decimal("1e-12")
        sent = integer(source["sent_data"])
        for count_name, rate_name in (("below_sensitivity_losses", "below_sensitivity_rate"),
                                      ("low_snr_losses", "low_snr_loss_rate"),
                                      ("collision_losses", "collision_loss_rate")):
            assert abs(Decimal(source[rate_name]) - Decimal(integer(source[count_name])) / sent) < Decimal("1e-12")
        assert gateway["numCollisionsBelowSens"] + gateway["numCollisionsAboveSens"] == integer(source["collision_losses"])
        result.append({
            **{name: base[name] for name in ("run_id", "pair_id", "campaign", "source_hash", "source_manifest_hash",
                                              "config_hash", "included_config_hash", "population", "control_policy",
                                              "end_devices", "stage", "repetition", "seed", "seed_kind", "accepted",
                                              "failure_reason", "simulated_days", "warmup_days", "unique_uplinks_sent",
                                              "unique_uplinks_received")},
            "server_adr_algorithm_code": 1,
            "device_client_adr_mode_code": 0,
            "device_client_adr_mode_count": metrics["node_modes"],
            "paper_mean_of_node_airtime_means_s_12d": source["mean_time_on_air_s"],
            "phy_uplink_count_12d": metrics["phy_uplinks"],
            "phy_uplink_total_airtime_s_12d": str(total_air),
            "mean_airtime_per_phy_uplink_s_12d": str(total_air / metrics["phy_uplinks"]),
            "gateway_below_sensitivity_noncollision_10d": integer(source["below_sensitivity_losses"]),
            "gateway_low_snir_loss_10d": integer(source["low_snr_losses"]),
            "gateway_collision_loss_10d": integer(source["collision_losses"]),
            "gateway_collision_below_sensitivity_10d": gateway["numCollisionsBelowSens"],
            "gateway_collision_above_sensitivity_10d": gateway["numCollisionsAboveSens"],
            "gateway_total_below_sensitivity_10d": gateway["Total_rcvBelowSensitivity"],
            "gateway_other_phy_loss_scalar_inactive": integer(source["other_phy_losses"]),
            "paper_below_sensitivity_events_per_sent_10d": source["below_sensitivity_rate"],
            "paper_low_snir_events_per_sent_10d": source["low_snr_loss_rate"],
            "paper_collision_events_per_sent_10d": source["collision_loss_rate"],
            "server_adr_commands_issued_10d": metrics["server_commands_10d"],
            "device_downlinks_delivered_to_app_10d": device_downlinks[base["run_id"]],
            "server_adr_noop_suppressed_12d": metrics["suppressed_noop_12d"],
            "gateway_downlinks_from_server_12d": gateway["forwarder_downlinks_12d"],
            "gateway_downlinks_forwarded_to_radio_12d": forwarded,
            "gateway_phy_downlink_transmissions_12d": gateway["phy_downlink_transmissions_12d"],
            "gateway_downlinks_rejected_duty_12d": rejected,
            "gateway_downlink_rejection_fraction_12d": source["gateway_downlink_drop_rate"],
            "gateway_airtime_over_holdoff_ratio_12d": source["gateway_observed_duty_cycle"],
            "paper_mean_sf_changes_per_node": source["mean_sf_changes_per_node"],
            "paper_mean_tp_changes_per_node": source["mean_tp_changes_per_node"],
        })
        if idx % 100 == 0:
            print(f"validated {idx}/900 runs", flush=True)
    assert len(result) == 900
    path = HERE / "enhanced-mechanisms.csv"
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(result[0]))
        writer.writeheader()
        writer.writerows(result)
    inputs = {name: digest(path) for name, path in (("enhanced-runs.csv", base_path),
                                                  ("enhanced-device-summary.csv", devices_path),
                                                  ("run-metrics.csv", source_path),
                                                  ("campaign-manifest.json", CAMPAIGN / "state/campaign-manifest.json"))}
    manifest = {"version": VERSION, "rows": 900, "input_sha256": inputs,
                "output_sha256": {path.name: digest(path)},
                "exporter_sha256": digest(HERE / "export_mechanisms.py")}
    (HERE / "export-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"rows": len(result), "output_sha256": manifest["output_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
