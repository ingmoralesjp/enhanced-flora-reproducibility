#!/usr/bin/env python3
"""Run a new F0--F5 experiment without touching the frozen paper campaign.

Install this script as ``tools/rerun_corrected_release.py`` in the compact
Enhanced FLoRa distribution. Accepted jobs are resumable by exact hashes.
"""

import argparse
import hashlib
import json
import subprocess
import sys
import uuid
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path


STAGES = {
    "F0": ("Legacy", "f0-legacy"),
    "F1": ("ThermalNoise", "f1-noise"),
    "F2": ("SnrReception", "f2-reception"),
    "F3": ("SemtechToa", "f3-toa"),
    "F4": ("ToaDutyCycle", "f4-duty"),
    "F5": ("Enhanced", "f5-enhanced"),
}
POPULATIONS = ("static", "mobile", "mixed")
PAPER_LIMIT = 1036800


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def selection(value, valid, label):
    items = value.split(",")
    if not items or len(items) != len(set(items)) or any(x not in valid for x in items):
        raise ValueError(f"Invalid or duplicate {label}: {value}")
    return items


def scalar_identity(path, config, run, nodes, repetition):
    values = {}
    with path.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if line.startswith("attr "):
                fields = line.strip().split(maxsplit=2)
                if len(fields) == 3:
                    values[fields[1]] = fields[2]
            if line.startswith(("scalar ", "statistic ")):
                break
    return (values.get("configname") == config
            and values.get("runnumber") == str(run)
            and values.get("repetition") == str(repetition)
            and f"$nodes={nodes}" in values.get("iterationvars", ""))


def job_name(stage, population, nodes, repetition):
    return f"{STAGES[stage][1]}-{population}-n{nodes}-r{repetition}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inet-dir", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--stages", default="F0,F1,F2,F3,F4,F5")
    parser.add_argument("--populations", default="static,mobile,mixed")
    parser.add_argument("--nodes", default="100,200,300,400,500,600,700,800,900,1000")
    parser.add_argument("--repetitions", type=int, default=5,
                        help="count per cell; the paper configuration supports 1--5")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--limit-seconds", type=int, default=PAPER_LIMIT)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    inet = args.inet_dir.resolve()
    output = args.output_dir.resolve()
    if root == output or root in output.parents:
        parser.error("output directory must be outside the release tree")
    if not 1 <= args.repetitions <= 5 or args.jobs < 1 or args.limit_seconds < 1:
        parser.error("repetitions must be 1--5; jobs and limit must be positive")
    try:
        stages = selection(args.stages, STAGES, "stages")
        populations = selection(args.populations, POPULATIONS, "populations")
        nodes = [int(x) for x in selection(args.nodes,
                                            [str(x) for x in range(100, 1001, 100)],
                                            "nodes")]
    except ValueError as error:
        parser.error(str(error))
    jobs = [(s, p, n, r) for s in stages for p in populations for n in nodes
            for r in range(args.repetitions)]
    if args.dry_run:
        print(json.dumps({"jobs": len(jobs), "first": jobs[0], "last": jobs[-1],
                          "limit_seconds": args.limit_seconds,
                          "paper_comparable_horizon": args.limit_seconds == PAPER_LIMIT},
                         indent=2))
        return 0

    binary = root / "out/clang-release/src/libflora.so"
    inet_binary = inet / "out/clang-release/src/libINET.so"
    ini_dir = root / "simulations/examples"
    for path in (binary, inet_binary, ini_dir / "fidelity-ablation.ini"):
        if not path.is_file():
            raise ValueError(f"Required built dependency missing: {path}")
    identity = {
        "source_manifest_sha256": digest(root / "MANIFEST.sha256"),
        "libflora_sha256": digest(binary),
        "libinet_sha256": digest(inet_binary),
        "ini_sha256": digest(ini_dir / "Analisis-fidelity-ablation.ini"),
        "stages": stages, "populations": populations, "nodes": nodes,
        "repetitions": args.repetitions, "limit_seconds": args.limit_seconds,
        "jobs": len(jobs),
        "comparison_warning": "New build/campaign; do not pool with frozen paper results without an equivalence test",
    }
    manifest = output / "rerun-identity.json"
    if args.resume:
        if not manifest.is_file() or json.loads(manifest.read_text()) != identity:
            raise ValueError("Cannot resume: rerun identity differs or manifest missing")
    else:
        if output.exists():
            raise ValueError(f"Refusing to overwrite existing output: {output}")
        for name in ("scalars", "logs", "accepted", "attempts"):
            (output / name).mkdir(parents=True)
        manifest.write_text(json.dumps(identity, indent=2, sort_keys=True) + "\n")

    def run_one(job):
        stage, population, node_count, repetition = job
        name = job_name(*job)
        config = f"Fidelity{stage}{STAGES[stage][0]}{population.capitalize()}"
        run = (node_count // 100 - 1) * 5 + repetition
        scalar = output / "scalars" / f"{name}.sca"
        log = output / "logs" / f"{name}.log"
        receipt = output / "accepted" / f"{name}.ok"
        if receipt.exists():
            previous = json.loads(receipt.read_text())
            if (previous["configuration"] == config and previous["run"] == run
                    and scalar.is_file() and log.is_file()
                    and previous["scalar_sha256"] == digest(scalar)
                    and previous["log_sha256"] == digest(log)):
                return ("SKIP", name, "hash-verified accepted result")
            raise ValueError(f"Accepted receipt or output was altered: {name}")
        if scalar.exists() or log.exists():
            raise ValueError(f"Unreceipted final output exists: {name}")
        attempt = uuid.uuid4().hex[:12]
        temporary_scalar = output / "attempts" / f"{name}-{attempt}.sca"
        temporary_log = output / "attempts" / f"{name}-{attempt}.log"
        command = [
            "opp_run", "-u", "Cmdenv", "-c", config, "-r", str(run),
            "-n", f"{root/'src'}:{root/'simulations'}:{inet/'src'}",
            "-l", str(binary), "-l", str(inet_binary),
            f"--sim-time-limit={args.limit_seconds}s",
            f"--output-scalar-file={temporary_scalar}",
            "fidelity-ablation.ini",
        ]
        with temporary_log.open("w") as stream:
            result = subprocess.run(command, cwd=ini_dir, stdout=stream,
                                    stderr=subprocess.STDOUT, check=False)
        log_text = temporary_log.read_text(errors="replace")
        valid = (result.returncode == 0 and temporary_scalar.is_file()
                 and temporary_scalar.stat().st_size > 0
                 and f"Simulation time limit reached -- at t={args.limit_seconds}s" in log_text
                 and "End." in log_text
                 and scalar_identity(temporary_scalar, config, run, node_count, repetition))
        if not valid:
            return ("FAIL", name, f"exit={result.returncode}; see {temporary_log}")
        temporary_scalar.replace(scalar)
        temporary_log.replace(log)
        value = {"configuration": config, "run": run, "nodes": node_count,
                 "repetition": repetition, "limit_seconds": args.limit_seconds,
                 "scalar_sha256": digest(scalar), "log_sha256": digest(log)}
        receipt.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
        return ("PASS", name, "accepted")

    counts = {"PASS": 0, "SKIP": 0, "FAIL": 0}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run_one, job) for job in jobs]
        for future in as_completed(futures):
            state, name, detail = future.result()
            counts[state] += 1
            print(f"{state} {name}: {detail}", flush=True)
    status = {"updated_at_utc": datetime.now(timezone.utc).isoformat(),
              "total": len(jobs), **counts}
    (output / "status.json").write_text(json.dumps(status, indent=2, sort_keys=True) + "\n")
    print(json.dumps(status, indent=2, sort_keys=True))
    return 2 if counts["FAIL"] else 0


if __name__ == "__main__":
    sys.exit(main())
