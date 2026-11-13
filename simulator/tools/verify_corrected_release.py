#!/usr/bin/env python3
"""Verify a corrected Enhanced FLoRa 0.2.0-dev distribution without raw data."""

import argparse
import csv
import hashlib
import json
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def rows(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def check(root):
    manifest = root / "MANIFEST.sha256"
    if not manifest.is_file():
        raise ValueError("MANIFEST.sha256 missing")
    expected = {}
    for line in manifest.read_text(encoding="utf-8").splitlines():
        digest, name = line.split("  ", 1)
        if name in expected:
            raise ValueError(f"Duplicate manifest entry: {name}")
        expected[name] = digest
    actual = {
        path.relative_to(root).as_posix(): sha256(path)
        for path in root.rglob("*")
        if path.is_file() and path != manifest
    }
    if actual != expected:
        changed = sorted(
            set(actual) ^ set(expected)
            | {name for name in set(actual) & set(expected)
               if actual[name] != expected[name]}
        )
        raise ValueError(f"Archive manifest mismatch: {changed[:20]}")
    if (root / "ENHANCED-FLORA-VERSION").read_text().strip() != "0.2.0-dev":
        raise ValueError("Wrong corrected release version")
    if any("fidelity-ablation-20260909" in name for name in actual):
        raise ValueError("Historical 0.1.0-dev results leaked into corrected package")
    forbidden = (".so", ".o", ".d", ".sca", ".log", ".orig", ".old", ".rej")
    if any(name.endswith(forbidden) for name in actual):
        raise ValueError("Compiled, raw, log, or backup file included")

    data = (root / "experiments/enhanced-flora-platform/results"
            / "fidelity-ablation-corrected-v0.2.0-dev")
    bound = json.loads((data / "state/campaign-manifest.json").read_text())
    source_map = {
        "LoRaMac.cc": "src/LoRa/LoRaMac.cc",
        "LoRaRadio.cc": "src/LoRa/LoRaRadio.cc",
        "LoRaRadio.h": "src/LoRa/LoRaRadio.h",
        "fidelity-ablation.ini": "simulations/examples/fidelity-ablation.ini",
        "Analisis-fidelity-ablation.ini":
            "simulations/examples/Analisis-fidelity-ablation.ini",
    }
    for name, relative in source_map.items():
        if sha256(root / relative) != bound["source_sha256"][name]:
            raise ValueError(f"Packaged source differs from corrected campaign: {name}")
    ini_dir = root / "simulations/examples"
    for name in ("fidelity-ablation.ini", "Analisis-fidelity-ablation.ini",
                 "Analisis-mi-PROPOSAL-ADR.ini"):
        ini = ini_dir / name
        if not ini.is_file():
            raise ValueError(f"Missing required simulation INI: {name}")
        for line in ini.read_text(encoding="utf-8").splitlines():
            if line.strip().startswith("include "):
                target = line.strip().split(None, 1)[1].strip('"')
                if not (ini_dir / target).is_file():
                    raise ValueError(f"Missing included simulation INI: {target}")
    regression = json.loads((root / "tests/evidence/mac-one-send-rc4.json").read_text())
    patch = root / "tests/0001-assert-one-mac-send-per-event.patch"
    if (regression["status"] != "PASS"
            or len(regression["configurations"]) != 10
            or regression["source_mac_sha256"] != bound["source_sha256"]["LoRaMac.cc"]
            or regression["assertion_patch_sha256"] != sha256(patch)):
        raise ValueError("MAC diagnostic receipt is not bound to this source and patch")
    status = json.loads((data / "state/status.json").read_text())
    if status["pass"] != 900 or status["fail"] or status["total"] != 900:
        raise ValueError("Incorrect corrected campaign status")
    if len(list((data / "state/accepted").glob("*.ok"))) != 900:
        raise ValueError("Incorrect corrected receipt count")
    metrics = rows(data / "analysis/run-metrics.csv")
    if len(metrics) != 900 or len({
            (r["stage"], r["population"], int(r["nodes"]), int(r["repetition"]))
            for r in metrics}) != 900:
        raise ValueError("Corrected run-level matrix is incomplete or duplicated")
    for name, expected_count in (
        ("checkpoint-metrics.csv", 900),
        ("summary-ci95.csv", 180),
        ("paired-contrasts-ci95.csv", 300),
    ):
        if len(rows(data / "analysis" / name)) != expected_count:
            raise ValueError(f"Incorrect corrected data dimension: {name}")

    raw_reference = root / "evidence/raw-corrected"
    raw = json.loads((raw_reference / "raw-data-manifest.json").read_text())
    raw_files = raw["files"]
    raw_paths = [entry["path"] for entry in raw_files]
    if (raw["accepted_runs"], raw["scalar_files"], raw["completion_logs"],
            len(raw_files), len(set(raw_paths))) != (900, 900, 900, 2702, 2702):
        raise ValueError("Incorrect corrected raw-data inventory")
    if raw["campaign_manifest_sha256"] != sha256(data / "state/campaign-manifest.json"):
        raise ValueError("Raw-data inventory belongs to another campaign")
    if (sum(name.startswith("scalars/") for name in raw_paths) != 900
            or sum(name.startswith("logs/") for name in raw_paths) != 900
            or sum(name.startswith("state/") for name in raw_paths) != 902
            or any(Path(name).is_absolute() or ".." in Path(name).parts
                   for name in raw_paths)):
        raise ValueError("Raw-data paths are incomplete or unsafe")
    raw_name = "enhanced-flora-corrected-0.2.0-dev-accepted-raw.tar.gz"
    checksum = (raw_reference / f"{raw_name}.sha256").read_text().strip()
    digest, filename = checksum.split("  ", 1)
    if (len(digest) != 64 or any(char not in "0123456789abcdef" for char in digest)
            or filename != raw_name):
        raise ValueError("Invalid raw-data archive checksum record")

    reference = root / "evidence/official-flora-1.1"
    survey = json.loads((reference / "survey-status.json").read_text())
    compact = json.loads((reference / "compact-provenance-n1000.json").read_text())
    if (survey["accepted"], survey["failed"], survey["pending"]) != (92, 8, 0):
        raise ValueError("Incorrect official survey coverage")
    if (sha256(reference / "campaign-manifest.json")
            != compact["official_manifest_sha256"]
            or survey["manifest_sha256"] != compact["official_manifest_sha256"]):
        raise ValueError("Official compact data and sealed manifest differ")
    for name, key in (
        ("official-run-level-n1000.csv", "official_run_level_sha256"),
        ("official-failures-n1000.csv", "official_failure_map_sha256"),
        ("coverage-through-n1000.csv", "coverage_sha256"),
    ):
        if sha256(reference / name) != compact[key]:
            raise ValueError(f"Official compact data changed: {name}")
    valid = rows(reference / "official-run-level-n1000.csv")
    failed = rows(reference / "official-failures-n1000.csv")
    coverage = rows(reference / "coverage-through-n1000.csv")
    if len(valid) != 92 or len(failed) != 8 or len(coverage) != 10:
        raise ValueError("Official compact data has incorrect dimensions")
    if "pdr" in failed[0]:
        raise ValueError("Failed official runs must not have a PDR column")
    for nodes in range(100, 1001, 100):
        ok = {int(r["repetition"]) for r in valid if int(r["nodes"]) == nodes}
        bad = {int(r["repetition"]) for r in failed if int(r["nodes"]) == nodes}
        if ok & bad or ok | bad != set(range(10)):
            raise ValueError(f"Official receipt map incomplete at {nodes} nodes")
    paper = root / "docs/enhanced-flora-platform-paper"
    context_rows = rows(paper / "official-reference/cross-stack-f0-f5.csv")
    if len(context_rows) != 10:
        raise ValueError("Cross-stack context must contain ten densities")
    covered = {int(row["nodes"]): row for row in coverage}
    for row in context_rows:
        nodes = int(row["nodes"])
        if (nodes not in covered
                or int(row["official_valid"]) != int(covered[nodes]["official_accepted"])
                or int(row["official_failed"]) != int(covered[nodes]["official_failed"])
                or abs(float(row["official_pdr_pct"]) -
                       float(covered[nodes]["official_pdr_pct"])) > 1e-10):
            raise ValueError(f"Official context differs at {nodes} devices")
        for stage, prefix in (("F0", "f0"), ("F5", "f5")):
            subset = [item for item in metrics if item["stage"] == stage
                      and item["population"] == "static"
                      and int(item["nodes"]) == nodes]
            if (len(subset) != 5
                    or {int(item["repetition"]) for item in subset} != set(range(5))
                    or int(row[f"{prefix}_valid"]) != 5
                    or abs(float(row[f"{prefix}_pdr_pct"]) -
                           sum(100 * float(item["pdr"]) for item in subset) / 5) > 1e-10):
                raise ValueError(f"{stage} context differs at {nodes} devices")
    for name in (
        "main.tex", "main.pdf", "official-reference/coverage-through-n1000.tex",
        "official-reference/pdr-through-n1000.pdf",
        "official-reference/cross-stack-f0-f5.tex",
        "official-reference/cross-stack-f0-f5.pdf",
        "sections/02b_official_baseline.tex",
        "corrected-v0.2.0-dev/generated/complete_path_table.tex",
    ):
        if not (paper / name).is_file():
            raise ValueError(f"Missing paper artifact: {name}")
    return {"status": "PASS", "files": len(expected), "corrected_runs": 900,
            "official_accepted": 92, "official_failed": 8, "raw_files": 2702}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release", type=Path)
    args = parser.parse_args()
    print(json.dumps(check(args.release.resolve()), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
