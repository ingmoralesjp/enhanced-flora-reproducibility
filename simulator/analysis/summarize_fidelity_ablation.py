#!/usr/bin/env python3
"""Build complete-cell means, CIs, and paired fidelity-ablation contrasts."""

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path


T975_DF4 = 2.7764451051977987
METRICS = (
    "pdr", "energy_per_success_j", "energy_per_attempt_j",
    "wasted_energy_index_j", "mean_time_on_air_s",
    "gateway_downlink_drop_rate", "gateway_observed_duty_cycle",
    "below_sensitivity_rate", "low_snr_loss_rate", "collision_loss_rate",
    "mean_sf_changes_per_node", "mean_tp_changes_per_node",
)
STAGES = tuple(f"F{index}" for index in range(6))


def mean_ci(values):
    mean = statistics.fmean(values)
    ci = T975_DF4 * statistics.stdev(values) / math.sqrt(len(values))
    return mean, ci


def read_complete_cells(path):
    with path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    cells = defaultdict(list)
    for row in rows:
        key = (row["stage"], row["population"], int(row["nodes"]))
        cells[key].append(row)
    incomplete = []
    for key, cell in cells.items():
        repetitions = {int(row["repetition"]) for row in cell}
        seeds = {int(row["seed_set"]) for row in cell}
        if len(cell) != 5 or repetitions != set(range(5)) or len(seeds) != 5:
            incomplete.append(
                f"{key}: rows={len(cell)} repetitions={sorted(repetitions)} "
                f"seeds={sorted(seeds)}"
            )
    expected = {(stage, population, nodes) for stage in STAGES
                for population in ("static", "mobile", "mixed")
                for nodes in range(100, 1001, 100)}
    missing = expected - set(cells)
    if incomplete or missing:
        details = incomplete + [f"missing {key}" for key in sorted(missing)]
        raise ValueError("Primary matrix is incomplete:\n" + "\n".join(details))
    return cells


def build_summary(cells):
    output = []
    for (stage, population, nodes), rows in sorted(cells.items()):
        record = {"stage": stage, "population": population, "nodes": nodes, "replications": 5}
        for metric in METRICS:
            values = [float(row[metric]) for row in rows]
            mean, ci = mean_ci(values)
            record[f"{metric}_mean"] = mean
            record[f"{metric}_ci95"] = ci
        output.append(record)
    return output


def build_contrasts(cells):
    output = []
    for population in ("static", "mobile", "mixed"):
        for nodes in range(100, 1001, 100):
            indexed = {}
            for stage in STAGES:
                indexed[stage] = {int(row["repetition"]): row
                                  for row in cells[(stage, population, nodes)]}
            for stage_index in range(1, len(STAGES)):
                stage = STAGES[stage_index]
                for baseline, contrast_type in ((STAGES[stage_index - 1], "adjacent"),
                                                ("F0", "cumulative")):
                    record = {
                        "stage": stage, "baseline": baseline,
                        "contrast": contrast_type, "population": population,
                        "nodes": nodes, "pairs": 5,
                    }
                    for metric in METRICS:
                        differences = [
                            float(indexed[stage][repetition][metric]) -
                            float(indexed[baseline][repetition][metric])
                            for repetition in range(5)
                        ]
                        mean, ci = mean_ci(differences)
                        record[f"delta_{metric}_mean"] = mean
                        record[f"delta_{metric}_ci95"] = ci
                    output.append(record)
    return output


def write_rows(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as destination:
        writer = csv.DictWriter(destination, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("metrics", type=Path)
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("--contrasts", type=Path, required=True)
    args = parser.parse_args()
    cells = read_complete_cells(args.metrics)
    write_rows(args.summary, build_summary(cells))
    write_rows(args.contrasts, build_contrasts(cells))


if __name__ == "__main__":
    main()
