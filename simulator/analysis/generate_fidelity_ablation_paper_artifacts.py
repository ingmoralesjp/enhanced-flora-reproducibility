#!/usr/bin/env python3
"""Generate the fidelity-ablation paper's audited tables and vector figures."""

import argparse
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import TwoSlopeNorm
import numpy as np
import pandas as pd


T975_DF4 = 2.7764451051977987
STAGES = [f"F{i}" for i in range(6)]
TRANSITIONS = [f"F{i}" for i in range(1, 6)]
POPULATIONS = ["static", "mobile", "mixed"]
POPULATION_LABELS = {
    "static": "Stationary",
    "mobile": "RandomWaypoint",
    "mixed": "50/50 mixed",
}
COLORS = {
    "F0": "#4D4D4D",
    "F1": "#0072B2",
    "F2": "#56B4E9",
    "F3": "#009E73",
    "F4": "#E69F00",
    "F5": "#D55E00",
}
MARKERS = dict(zip(STAGES, ["o", "s", "^", "D", "v", "P"]))


def mean_ci(values):
    values = np.asarray(values, dtype=float)
    return values.mean(), T975_DF4 * values.std(ddof=1) / math.sqrt(len(values))


def validate(raw, summary, contrasts):
    expected_cells = {
        (stage, population, nodes)
        for stage in STAGES
        for population in POPULATIONS
        for nodes in range(100, 1001, 100)
    }
    actual_cells = set()
    for key, cell in raw.groupby(["stage", "population", "nodes"]):
        actual_cells.add(key)
        if len(cell) != 5 or set(cell["repetition"]) != set(range(5)):
            raise ValueError(f"Incomplete raw cell {key}")
    if actual_cells != expected_cells or len(raw) != 900:
        raise ValueError("Raw matrix is not the audited 6 x 3 x 10 x 5 design")
    if len(summary) != 180:
        raise ValueError(f"Expected 180 summary rows, found {len(summary)}")
    if len(contrasts) != 300:
        raise ValueError(f"Expected 300 contrast rows, found {len(contrasts)}")


def paired_relative_energy(raw, stage, baseline, population, nodes):
    columns = ["repetition", "energy_per_success_j"]
    current = raw[
        (raw.stage == stage) & (raw.population == population) & (raw.nodes == nodes)
    ][columns]
    reference = raw[
        (raw.stage == baseline)
        & (raw.population == population)
        & (raw.nodes == nodes)
    ][columns]
    paired = current.merge(reference, on="repetition", suffixes=("_new", "_base"))
    differences = 100.0 * (
        paired.energy_per_success_j_new - paired.energy_per_success_j_base
    ) / paired.energy_per_success_j_base
    return mean_ci(differences)


def tex_pm(value, ci, decimals=2, signed=False):
    spec = f"+.{decimals}f" if signed else f".{decimals}f"
    return f"{value:{spec}} $\\pm$ {ci:.{decimals}f}"


def write_status(generated_dir):
    text = r"""% Generated from the audited 900-run matrix; do not edit.
\paragraph{Campaign status.} The complete $6\times3\times10\times5=900$
matrix passed full-horizon and configuration-identity audits.  The tables and
figures in this section are generated from all 180 complete five-repetition
cells and 300 paired adjacent/cumulative contrasts; no partial cell or
recovered rejected artifact is included.
"""
    (generated_dir / "results_status.tex").write_text(text, encoding="utf-8")


def write_anchor_table(raw, summary, contrasts, generated_dir):
    lines = [
        "% Generated from the audited fidelity-ablation matrix; do not edit.",
        r"\begin{table*}[t]",
        r"\centering",
        r"\caption{Complete-path comparison at representative densities. Mean intervals and paired differences are 95\% Student-$t$ intervals over five matched repetitions.}",
        r"\label{tab:complete-path}",
        r"\small",
        r"\resizebox{\textwidth}{!}{%",
        r"\begin{tabular}{@{}llrrrrrr@{}}",
        r"\toprule",
        r"Population & EDs & F0 PDR (\%) & F5 PDR (\%) & $\Delta$PDR (pp) & F0 $E_{succ}$ (mJ) & F5 $E_{succ}$ (mJ) & $\Delta E_{succ}$ (\%) \\",
        r"\midrule",
    ]
    for population in POPULATIONS:
        for row_index, nodes in enumerate((100, 500, 1000)):
            f0 = summary[
                (summary.stage == "F0")
                & (summary.population == population)
                & (summary.nodes == nodes)
            ].iloc[0]
            f5 = summary[
                (summary.stage == "F5")
                & (summary.population == population)
                & (summary.nodes == nodes)
            ].iloc[0]
            delta = contrasts[
                (contrasts.stage == "F5")
                & (contrasts.baseline == "F0")
                & (contrasts.contrast == "cumulative")
                & (contrasts.population == population)
                & (contrasts.nodes == nodes)
            ].iloc[0]
            energy_relative, energy_relative_ci = paired_relative_energy(
                raw, "F5", "F0", population, nodes
            )
            population_cell = POPULATION_LABELS[population] if row_index == 0 else ""
            lines.append(
                " & ".join(
                    [
                        population_cell,
                        str(nodes),
                        tex_pm(100 * f0.pdr_mean, 100 * f0.pdr_ci95),
                        tex_pm(100 * f5.pdr_mean, 100 * f5.pdr_ci95),
                        tex_pm(
                            100 * delta.delta_pdr_mean,
                            100 * delta.delta_pdr_ci95,
                            signed=True,
                        ),
                        tex_pm(
                            1000 * f0.energy_per_success_j_mean,
                            1000 * f0.energy_per_success_j_ci95,
                            decimals=1,
                        ),
                        tex_pm(
                            1000 * f5.energy_per_success_j_mean,
                            1000 * f5.energy_per_success_j_ci95,
                            decimals=1,
                        ),
                        tex_pm(
                            energy_relative,
                            energy_relative_ci,
                            decimals=1,
                            signed=True,
                        ),
                    ]
                )
                + r" \\"
            )
        if population != POPULATIONS[-1]:
            lines.append(r"\addlinespace")
    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}%",
            r"}",
            r"\end{table*}",
            "",
        ]
    )
    (generated_dir / "complete_path_table.tex").write_text(
        "\n".join(lines), encoding="utf-8"
    )


def write_adjacent_table(contrasts, generated_dir):
    lines = [
        "% Generated from the audited fidelity-ablation matrix; do not edit.",
        r"\begin{table*}[t]",
        r"\centering",
        r"\caption{Dominant adjacent corrections at 1000 EDs. Values are paired mean differences with 95\% confidence intervals.}",
        r"\label{tab:dominant-adjacent}",
        r"\small",
        r"\resizebox{\textwidth}{!}{%",
        r"\begin{tabular}{@{}llrrrrr@{}}",
        r"\toprule",
        r"Transition & Population & $\Delta$PDR (pp) & $\Delta E_{succ}$ (mJ) & $\Delta$ToA (ms) & $\Delta$collision loss (pp) & $\Delta$DL rejection (pp) \\",
        r"\midrule",
    ]
    for stage, baseline, label in (
        ("F3", "F2", "Physical ToA"),
        ("F4", "F3", "Duty cycle"),
    ):
        for row_index, population in enumerate(POPULATIONS):
            row = contrasts[
                (contrasts.stage == stage)
                & (contrasts.baseline == baseline)
                & (contrasts.contrast == "adjacent")
                & (contrasts.population == population)
                & (contrasts.nodes == 1000)
            ].iloc[0]
            transition_cell = f"{baseline}--{stage}: {label}" if row_index == 0 else ""
            values = [transition_cell, POPULATION_LABELS[population]]
            for metric, scale, decimals in (
                ("pdr", 100, 2),
                ("energy_per_success_j", 1000, 2),
                ("mean_time_on_air_s", 1000, 1),
                ("collision_loss_rate", 100, 2),
                ("gateway_downlink_drop_rate", 100, 2),
            ):
                values.append(
                    tex_pm(
                        scale * row[f"delta_{metric}_mean"],
                        scale * row[f"delta_{metric}_ci95"],
                        decimals=decimals,
                        signed=True,
                    )
                )
            lines.append(" & ".join(values) + r" \\")
        if stage == "F3":
            lines.append(r"\addlinespace")
    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}%",
            r"}",
            r"\end{table*}",
            "",
        ]
    )
    (generated_dir / "dominant_adjacent_table.tex").write_text(
        "\n".join(lines), encoding="utf-8"
    )


def write_significance_table(contrasts, generated_dir):
    adjacent = contrasts[contrasts.contrast == "adjacent"]
    metrics = [
        ("pdr", "PDR"),
        ("energy_per_success_j", r"$E_{succ}$"),
        ("mean_time_on_air_s", "ToA"),
        ("gateway_downlink_drop_rate", "DL rejection"),
    ]
    names = {
        "F1": "Thermal noise",
        "F2": "SNIR gate",
        "F3": "Physical ToA",
        "F4": "Duty cycle",
        "F5": "TP--current",
    }
    lines = [
        "% Generated descriptive interval-exclusion counts; do not edit.",
        r"\begin{table}[t]",
        r"\centering",
        r"\caption{Cells whose paired 95\% interval excludes zero (out of 30). Counts are descriptive and are not multiplicity-adjusted tests.}",
        r"\label{tab:interval-counts}",
        r"\small",
        r"\resizebox{\columnwidth}{!}{%",
        r"\begin{tabular}{@{}llrrrr@{}}",
        r"\toprule",
        r"Step & Correction & PDR & $E_{succ}$ & ToA & DL rej. \\",
        r"\midrule",
    ]
    for stage in TRANSITIONS:
        group = adjacent[adjacent.stage == stage]
        counts = []
        for metric, _ in metrics:
            excluded = (
                group[f"delta_{metric}_mean"].abs()
                > group[f"delta_{metric}_ci95"]
            ).sum()
            counts.append(str(int(excluded)))
        lines.append(
            " & ".join([stage, names[stage], *counts]) + r" \\"
        )
    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}%",
            r"}",
            r"\end{table}",
            "",
        ]
    )
    (generated_dir / "interval_counts_table.tex").write_text(
        "\n".join(lines), encoding="utf-8"
    )


def style_axes(axis):
    axis.grid(True, color="#D9D9D9", linewidth=0.5, alpha=0.8)
    axis.spines[["top", "right"]].set_visible(False)
    axis.set_xlim(80, 1020)
    axis.set_xticks(range(100, 1001, 100))
    axis.tick_params(labelsize=7)


def plot_outcomes(summary, figures_dir):
    fig, axes = plt.subplots(2, 3, figsize=(10.2, 5.6), sharex=True)
    for column, population in enumerate(POPULATIONS):
        for stage in STAGES:
            data = summary[
                (summary.population == population) & (summary.stage == stage)
            ].sort_values("nodes")
            for row, metric, scale in (
                (0, "pdr", 100),
                (1, "energy_per_success_j", 1000),
            ):
                axis = axes[row, column]
                mean = scale * data[f"{metric}_mean"].to_numpy()
                ci = scale * data[f"{metric}_ci95"].to_numpy()
                axis.plot(
                    data.nodes,
                    mean,
                    color=COLORS[stage],
                    marker=MARKERS[stage],
                    markersize=3.0,
                    linewidth=1.1,
                    label=stage,
                )
                axis.fill_between(
                    data.nodes, mean - ci, mean + ci, color=COLORS[stage], alpha=0.08
                )
        axes[0, column].set_title(POPULATION_LABELS[population], fontsize=9)
        axes[1, column].set_xlabel("End devices")
        style_axes(axes[0, column])
        style_axes(axes[1, column])
    axes[0, 0].set_ylabel("PDR (%)")
    axes[1, 0].set_ylabel(r"$E_{succ}$ (mJ/delivery)")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=6, frameon=False, fontsize=8)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    fig.savefig(figures_dir / "outcome_scaling.pdf", bbox_inches="tight")
    plt.close(fig)


def plot_adjacent_heatmaps(contrasts, figures_dir):
    adjacent = contrasts[contrasts.contrast == "adjacent"]
    specifications = [
        ("pdr", 100, r"Paired $\Delta$PDR (percentage points)"),
        ("energy_per_success_j", 1000, r"Paired $\Delta E_{succ}$ (mJ)"),
    ]
    maxima = {}
    for metric, scale, _ in specifications:
        maxima[metric] = max(
            1e-12, scale * adjacent[f"delta_{metric}_mean"].abs().max()
        )
    fig, axes = plt.subplots(2, 3, figsize=(10.2, 5.0), sharex=True, sharey=True)
    images = []
    for row_index, (metric, scale, label) in enumerate(specifications):
        for column, population in enumerate(POPULATIONS):
            data = adjacent[adjacent.population == population]
            values = np.empty((5, 10))
            significant = np.zeros((5, 10), dtype=bool)
            for stage_index, stage in enumerate(TRANSITIONS):
                stage_rows = data[data.stage == stage].set_index("nodes")
                for node_index, nodes in enumerate(range(100, 1001, 100)):
                    item = stage_rows.loc[nodes]
                    values[stage_index, node_index] = scale * item[f"delta_{metric}_mean"]
                    significant[stage_index, node_index] = (
                        abs(item[f"delta_{metric}_mean"])
                        > item[f"delta_{metric}_ci95"]
                    )
            axis = axes[row_index, column]
            image = axis.imshow(
                values,
                aspect="auto",
                cmap="RdBu_r",
                norm=TwoSlopeNorm(
                    vmin=-maxima[metric], vcenter=0, vmax=maxima[metric]
                ),
                interpolation="nearest",
            )
            images.append(image)
            ys, xs = np.where(significant)
            axis.scatter(xs, ys, s=9, facecolor="white", edgecolor="black", linewidth=0.35)
            axis.set_xticks(range(10), range(100, 1001, 100), rotation=45)
            axis.set_yticks(range(5), [f"F{i-1}$\\to$F{i}" for i in range(1, 6)])
            axis.tick_params(labelsize=7)
            if row_index == 0:
                axis.set_title(POPULATION_LABELS[population], fontsize=9)
            if row_index == 1:
                axis.set_xlabel("End devices")
        colorbar = fig.colorbar(images[-1], ax=axes[row_index, :], fraction=0.022, pad=0.02)
        colorbar.set_label(label, fontsize=8)
        colorbar.ax.tick_params(labelsize=7)
    fig.text(
        0.5,
        0.01,
        "White circles mark paired 95% intervals that exclude zero.",
        ha="center",
        fontsize=8,
    )
    fig.subplots_adjust(left=0.08, right=0.91, top=0.92, bottom=0.13, hspace=0.28, wspace=0.12)
    fig.savefig(figures_dir / "adjacent_effects.pdf", bbox_inches="tight")
    plt.close(fig)


def plot_mechanisms(summary, figures_dir):
    fig, axes = plt.subplots(3, 3, figsize=(10.2, 7.4), sharex=True)
    stage_subset = ["F2", "F3", "F4"]
    for column, population in enumerate(POPULATIONS):
        for stage in stage_subset:
            data = summary[
                (summary.population == population) & (summary.stage == stage)
            ].sort_values("nodes")
            for row, metric, scale in (
                (0, "mean_time_on_air_s", 1000),
                (1, "collision_loss_rate", 100),
            ):
                mean = scale * data[f"{metric}_mean"].to_numpy()
                ci = scale * data[f"{metric}_ci95"].to_numpy()
                axes[row, column].plot(
                    data.nodes,
                    mean,
                    color=COLORS[stage],
                    marker=MARKERS[stage],
                    markersize=3,
                    linewidth=1.2,
                    label=stage,
                )
                axes[row, column].fill_between(
                    data.nodes, mean - ci, mean + ci, color=COLORS[stage], alpha=0.1
                )
        for stage in ("F3", "F4"):
            data = summary[
                (summary.population == population) & (summary.stage == stage)
            ].sort_values("nodes")
            axes[2, column].plot(
                data.nodes,
                100 * data.gateway_observed_duty_cycle_mean,
                color=COLORS[stage],
                marker=MARKERS[stage],
                markersize=3,
                linewidth=1.2,
                label=f"{stage} observed duty",
            )
            axes[2, column].plot(
                data.nodes,
                100 * data.gateway_downlink_drop_rate_mean,
                color=COLORS[stage],
                linestyle="--",
                linewidth=1.2,
                label=f"{stage} DL rejection",
            )
        axes[0, column].set_title(POPULATION_LABELS[population], fontsize=9)
        axes[2, column].set_xlabel("End devices")
        for row in range(3):
            style_axes(axes[row, column])
    axes[0, 0].set_ylabel("Mean packet ToA (ms)")
    axes[1, 0].set_ylabel("Collision loss rate (%)")
    axes[2, 0].set_ylabel("Gateway outcome (%)")
    handles_a, labels_a = axes[0, 0].get_legend_handles_labels()
    handles_b, labels_b = axes[2, 0].get_legend_handles_labels()
    fig.legend(
        handles_a + handles_b,
        labels_a + labels_b,
        loc="upper center",
        ncol=7,
        frameon=False,
        fontsize=7,
    )
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    fig.savefig(figures_dir / "mechanism_scaling.pdf", bbox_inches="tight")
    plt.close(fig)


def main():
    flora_dir = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--data-dir",
        type=Path,
        default=flora_dir / "simulations/results/fidelity-ablation-20260909/state",
    )
    parser.add_argument(
        "--paper-dir",
        type=Path,
        default=flora_dir / "docs/enhanced-flora-fidelity-ablation-paper",
    )
    args = parser.parse_args()

    raw = pd.read_csv(args.data_dir / "checkpoint-metrics.csv")
    summary = pd.read_csv(args.data_dir / "summary-ci95.csv")
    contrasts = pd.read_csv(args.data_dir / "paired-contrasts-ci95.csv")
    validate(raw, summary, contrasts)

    figures_dir = args.paper_dir / "figures"
    generated_dir = args.paper_dir / "generated"
    figures_dir.mkdir(parents=True, exist_ok=True)
    generated_dir.mkdir(parents=True, exist_ok=True)

    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 8,
            "axes.labelsize": 8,
            "axes.titlesize": 9,
            "legend.fontsize": 7,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    write_status(generated_dir)
    write_anchor_table(raw, summary, contrasts, generated_dir)
    write_adjacent_table(contrasts, generated_dir)
    write_significance_table(contrasts, generated_dir)
    plot_outcomes(summary, figures_dir)
    plot_adjacent_heatmaps(contrasts, figures_dir)
    plot_mechanisms(summary, figures_dir)


if __name__ == "__main__":
    main()
