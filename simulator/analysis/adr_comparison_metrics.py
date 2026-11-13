#!/usr/bin/env python3
"""Extract paper-compatible ADR metrics from OMNeT++ scalar files.

Energy is taken from the post-warm-up measurement window. Data-packet PDR uses
the network-server counters, while radio energy includes data, heartbeat and
Class-A RX activity exactly as integrated by LoRaEnergyConsumer.
"""

import argparse
import csv
import math
import shlex
import sys
from pathlib import Path


ALGORITHM_NAMES = {0: "disabled", 1: "lorawanReference", 2: "thesisLegacy", 3: "mobilityEnergy"}


def parse_scalar_file(path):
    attributes = {}
    scalars = []
    with path.open(encoding="utf-8", errors="replace") as source:
        for raw_line in source:
            # Scalar files contain hundreds of parameter lines per node.  They
            # are irrelevant to metric extraction, so avoid invoking shlex on
            # them; this materially reduces checkpoint time at 2000--5000 EDs.
            if raw_line.startswith("attr "):
                fields = raw_line.rstrip("\n").split(maxsplit=2)
                if len(fields) >= 3:
                    attributes[fields[1]] = fields[2].strip('"')
            elif raw_line.startswith("scalar "):
                payload = raw_line[7:].rstrip("\n")
                try:
                    module, remainder = payload.split(maxsplit=1)
                    if remainder.startswith('"'):
                        quote_end = remainder.find('"', 1)
                        if quote_end < 0:
                            raise ValueError
                        scalar_name = remainder[1:quote_end]
                        value_text = remainder[quote_end + 1:].strip()
                    else:
                        scalar_name, value_text = remainder.split(maxsplit=1)
                    scalars.append((module, scalar_name, float(value_text)))
                except ValueError:
                    # Retain a conservative parser for unusual quoted names;
                    # ordinary OMNeT++ scalar lines stay on the fast path.
                    fields = shlex.split(raw_line)
                    if len(fields) >= 4:
                        try:
                            scalars.append((fields[1], fields[2], float(fields[3])))
                        except ValueError:
                            pass
            else:
                continue
    return attributes, scalars


def summarize(path):
    attributes, scalars = parse_scalar_file(path)

    def values(name, module_contains=None, name_prefix=False):
        result = []
        for module, scalar_name, value in scalars:
            name_matches = scalar_name.startswith(name) if name_prefix else scalar_name == name
            if name_matches and (module_contains is None or module_contains in module):
                result.append(value)
        return result

    def one(name, default=0.0):
        candidates = values(name, "networkServer.app[0]")
        return candidates[0] if candidates else default

    def node_sum(name):
        return sum(values(name, ".loRaNodes["))

    def node_mean(name):
        candidates = values(name, ".loRaNodes[")
        return sum(candidates) / len(candidates) if candidates else 0.0

    def gateway_sum(name):
        return sum(values(name, ".loRaGW["))

    def server_node_mean(name_prefix):
        candidates = values(name_prefix, "networkServer.app[0]", name_prefix=True)
        return sum(candidates) / len(candidates) if candidates else 0.0

    sent = one("counterOfSentPacketsFromNodes")
    received = one("counterUniqueReceivedPackets")
    pdr = received / sent if sent else 0.0
    total_energy = node_sum("measurementEnergyConsumed")
    algorithm_code = int(round(one("adrAlgorithmCode")))
    displacements = values("displacement", ".loRaNodes[")
    server_classes = values("adaptiveMobilityClass node", "networkServer.app[0]", name_prefix=True)
    gateway_forwarded = gateway_sum("GW_forwardedDown")
    gateway_dropped = gateway_sum("GW_droppedDC")
    gateway_toa = gateway_sum("GW_downlinkTimeOnAir")
    gateway_holdoff = gateway_sum("GW_dutyCycleHoldoff")

    return {
        "file": str(path),
        "configuration": attributes.get("configname", ""),
        "repetition": attributes.get("repetition", ""),
        "run_number": attributes.get("runnumber", ""),
        "seed_set": attributes.get("seedset", ""),
        "algorithm": ALGORITHM_NAMES.get(algorithm_code, f"unknown-{algorithm_code}"),
        "algorithm_code": algorithm_code,
        "nodes": len(values("measurementEnergyConsumed", ".loRaNodes[")),
        "sent_data": int(sent),
        "received_data": int(received),
        "pdr": pdr,
        "measurement_energy_j": total_energy,
        "energy_per_success_j": total_energy / received if received else math.nan,
        "useful_energy_j": pdr * total_energy,
        "energy_per_attempt_j": total_energy / sent if sent else math.nan,
        "wasted_energy_index_j": (total_energy / sent) * (1 - pdr) if sent else math.nan,
        "measurement_tx_energy_j": node_sum("measurementTxEnergyConsumed"),
        "measurement_rx_energy_j": node_sum("measurementRxEnergyConsumed"),
        "measurement_idle_energy_j": node_sum("measurementIdleEnergyConsumed"),
        "measurement_sleep_energy_j": node_sum("measurementSleepEnergyConsumed"),
        "heartbeats": int(node_sum("heartbeats")),
        "heartbeat_interval_changes": int(node_sum("heartbeatIntervalChanges")),
        "heartbeat_resets_by_data": int(node_sum("heartbeatResetsByData")),
        "adr_commands_received": int(node_sum("receivedADRCommands")),
        "gateway_downlinks_forwarded": int(gateway_forwarded),
        "gateway_downlinks_dropped_dc": int(gateway_dropped),
        "gateway_downlink_drop_rate": gateway_dropped /
            (gateway_forwarded + gateway_dropped) if gateway_forwarded + gateway_dropped else 0.0,
        "gateway_downlink_toa_s": gateway_toa,
        "gateway_duty_holdoff_s": gateway_holdoff,
        "gateway_observed_duty_cycle": gateway_toa / gateway_holdoff if gateway_holdoff else 0.0,
        "mean_confirmed_deteriorations_per_node": server_node_mean("adaptiveDeteriorationEvents node"),
        "mean_sf_changes_per_node": node_mean("SFChanges"),
        "mean_tp_changes_per_node": node_mean("TPChanges"),
        "mobile_trajectories": sum(value > 0 for value in displacements),
        "static_trajectories": sum(value == 0 for value in displacements),
        "server_static_classes": sum(int(value) == 1 for value in server_classes),
        "server_mobile_classes": sum(int(value) == 2 for value in server_classes),
        "server_unknown_classes": sum(int(value) == 0 for value in server_classes),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scalar_files", nargs="+", type=Path)
    parser.add_argument("--output", "-o", type=Path)
    args = parser.parse_args()

    rows = [summarize(path) for path in args.scalar_files]
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
