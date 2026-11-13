#!/usr/bin/env python3
"""Build an instrumented disposable copy and audit ten short MAC paths.

The assertion patch is applied only to a temporary copy of the released
source. It cannot alter the frozen F0--F5 campaign or the release manifest.
"""

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
from datetime import datetime, timezone
from pathlib import Path


CONFIGS = (
    "FidelityValidateF0", "FidelityValidateF1", "FidelityValidateF2",
    "FidelityValidateF3", "FidelityValidateF4", "FidelityValidateF5",
    "FidelityValidateLegacyDuty", "FidelityValidateToaDuty",
    "FidelityValidateF5Mobile", "FidelityValidateF5Mixed",
)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def checked(command, cwd, log):
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(
            command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
            check=False)
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release", type=Path)
    parser.add_argument("--inet-dir", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--patch", type=Path,
                        help="override assertion patch when testing an older package")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    root = args.release.resolve()
    patch = (args.patch or root / "tests/0001-assert-one-mac-send-per-event.patch").resolve()
    inet = args.inet_dir.resolve()
    output = args.output_dir.resolve()
    if output.exists():
        raise SystemExit(f"refusing to overwrite existing output: {output}")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    inet_lib = inet / "out/clang-release/src/libINET.so"
    if not inet_lib.is_file():
        raise ValueError(f"Built INET library missing: {inet_lib}")
    output.mkdir(parents=True)
    (output / "logs").mkdir()
    (output / "scalars").mkdir()

    frozen_mac_sha = sha256(root / "src/LoRa/LoRaMac.cc")
    with tempfile.TemporaryDirectory(prefix="flora-mac-regression-") as temporary:
        base = Path(temporary)
        work = base / "enhanced-flora"
        work.mkdir()
        shutil.copy2(root / "Makefile", work / "Makefile")
        shutil.copytree(root / "src", work / "src")
        shutil.copytree(root / "simulations", work / "simulations")
        (base / "inet").symlink_to(inet, target_is_directory=True)
        checked(
            ["patch", "-p0", "--forward", "--no-backup-if-mismatch",
             "-i", str(patch)],
            work, output / "logs/patch.log")
        checked(["make", "makefiles"], work, output / "logs/makefiles.log")
        checked(["make", "MODE=release", f"-j{args.jobs}"],
                work, output / "logs/build.log")
        libraries = {path.resolve() for path in work.rglob("libflora.so")
                     if path.is_file()}
        built = {path for path in libraries if "out" in path.parts}
        if len(built) != 1:
            raise ValueError(f"Expected one built library, found {libraries}")
        library = built.pop()

        for config in CONFIGS:
            log = output / "logs" / f"{config}.log"
            scalar = output / "scalars" / f"{config}.sca"
            command = [
                "opp_run", "-u", "Cmdenv", "-c", config, "-r", "0",
                "-n", f"{work/'src'}:{work/'simulations'}:{inet/'src'}",
                "-l", str(library), "-l", str(inet_lib),
                f"--output-scalar-file={scalar}",
                "fidelity-ablation.ini",
            ]
            checked(command, work / "simulations/examples", log)
            text = log.read_text(encoding="utf-8", errors="replace")
            if ("Simulation time limit reached" not in text
                    or "End." not in text
                    or "Duplicate MAC send" in text
                    or "already transmitting" in text
                    or not scalar.is_file()):
                raise ValueError(f"Incomplete or failed diagnostic path: {config}")

    if sha256(root / "src/LoRa/LoRaMac.cc") != frozen_mac_sha:
        raise ValueError("Frozen source changed during diagnostic test")
    receipt = {
        "status": "PASS",
        "checked_at_utc": datetime.now(timezone.utc).isoformat(),
        "source_mac_sha256": frozen_mac_sha,
        "assertion_patch_sha256": sha256(patch),
        "inet_library_sha256": sha256(inet_lib),
        "configurations": list(CONFIGS),
        "scope": "ten short instrumented paths; not a full-horizon proof",
    }
    (output / "receipt.json").write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
