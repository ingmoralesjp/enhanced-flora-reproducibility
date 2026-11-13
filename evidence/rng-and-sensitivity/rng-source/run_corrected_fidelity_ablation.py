#!/usr/bin/env python3
"""Resumable corrected-source runner for the Enhanced FLoRa ablation."""

import argparse
import datetime as dt
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
FLORA = PROJECT.parents[1]
OMNET = FLORA.parents[1]
SAMPLES = FLORA.parent
EXAMPLES = FLORA / "simulations" / "examples"
ROOT = PROJECT / "results" / "fidelity-ablation-corrected-v0.2.0-dev"
SCALARS, LOGS, STATE = ROOT / "scalars", ROOT / "logs", ROOT / "state"
RECOVERY = ROOT / "recovery"
INI = EXAMPLES / "fidelity-ablation.ini"
SIM_TIME_LIMIT_SECONDS = 12 * 24 * 60 * 60
NODE_COUNTS = tuple(range(100, 1001, 100))
LOCK = threading.Lock()

STAGES = {
    "F0": ("Legacy", (0, 0, 0, 0, 0), "f0-legacy"),
    "F1": ("ThermalNoise", (1, 0, 0, 0, 0), "f1-noise"),
    "F2": ("SnrReception", (1, 1, 0, 0, 0), "f2-reception"),
    "F3": ("SemtechToa", (1, 1, 1, 0, 0), "f3-toa"),
    "F4": ("ToaDutyCycle", (1, 1, 1, 1, 0), "f4-duty"),
    "F5": ("Enhanced", (1, 1, 1, 1, 1), "f5-enhanced"),
}
POPULATIONS = ("Static", "Mobile", "Mixed")
MATRIX = tuple(
    f"Fidelity{stage}{label}{population}"
    for stage, (label, _, _) in STAGES.items()
    for population in POPULATIONS
)
CONFIG_META = {
    f"Fidelity{stage}{label}{population}": (stage, population.lower(), f"{stem}-{population.lower()}")
    for stage, (label, _, stem) in STAGES.items()
    for population in POPULATIONS
}

def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def source_identity():
    paths = {
        "LoRaMac.cc": FLORA / "src" / "LoRa" / "LoRaMac.cc",
        "LoRaRadio.cc": FLORA / "src" / "LoRa" / "LoRaRadio.cc",
        "LoRaRadio.h": FLORA / "src" / "LoRa" / "LoRaRadio.h",
        "libflora.so": FLORA / "src" / "libflora.so",
        "fidelity-ablation.ini": INI,
        "Analisis-fidelity-ablation.ini": EXAMPLES / "Analisis-fidelity-ablation.ini",
    }
    return {name: sha256(path) for name, path in paths.items()}


def bind_campaign():
    STATE.mkdir(parents=True, exist_ok=True)
    manifest = STATE / "campaign-manifest.json"
    current = source_identity()
    if manifest.exists():
        recorded = json.loads(manifest.read_text(encoding="utf-8"))
        if recorded["source_sha256"] != current:
            raise RuntimeError("source identity differs from the bound campaign manifest")
        return recorded
    payload = {
        "campaign": "Enhanced FLoRa corrected MAC-reentry fidelity ablation",
        "created_at": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "design": {
            "stages": list(STAGES),
            "populations": [p.lower() for p in POPULATIONS],
            "nodes": list(NODE_COUNTS),
            "repetitions": 5,
            "sim_time_limit_seconds": SIM_TIME_LIMIT_SECONDS,
        },
        "source_sha256": current,
        "runner_sha256": sha256(Path(__file__).resolve()),
    }
    temporary = manifest.with_suffix(".tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(manifest)
    return payload



def base_command():
    return [
        str(OMNET / "bin" / "opp_run"),
        "-n", f"{FLORA/'src'}:{FLORA/'simulations'}:{SAMPLES/'inet'/'src'}",
        "-l", str(FLORA / "src" / "flora"),
        "-l", str(SAMPLES / "inet" / "src" / "INET"),
    ]


def identity(run):
    return NODE_COUNTS[run // 5], run % 5


def run_number(nodes, repetition):
    return NODE_COUNTS.index(nodes) * 5 + repetition


def result_path(config, run):
    nodes, repetition = identity(run)
    return SCALARS / f"{CONFIG_META[config][2]}-n{nodes}-r{repetition}.sca"


def log_path(config, run):
    return LOGS / f"{config}-run{run}.log"


def acceptance_marker(config, run):
    nodes, repetition = identity(run)
    return STATE / "accepted" / f"{CONFIG_META[config][2]}-n{nodes}-r{repetition}.ok"


def log_reached_limit(config, run, candidate=None):
    marker = f"Simulation time limit reached -- at t={SIM_TIME_LIMIT_SECONDS}s"
    try:
        source = candidate if candidate is not None else log_path(config, run)
        return marker in source.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False


def scalar_is_valid(path, config, run, candidate_log=None):
    if not path.exists() or not log_reached_limit(config, run, candidate_log):
        return False
    stage, population, _ = CONFIG_META[config]
    expected_codes = STAGES[stage][1]
    nodes, repetition = identity(run)
    found = {
        "configname": None, "runnumber": None, "repetition": None,
        "seedset": None, "nodes": None, "algorithm": set(), "step": set(),
        "noise": set(), "snr": set(), "toa": set(), "duty": set(),
        "current": set(),
    }
    mobile = []
    channels = []
    scalar_markers = {
        "adrAlgorithmCode": "algorithm",
        "adrStepSizeDb": "step",
        "backgroundNoiseModelCode": "noise",
        "requiredSnrGateEnabled": "snr",
        "airtimeModelCode": "toa",
        "GW_dutyCycleModelCode": "duty",
        "txCurrentLookupModelCode": "current",
    }
    try:
        with path.open(encoding="utf-8", errors="replace") as source:
            for raw in source:
                line = raw.rstrip("\n")
                for attribute in ("configname", "runnumber", "repetition", "seedset"):
                    if line.startswith(f"attr {attribute} "):
                        found[attribute] = line.split(maxsplit=2)[2]
                if line.startswith("par LoRaNetworkTest numberOfNodes "):
                    found["nodes"] = int(line.rsplit(maxsplit=1)[1])
                elif line.startswith("par "):
                    parts = line.split(maxsplit=3)
                    if len(parts) == 4:
                        module, name, value = parts[1:]
                        if name == "mobile" and ".loRaNodes[" in module:
                            mobile.append(value)
                        elif name == "useAllEu868Channels" and module.endswith(".SimpleLoRaApp"):
                            channels.append(value)
                elif line.startswith("scalar "):
                    for scalar_name, key in scalar_markers.items():
                        if f" {scalar_name} " in line:
                            found[key].add(float(line.rsplit(maxsplit=1)[1]))
    except (OSError, ValueError):
        return False

    expected_mobile = 0 if population == "static" else nodes if population == "mobile" else nodes // 2
    expected_static = nodes - expected_mobile
    noise, snr, toa, duty, current = expected_codes
    return all((
        found["configname"] == config,
        found["runnumber"] == str(run),
        found["repetition"] == str(repetition),
        # OMNeT++ defaults seed-set to the run number. Treatments with the
        # same run number therefore retain paired random streams, while each
        # node-count/repetition combination remains independently identified.
        found["seedset"] == str(run),
        found["nodes"] == nodes,
        found["algorithm"] == {1.0},
        found["step"] == {2.5},
        found["noise"] == {float(noise)},
        found["snr"] == {float(snr)},
        found["toa"] == {float(toa)},
        found["duty"] == {float(duty)},
        found["current"] == {float(current)},
        len(channels) == nodes and set(channels) == {"false"},
        mobile.count("true") == expected_mobile,
        mobile.count("false") == expected_static,
    ))


def archive_invalid(config, run):
    timestamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    destination = RECOVERY / timestamp
    moved = []
    for path in (result_path(config, run), log_path(config, run)):
        if path.exists():
            destination.mkdir(parents=True, exist_ok=True)
            shutil.move(str(path), destination / path.name)
            moved.append(path.name)
    marker = acceptance_marker(config, run)
    if marker.exists():
        destination.mkdir(parents=True, exist_ok=True)
        shutil.move(str(marker), destination / marker.name)
        moved.append(marker.name)
    return moved


def mark_accepted(config, run, path):
    marker = acceptance_marker(config, run)
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(
        f"configuration={config}\nrun={run}\nscalar={path.name}\n",
        encoding="utf-8")


def execute(config, run, dry_run=False):
    path = result_path(config, run)
    if scalar_is_valid(path, config, run):
        return "skip", config, run, "accepted"
    if dry_run:
        nodes, repetition = identity(run)
        return "dry", config, run, f"nodes={nodes} repetition={repetition}"

    with LOCK:
        archive_invalid(config, run)
        LOGS.mkdir(parents=True, exist_ok=True)
        SCALARS.mkdir(parents=True, exist_ok=True)
        acceptance_marker(config, run).parent.mkdir(parents=True, exist_ok=True)

    command = base_command() + ["-u", "Cmdenv", "-c", config, "-r", str(run),
                                str(INI), f"--output-scalar-file={path}"]
    with log_path(config, run).open("w", encoding="utf-8") as log:
        environment = os.environ.copy()
        environment.pop("LD_PRELOAD", None)
        completed = subprocess.run(command, cwd=EXAMPLES, env=environment,
                                   stdout=log, stderr=subprocess.STDOUT)
    if completed.returncode:
        return "fail", config, run, f"exit={completed.returncode}"
    if not scalar_is_valid(path, config, run):
        return "fail", config, run, "identity/completion audit failed"
    mark_accepted(config, run, path)
    return "pass", config, run, path.name


def parse_csv_choices(text, allowed, label):
    values = tuple(part.strip() for part in text.split(",") if part.strip())
    unknown = set(values) - set(allowed)
    if unknown:
        raise ValueError(f"Unknown {label}: {', '.join(sorted(unknown))}")
    return values


def selected_jobs(args):
    if args.target:
        match = re.fullmatch(r"([^:]+):(\d+)", args.target)
        if not match or match.group(1) not in MATRIX:
            raise ValueError("target must be ConfigName:run for a primary matrix config")
        run = int(match.group(2))
        if run < 0 or run >= 50:
            raise ValueError("run must be in 0..49")
        return [(match.group(1), run)]

    stages = parse_csv_choices(args.stages, STAGES, "stages")
    populations = parse_csv_choices(
        args.populations, (p.lower() for p in POPULATIONS), "populations")
    nodes = NODE_COUNTS if args.nodes == "all" else tuple(
        int(value) for value in args.nodes.split(","))
    if set(nodes) - set(NODE_COUNTS):
        raise ValueError("nodes must be all or comma-separated values from 100..1000")

    jobs = []
    for config in MATRIX:
        stage, population, _ = CONFIG_META[config]
        if stage not in stages or population not in populations:
            continue
        for node_count in nodes:
            for repetition in range(5):
                jobs.append((config, run_number(node_count, repetition)))
    return jobs

def write_status(counts, total):
    payload = {
        "updated_at": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "total": total,
        **counts,
    }
    destination = STATE / "status.json"
    temporary = destination.with_suffix(".tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(destination)



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("target", nargs="?", help="single ConfigName:run")
    parser.add_argument("--stages", default=",".join(STAGES))
    parser.add_argument("--populations", default="static,mobile,mixed")
    parser.add_argument("--nodes", default="all")
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    try:
        jobs = selected_jobs(args)
    except ValueError as error:
        parser.error(str(error))

    bind_campaign()
    counts = {"pass": 0, "skip": 0, "fail": 0, "dry": 0}
    write_status(counts, len(jobs))
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(execute, config, run, args.dry_run) for config, run in jobs]
        for future in as_completed(futures):
            status, config, run, detail = future.result()
            counts[status] += 1
            print(f"{status.upper():4} {config}:{run} {detail}", flush=True)
            write_status(counts, len(jobs))
    print(" ".join(f"{key}={value}" for key, value in counts.items()))
    return 2 if counts["fail"] else 0


if __name__ == "__main__":
    sys.exit(main())
