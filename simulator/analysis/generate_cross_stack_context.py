#!/usr/bin/env python3
"""Generate descriptive official/F0/F5 context without treating official as F0.

The official and corrected-source stacks are independent. Only F0/F5 share
the corrected simulator and paired seed map; no cross-stack contrast is tested.
"""

import argparse
import csv
import math
import statistics
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


NODES = tuple(range(100, 1001, 100))
T975 = {5: 2.776445, 8: 2.364624, 9: 2.306004, 10: 2.262157}


def read_rows(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def mean_ci(rows):
    values = [100 * float(row["pdr"]) for row in rows]
    if len(values) not in T975 or not all(0 <= x <= 100 for x in values):
        raise ValueError("Invalid sample size or PDR")
    return (statistics.mean(values),
            T975[len(values)] * statistics.stdev(values) / math.sqrt(len(values)))


def make_table(out, data):
    lines = [
        r"\begin{table*}[t]",
        r"\centering",
        r"\caption{Static one-gateway PDR context. Official FLoRa 1.1 uses completed runs only; F0 and F5 are paired within the corrected Enhanced source. Intervals are within-series 95\% Student-$t$ intervals, not intervals for cross-stack differences. Failed official runs have no PDR.}",
        r"\label{tab:cross-stack-context}",
        r"\small",
        r"\setlength{\tabcolsep}{5pt}",
        r"\begin{tabular}{@{}rcccc@{}}",
        r"\toprule",
        r"EDs & Official valid & Official PDR (\%) & Corrected F0 PDR (\%) & Corrected F5 PDR (\%) \\",
        r"\midrule",
    ]
    for row in data:
        lines.append(
            f"{row['nodes']} & {row['official_valid']}/10 & "
            f"{row['official_pdr_pct']:.2f} $\\pm$ {row['official_ci95_pp']:.2f} & "
            f"{row['f0_pdr_pct']:.2f} $\\pm$ {row['f0_ci95_pp']:.2f} & "
            f"{row['f5_pdr_pct']:.2f} $\\pm$ {row['f5_ci95_pp']:.2f} \\\\"
        )
    lines += [r"\bottomrule", r"\end{tabular}", r"\end{table*}"]
    (out / "cross-stack-f0-f5.tex").write_text("\n".join(lines) + "\n")


def make_figure(out, data):
    plt.rcParams.update({"font.size": 9, "pdf.fonttype": 42})
    fig, (pdr, coverage) = plt.subplots(
        1, 2, figsize=(6.8, 2.30), gridspec_kw={"width_ratios": [2.15, 1]},
        constrained_layout=True)
    x = [row["nodes"] for row in data]
    for key, ci, marker, style, color, label in (
        ("official_pdr_pct", "official_ci95_pp", "o", ":", "#555555",
         "Official 1.1: completed only"),
        ("f0_pdr_pct", "f0_ci95_pp", "s", "-", "#19659a",
         "Enhanced-source F0"),
        ("f5_pdr_pct", "f5_ci95_pp", "^", "--", "#c15b17",
         "Enhanced-source F5"),
    ):
        pdr.errorbar(x, [row[key] for row in data],
                     yerr=[row[ci] for row in data], marker=marker,
                     linestyle=style, color=color, linewidth=1.4,
                     markersize=4, capsize=2.5, label=label)
    pdr.set(xlabel="End devices", ylabel="PDR (%)", ylim=(0, 90),
            xticks=[100, 300, 500, 700, 1000])
    pdr.grid(alpha=0.2)
    pdr.legend(frameon=False, fontsize=7, loc="upper right")
    coverage.bar(x, [row["official_valid"] for row in data], width=62,
                 color="#777777", alpha=0.85)
    coverage.set(xlabel="End devices", ylabel="Official completed / 10",
                 ylim=(0, 10.8), yticks=[0, 2, 4, 6, 8, 10],
                 xticks=[100, 500, 1000])
    coverage.axhline(10, color="#222222", linestyle=":", linewidth=0.8)
    coverage.grid(axis="y", alpha=0.2)
    fig.savefig(out / "cross-stack-f0-f5.pdf")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--official-run-level", required=True, type=Path)
    parser.add_argument("--official-failures", required=True, type=Path)
    parser.add_argument("--enhanced-run-level", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    official = read_rows(args.official_run_level)
    failed = read_rows(args.official_failures)
    enhanced = [row for row in read_rows(args.enhanced_run_level)
                if row["population"] == "static" and row["stage"] in ("F0", "F5")]
    if (len(official), len(failed), len(enhanced)) != (92, 8, 100):
        raise ValueError("Expected 92/8 official and 50+50 corrected observations")

    data = []
    for nodes in NODES:
        o = [r for r in official if int(r["nodes"]) == nodes]
        bad = [r for r in failed if int(r["nodes"]) == nodes]
        f0 = [r for r in enhanced if int(r["nodes"]) == nodes and r["stage"] == "F0"]
        f5 = [r for r in enhanced if int(r["nodes"]) == nodes and r["stage"] == "F5"]
        o_reps = {int(r["repetition"]) for r in o}
        bad_reps = {int(r["repetition"]) for r in bad}
        if (o_reps & bad_reps or o_reps | bad_reps != set(range(10))
                or {int(r["repetition"]) for r in f0} != set(range(5))
                or {int(r["repetition"]) for r in f5} != set(range(5))):
            raise ValueError(f"Incomplete or overlapping run map at {nodes} devices")
        opdr, oci = mean_ci(o)
        f0pdr, f0ci = mean_ci(f0)
        f5pdr, f5ci = mean_ci(f5)
        data.append({
            "nodes": nodes, "official_valid": len(o), "official_failed": len(bad),
            "official_pdr_pct": opdr, "official_ci95_pp": oci,
            "f0_valid": len(f0), "f0_pdr_pct": f0pdr, "f0_ci95_pp": f0ci,
            "f5_valid": len(f5), "f5_pdr_pct": f5pdr, "f5_ci95_pp": f5ci,
        })
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / "cross-stack-f0-f5.csv").open(
            "w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(data[0]))
        writer.writeheader()
        writer.writerows(data)
    make_table(args.output, data)
    make_figure(args.output, data)
    print("generated ten-density official/F0/F5 descriptive context")


if __name__ == "__main__":
    main()
