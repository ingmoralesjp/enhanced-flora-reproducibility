#!/usr/bin/env python3
"""Rebuild the platform paper from released compact data in a fresh directory."""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def run(*args, cwd=None):
    subprocess.run(args, cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output_dir.resolve()
    if output.exists():
        raise SystemExit(f"refusing to overwrite existing output: {output}")

    paper = output / "enhanced-flora-platform-paper"
    source = root / "docs/enhanced-flora-platform-paper"
    shutil.copytree(source, paper)
    corrected = (root / "experiments/enhanced-flora-platform/results"
                 / "fidelity-ablation-corrected-v0.2.0-dev/analysis")
    official = root / "evidence/official-flora-1.1"

    run(sys.executable, str(root / "analysis/generate_fidelity_ablation_paper_artifacts.py"),
        "--data-dir", str(corrected),
        "--paper-dir", str(paper / "corrected-v0.2.0-dev"))
    run(sys.executable, str(root / "analysis/regenerate_official_reference.py"),
        "--official-run-level", str(official / "official-run-level-n1000.csv"),
        "--official-failures", str(official / "official-failures-n1000.csv"),
        "--enhanced-run-level", str(corrected / "run-metrics.csv"),
        "--output", str(paper / "official-reference"))
    run(sys.executable, str(root / "analysis/generate_cross_stack_context.py"),
        "--official-run-level", str(official / "official-run-level-n1000.csv"),
        "--official-failures", str(official / "official-failures-n1000.csv"),
        "--enhanced-run-level", str(corrected / "run-metrics.csv"),
        "--output", str(paper / "official-reference"))

    for name in (
        "complete_path_table.tex", "results_status.tex",
        "dominant_adjacent_table.tex", "interval_counts_table.tex",
    ):
        generated = paper / "corrected-v0.2.0-dev/generated" / name
        released = source / "corrected-v0.2.0-dev/generated" / name
        if generated.read_bytes() != released.read_bytes():
            raise ValueError(f"Corrected paper table differs from release: {name}")
    for name in (
        "coverage-through-n1000.csv", "coverage-through-n1000.tex",
        "cross-stack-f0-f5.csv", "cross-stack-f0-f5.tex",
    ):
        if ((paper / "official-reference" / name).read_bytes()
                != (source / "official-reference" / name).read_bytes()):
            raise ValueError(f"Official paper result differs from release: {name}")

    run("make", cwd=paper)
    if not (paper / "main.pdf").is_file():
        raise ValueError("Paper PDF was not built")
    print(f"regenerated paper: {paper / 'main.pdf'}")


if __name__ == "__main__":
    main()
