#!/usr/bin/env python3
"""Extract publication metrics and treatment identity from fidelity scalars."""

import argparse
import csv
import re
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

from adr_comparison_metrics import parse_scalar_file, summarize


CONFIG_RE = re.compile(
    r"^FidelityF(?P<stage>[0-5])(?P<label>[A-Za-z]+)(?P<population>Static|Mobile|Mixed)$")
VALIDATION_RE = re.compile(r"^FidelityValidateF(?P<stage>[0-5])$")


def extract(path):
    row = summarize(path)
    match = CONFIG_RE.match(row["configuration"])
    validation_match = VALIDATION_RE.match(row["configuration"])
    if not match and not validation_match:
        raise ValueError(f"Unsupported fidelity configuration in {path}: {row['configuration']}")
    stage = match.group("stage") if match else validation_match.group("stage")
    population = match.group("population").lower() if match else "static"
    _, scalars = parse_scalar_file(path)

    def values(name, module_fragment=None):
        return [value for module, scalar_name, value in scalars
                if scalar_name == name and
                (module_fragment is None or module_fragment in module)]

    def unique(name, module_fragment=None):
        found = set(values(name, module_fragment))
        if len(found) != 1:
            raise ValueError(f"{path}: expected one unique {name}, got {sorted(found)}")
        return next(iter(found))

    sent = row["sent_data"]
    losses = {
        "below_sensitivity": sum(values("rcvBelowSensitivity_NotCollided", ".loRaGW[")),
        "low_snr": sum(values("numLowSNRLosses", ".loRaGW[")),
        "collisions": sum(values("numCollisions", ".loRaGW[")),
        "other_phy": sum(values("otherPhyLosses", ".loRaGW[")),
    }
    return {
        "file": row["file"],
        "configuration": row["configuration"],
        "stage": f"F{stage}",
        "population": population,
        "nodes": row["nodes"],
        "repetition": row["repetition"],
        "run_number": row["run_number"],
        "seed_set": row["seed_set"],
        "adr_step_db": unique("adrStepSizeDb", "networkServer.app[0]"),
        "noise_code": unique("backgroundNoiseModelCode"),
        "snr_gate_code": unique("requiredSnrGateEnabled"),
        "toa_code": unique("airtimeModelCode"),
        "duty_code": unique("GW_dutyCycleModelCode"),
        "tx_current_code": unique("txCurrentLookupModelCode"),
        "sent_data": sent,
        "received_data": row["received_data"],
        "pdr": row["pdr"],
        "measurement_energy_j": row["measurement_energy_j"],
        "energy_per_success_j": row["energy_per_success_j"],
        "energy_per_attempt_j": row["energy_per_attempt_j"],
        "wasted_energy_index_j": row["wasted_energy_index_j"],
        "measurement_tx_energy_j": row["measurement_tx_energy_j"],
        "measurement_rx_energy_j": row["measurement_rx_energy_j"],
        "measurement_idle_energy_j": row["measurement_idle_energy_j"],
        "measurement_sleep_energy_j": row["measurement_sleep_energy_j"],
        "mean_time_on_air_s": sum(values("meanTimeOnAir", ".loRaNodes[")) /
            max(1, len(values("meanTimeOnAir", ".loRaNodes["))),
        "gateway_downlinks_forwarded": row["gateway_downlinks_forwarded"],
        "gateway_downlinks_dropped_dc": row["gateway_downlinks_dropped_dc"],
        "gateway_downlink_drop_rate": row["gateway_downlink_drop_rate"],
        "gateway_observed_duty_cycle": row["gateway_observed_duty_cycle"],
        "below_sensitivity_losses": losses["below_sensitivity"],
        "low_snr_losses": losses["low_snr"],
        "collision_losses": losses["collisions"],
        "other_phy_losses": losses["other_phy"],
        "below_sensitivity_rate": losses["below_sensitivity"] / sent if sent else 0.0,
        "low_snr_loss_rate": losses["low_snr"] / sent if sent else 0.0,
        "collision_loss_rate": losses["collisions"] / sent if sent else 0.0,
        "mean_sf_changes_per_node": row["mean_sf_changes_per_node"],
        "mean_tp_changes_per_node": row["mean_tp_changes_per_node"],
        "mobile_trajectories": row["mobile_trajectories"],
        "static_trajectories": row["static_trajectories"],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scalar_files", nargs="+", type=Path)
    parser.add_argument("--output", "-o", type=Path)
    parser.add_argument("--jobs", type=int, default=1,
                        help="number of scalar files to process concurrently")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be a positive integer")
    if args.jobs == 1:
        rows = [extract(path) for path in args.scalar_files]
    else:
        with ProcessPoolExecutor(max_workers=args.jobs) as executor:
            rows = list(executor.map(extract, args.scalar_files, chunksize=1))
    destination = args.output.open("w", newline="", encoding="utf-8") if args.output else sys.stdout
    try:
        writer = csv.DictWriter(destination, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if args.output:
            destination.close()


if __name__ == "__main__":
    main()
