#!/usr/bin/env python3
"""Export the frozen platform-paper campaigns without imputing failed runs."""

import csv
import hashlib
import json
from collections import Counter, defaultdict
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[6]
HERE = Path(__file__).resolve().parent
ENH = ROOT / "samples/flora/experiments/enhanced-flora-platform/results/fidelity-ablation-corrected-v0.2.0-dev"
OFF = ROOT / "samples/flora/experiments/adr-m1-m2-replication/testbeds/flora-1.1-official-legacy"
OFF_RESULTS = OFF / "results/official-matched-static"
POPULATIONS = {"static": "stationary", "mobile": "random_waypoint", "mixed": "mixed"}
STAGES = tuple(f"F{i}" for i in range(6))


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)


def as_int(value):
    d = Decimal(str(value))
    if d != d.to_integral_value():
        raise ValueError(f"nonintegral count: {value}")
    return int(d)


def export_enhanced():
    manifest_path = ENH / "state/campaign-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    hashes = manifest["source_sha256"]
    status = json.loads((ENH / "state/status.json").read_text())
    assert status["pass"] == status["total"] == 900 and status["fail"] == 0
    assert manifest["design"]["sim_time_limit_seconds"] == 12 * 86400
    accepted = list((ENH / "state/accepted").glob("*.ok"))
    assert len(accepted) == 900
    assert len(list((ENH / "scalars").glob("*.sca"))) == 900
    with (ENH / "analysis/run-metrics.csv").open(newline="", encoding="utf-8") as f:
        raw = list(csv.DictReader(f))
    assert len(raw) == 900
    rows, downlinks = [], []
    cells = Counter()
    pair_stages = defaultdict(set)
    pair_seeds = defaultdict(set)
    seen = set()
    for src in raw:
        stage = src["stage"]
        population = POPULATIONS[src["population"]]
        nodes = as_int(src["nodes"])
        repetition = as_int(src["repetition"])
        seed = as_int(src["seed_set"])
        run_number = as_int(src["run_number"])
        sent = as_int(src["sent_data"])
        received = as_int(src["received_data"])
        energy = Decimal(src["measurement_energy_j"])
        assert stage in STAGES and nodes in range(100, 1001, 100)
        assert 0 <= repetition < 5 and run_number == (nodes // 100 - 1) * 5 + repetition
        assert seed == run_number and 0 <= received <= sent and sent > 0 and energy > 0
        assert abs(Decimal(src["pdr"]) - Decimal(received) / Decimal(sent)) < Decimal("1e-12")
        relative_scalar = Path(src["file"])
        assert (ROOT / relative_scalar).is_file()
        receipt = ENH / "state/accepted" / (relative_scalar.stem + ".ok")
        assert receipt.is_file(), receipt
        receipt_content = receipt.read_text()
        assert f"configuration={src['configuration']}\n" in receipt_content
        assert f"run={run_number}\n" in receipt_content
        assert f"scalar={relative_scalar.name}\n" in receipt_content
        key = (stage, population, nodes, repetition)
        assert key not in seen
        seen.add(key)
        cells[(stage, population, nodes)] += 1
        pair_id = f"{population}-n{nodes}-r{repetition}"
        pair_stages[pair_id].add(stage)
        pair_seeds[pair_id].add(seed)
        run_id = f"enhanced-{stage.lower()}-{pair_id}"
        energy_mj = energy * 1000
        rows.append({
            "run_id": run_id,
            "pair_id": pair_id,
            "campaign": "enhanced-fidelity-ablation-corrected-v0.2.0-dev",
            "source_hash": hashes["libflora.so"],
            "source_manifest_hash": sha256(manifest_path),
            "config_hash": hashes["fidelity-ablation.ini"],
            "included_config_hash": hashes["Analisis-fidelity-ablation.ini"],
            "population": population,
            "population_original": src["population"],
            "control_policy": "lorawanReference",
            "end_devices": nodes,
            "stage": stage,
            "repetition": repetition,
            "seed": seed,
            "seed_kind": "OMNeT++ seed-set index",
            "accepted": "true",
            "failure_reason": "",
            "simulated_days": 12,
            "warmup_days": 2,
            "unique_uplinks_sent": sent,
            "unique_uplinks_received": received,
            "end_device_energy_mJ": str(energy_mj),
            "end_device_count": nodes,
            "pdr_recomputed": str(Decimal(received) / Decimal(sent)),
            "energy_per_delivery_mJ_recomputed": str(energy_mj / received) if received else "",
            "scalar_path": relative_scalar.as_posix(),
            "receipt_path": receipt.relative_to(ROOT).as_posix(),
        })
        forwarded = as_int(src["gateway_downlinks_forwarded"])
        rejected = as_int(src["gateway_downlinks_dropped_dc"])
        downlinks.append({
            "run_id": run_id,
            "stage": stage,
            "gateway_forwarded_to_phy": forwarded,
            "gateway_rejected_duty": rejected,
            "gateway_attempts_derived": forwarded + rejected,
        })
    assert len(cells) == 180 and set(cells.values()) == {5}
    assert len(pair_stages) == 150 and all(stages == set(STAGES) for stages in pair_stages.values())
    assert all(len(seeds) == 1 for seeds in pair_seeds.values())
    assert len(seen) == 900
    rows.sort(key=lambda r: (r["population"], int(r["end_devices"]), int(r["repetition"]), r["stage"]))
    downlinks.sort(key=lambda r: (r["run_id"]))
    write_csv(HERE / "enhanced-runs.csv", list(rows[0]), rows)
    write_csv(HERE / "enhanced-downlink-counters.csv", list(downlinks[0]), downlinks)
    return {"rows": len(rows), "pairs": len(pair_stages), "cells": len(cells), "manifest_sha256": sha256(manifest_path)}


def export_official():
    manifest_path = OFF / "provenance/campaign-manifest.json"
    manifest_hash = sha256(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    status = json.loads((OFF_RESULTS / "survey/status.json").read_text())
    assert status["accepted"] == 92 and status["failed"] == 8 and status["expected"] == 100
    assert status["pending"] == 0 and status["manifest_sha256"] == manifest_hash
    accepted_files = sorted((OFF_RESULTS / "campaign/accepted").glob("*.json"))
    failed_files = sorted((OFF_RESULTS / "survey/failed").glob("*.json"))
    assert len(accepted_files) == 92 and len(failed_files) == 8
    rows = []
    seen = set()
    for accepted_flag, files in ((True, accepted_files), (False, failed_files)):
        for path in files:
            record = json.loads(path.read_text())
            data = record["row"] if accepted_flag else record
            nodes = as_int(data["nodes"])
            repetition = as_int(data["repetition"])
            seed = as_int(data["seedset"])
            assert nodes in range(100, 1001, 100) and 0 <= repetition < 10
            assert seed == (nodes // 100 - 1) * 10 + repetition
            assert (nodes, repetition) not in seen
            seen.add((nodes, repetition))
            assert (record["manifest"] if accepted_flag else record["campaign_manifest_sha256"]) == manifest_hash
            sent = as_int(data["sent"]) if accepted_flag else ""
            received = as_int(data["received"]) if accepted_flag else ""
            assert not accepted_flag or 0 <= received <= sent
            if accepted_flag:
                assert sent > 0
                assert abs(Decimal(str(data["pdr"])) - Decimal(received) / Decimal(sent)) < Decimal("1e-12")
                assert Path(record["scalar"]).is_file()
            rows.append({
                "run_id": f"official-flora-1.1-stationary-n{nodes}-r{repetition}",
                "campaign": "official-flora-1.1-matched-static",
                "source_hash": manifest["hashes"]["work/flora/src/libflora.so"],
                "source_manifest_hash": manifest_hash,
                "config_hash": manifest["hashes"]["experiment/official-legacy.ini"],
                "stack": "OMNeT++ 6.0 / INET 4.4 / FLoRa 1.1",
                "population": "stationary",
                "control_policy": "official-flora-1.1-legacy",
                "end_devices": nodes,
                "repetition": repetition,
                "seed": seed,
                "seed_kind": "OMNeT++ seed-set index",
                "accepted": str(accepted_flag).lower(),
                "failure_reason": "" if accepted_flag else record["error"],
                "unique_uplinks_sent": sent,
                "unique_uplinks_received": received,
                "native_end_device_energy_mJ": str(Decimal(str(data["native_total_energy_j"])) * 1000) if accepted_flag else "",
                "pdr_recomputed": str(Decimal(received) / Decimal(sent)) if accepted_flag and sent else "",
                "receipt_path": path.relative_to(ROOT).as_posix(),
                "scalar_path": Path(record["scalar"]).relative_to(ROOT).as_posix() if accepted_flag else "",
            })
    assert len(seen) == 100 and len(rows) == 100
    rows.sort(key=lambda r: (int(r["end_devices"]), int(r["repetition"])))
    write_csv(HERE / "official-flora-1.1-runs.csv", list(rows[0]), rows)
    return {"rows": len(rows), "accepted": 92, "failed": 8, "manifest_sha256": manifest_hash}


def main():
    enhanced = export_enhanced()
    official = export_official()
    outputs = {}
    for name in ("enhanced-runs.csv", "enhanced-downlink-counters.csv", "official-flora-1.1-runs.csv"):
        path = HERE / name
        outputs[name] = {"sha256": sha256(path), "bytes": path.stat().st_size}
    device_path = HERE / "enhanced-device-summary.csv"
    if device_path.is_file():
        with device_path.open(newline="", encoding="utf-8") as f:
            device_rows = sum(1 for _ in f) - 1
        assert device_rows == 495000
        enhanced["device_rows"] = device_rows
        outputs[device_path.name] = {"sha256": sha256(device_path), "bytes": device_path.stat().st_size}
    scripts = {}
    for name in ("export_paper_data.py", "export_device_summary.py", "DATA_DICTIONARY.md"):
        scripts[name] = sha256(HERE / name)
    (HERE / "export-manifest.json").write_text(json.dumps({
        "enhanced": enhanced,
        "official": official,
        "outputs": outputs,
        "export_files_sha256": scripts,
    }, indent=2) + "\n")
    print(json.dumps({"enhanced": enhanced, "official": official, "outputs": outputs}, indent=2))


if __name__ == "__main__":
    main()
