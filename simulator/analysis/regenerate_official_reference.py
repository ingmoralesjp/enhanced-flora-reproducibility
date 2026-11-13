#!/usr/bin/env python3
"""Regenerate the static official/Enhanced table and figure from compact data.

Inputs are independent run-level stacks. Failed official runs have no PDR.
The plotted intervals describe each stack separately, never a causal contrast.
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


def rows(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def mean_ci(values):
    n = len(values)
    return (statistics.mean(values),
            T975[n] * statistics.stdev(values) / math.sqrt(n))


def make_table(output, data):
    lines = [
        r"\begin{table}[t]",
        r"\centering",
        r"\caption{Pristine official FLoRa 1.1 and corrected Enhanced F5 under the distribution-matched static scenario. PDR means and 95\% $t$ intervals use completed runs only; the stacks are independent and the gap is descriptive, not causal. Failed official seeds have no PDR.}",
        r"\label{tab:official-partial-pdr}",
        r"\scriptsize",
        r"\setlength{\tabcolsep}{3pt}",
        r"\begin{tabular}{@{}rccc@{}}",
        r"\toprule",
        r"Nodes & Official valid & Official PDR (\%) & Enhanced F5 PDR (\%) \\",
        r"\midrule",
    ]
    for row in data:
        lines.append(
            f"{row['nodes']} & {row['official_accepted']}/10 & "
            f"{row['official_pdr_pct']:.2f} $\\pm$ {row['official_ci95_pp']:.2f} & "
            f"{row['enhanced_pdr_pct']:.2f} $\\pm$ {row['enhanced_ci95_pp']:.2f} \\\\"
        )
    lines += [r"\bottomrule", r"\end{tabular}", r"\end{table}"]
    (output / "coverage-through-n1000.tex").write_text(
        "\n".join(lines) + "\n", encoding="utf-8")


def make_plot(output, data):
    plt.rcParams.update({"font.size": 9, "pdf.fonttype": 42})
    fig, ax = plt.subplots(figsize=(3.35, 2.65))
    x = [row["nodes"] for row in data]
    for key, ci, marker, label in (
        ("official_pdr_pct", "official_ci95_pp", "o", "Official 1.1"),
        ("enhanced_pdr_pct", "enhanced_ci95_pp", "s", "Enhanced F5"),
    ):
        ax.errorbar(x, [row[key] for row in data],
                    yerr=[row[ci] for row in data], marker=marker,
                    linewidth=1.5, capsize=3, label=label)
    ax.set(xlabel="End devices", ylabel="PDR (%)",
           xticks=[100, 300, 500, 700, 1000], ylim=(0, 75))
    ax.grid(alpha=0.25)
    ax.legend(frameon=False, fontsize=8)
    fig.tight_layout()
    fig.savefig(output / "pdr-through-n1000.pdf")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--official-run-level", required=True, type=Path)
    parser.add_argument("--official-failures", required=True, type=Path)
    parser.add_argument("--enhanced-run-level", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    official = rows(args.official_run_level)
    failed = rows(args.official_failures)
    enhanced = [
        row for row in rows(args.enhanced_run_level)
        if row["stage"] == "F5" and row["population"] == "static"
    ]
    if len(official) != 92 or len(failed) != 8 or len(enhanced) != 50:
        raise ValueError("Compact inputs have incorrect 92/8/50 dimensions")

    output = []
    for nodes in NODES:
        valid = [row for row in official if int(row["nodes"]) == nodes]
        broken = [row for row in failed if int(row["nodes"]) == nodes]
        e = [row for row in enhanced if int(row["nodes"]) == nodes]
        valid_reps = {int(row["repetition"]) for row in valid}
        failed_reps = {int(row["repetition"]) for row in broken}
        enhanced_reps = {int(row["repetition"]) for row in e}
        if (valid_reps & failed_reps
                or valid_reps | failed_reps != set(range(10))
                or enhanced_reps != set(range(5))):
            raise ValueError(f"Incomplete or overlapping run map at {nodes}")
        opdr, oci = mean_ci([100 * float(row["pdr"]) for row in valid])
        epdr, eci = mean_ci([100 * float(row["pdr"]) for row in e])
        output.append({
            "nodes": nodes,
            "official_accepted": len(valid),
            "official_failed": len(broken),
            "official_pdr_pct": opdr,
            "official_ci95_pp": oci,
            "official_final_sf7_pct":
                statistics.mean(100 * float(row["final_sf7_fraction"]) for row in valid),
            "official_final_sf12_pct":
                statistics.mean(100 * float(row["final_sf12_fraction"]) for row in valid),
            "enhanced_accepted": len(e),
            "enhanced_pdr_pct": epdr,
            "enhanced_ci95_pp": eci,
            "descriptive_gap_pp": epdr - opdr,
        })
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / "coverage-through-n1000.csv").open(
            "w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(output[0]))
        writer.writeheader()
        writer.writerows(output)
    make_table(args.output, output)
    make_plot(args.output, output)
    print(f"regenerated {len(output)} densities from portable run-level inputs")


if __name__ == "__main__":
    main()
