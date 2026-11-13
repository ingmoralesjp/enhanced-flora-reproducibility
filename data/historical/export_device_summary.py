#!/usr/bin/env python3
"""Stream node summaries from accepted Enhanced scalar files; no event trajectory."""

import csv
import hashlib
import json
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[6]
HERE = Path(__file__).resolve().parent
METRICS = {
    "measurementEnergyConsumed": "energy_mJ",
    "initialLoRaSF": "initial_SF",
    "finalSF": "final_SF",
    "initialLoRaTP": "initial_tx_power_dBm",
    "finalTP": "final_tx_power_dBm",
    "SFChanges": "SF_changes",
    "TPChanges": "tx_power_changes",
    "receivedADRCommands": "ADR_commands_received",
}
FIELDS = ["run_id", "device_id", *METRICS.values()]
PREFIX = "scalar LoRaNetworkTest.loRaNodes["


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def extract(path, expected_nodes, expected_energy_mj):
    devices = {}
    with path.open(encoding="utf-8", errors="replace") as f:
        for line in f:
            if not line.startswith(PREFIX):
                continue
            parts = line.split()
            if len(parts) != 4 or parts[0] != "scalar":
                continue
            module, metric, value = parts[1:]
            if metric not in METRICS:
                continue
            node_part, suffix = module[len("LoRaNetworkTest.loRaNodes["):].split("]", 1)
            if suffix not in (".SimpleLoRaApp", ".LoRaNic.radio.energyConsumer"):
                continue
            node = int(node_part)
            assert 0 <= node < expected_nodes
            record = devices.setdefault(node, {})
            output_field = METRICS[metric]
            assert output_field not in record, (path, node, metric)
            record[output_field] = value
    assert len(devices) == expected_nodes, (path, len(devices), expected_nodes)
    total = Decimal(0)
    rows = []
    for node in range(expected_nodes):
        record = devices[node]
        assert set(record) == set(METRICS.values()), (path, node, record.keys())
        energy_mj = Decimal(record["energy_mJ"]) * 1000
        total += energy_mj
        record["energy_mJ"] = str(energy_mj)
        for field in ("initial_SF", "final_SF"):
            assert 7 <= int(float(record[field])) <= 12, (path, node, field)
        rows.append((node, record))
    assert abs(total - expected_energy_mj) < Decimal("0.0001"), (path, total, expected_energy_mj)
    return rows


def main():
    source = HERE / "enhanced-runs.csv"
    with source.open(newline="", encoding="utf-8") as f:
        runs = list(csv.DictReader(f))
    assert len(runs) == 900 and all(r["accepted"] == "true" for r in runs)
    destination = HERE / "enhanced-device-summary.csv"
    partial = HERE / "enhanced-device-summary.csv.partial"
    count = 0
    with partial.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDS)
        writer.writeheader()
        for index, run in enumerate(runs, 1):
            scalar = ROOT / run["scalar_path"]
            devices = extract(scalar, int(run["end_devices"]), Decimal(run["end_device_energy_mJ"]))
            for node, values in devices:
                writer.writerow({"run_id": run["run_id"], "device_id": node, **values})
                count += 1
            if index % 50 == 0:
                print(f"validated {index}/900 runs; {count} devices", flush=True)
    assert count == 6 * 3 * 5 * sum(range(100, 1001, 100)) == 495000
    partial.replace(destination)
    manifest_path = HERE / "export-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    manifest["enhanced"]["device_rows"] = count
    manifest["outputs"][destination.name] = {"sha256": sha256(destination), "bytes": destination.stat().st_size}
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"validated {count} device rows; {destination}")


if __name__ == "__main__":
    main()
