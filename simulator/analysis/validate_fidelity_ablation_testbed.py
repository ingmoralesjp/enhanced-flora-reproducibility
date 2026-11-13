#!/usr/bin/env python3
"""Run and audit the six cumulative FLoRa fidelity-ablation paths."""

import argparse
import csv
import subprocess
import sys
from pathlib import Path


FLORA = Path(__file__).resolve().parents[1]
OMNET = FLORA.parents[1]
SAMPLES = FLORA.parent
EXAMPLES = FLORA / "simulations" / "examples"
ROOT = FLORA / "simulations" / "results" / "fidelity-ablation-20260909"
VALIDATION = ROOT / "validation"
INI = EXAMPLES / "fidelity-ablation.ini"

EXPECTED = {
    "FidelityValidateF0": (0, 0, 0, 0, 0),
    "FidelityValidateF1": (1, 0, 0, 0, 0),
    "FidelityValidateF2": (1, 1, 0, 0, 0),
    "FidelityValidateF3": (1, 1, 1, 0, 0),
    "FidelityValidateF4": (1, 1, 1, 1, 0),
    "FidelityValidateF5": (1, 1, 1, 1, 1),
}
DUTY_CONFIGS = {
    "FidelityValidateLegacyDuty": "duty-legacy",
    "FidelityValidateToaDuty": "duty-toa",
}
POPULATION_CONFIGS = {
    "FidelityValidateF5Mobile": ("f5-mobile", 12, 0),
    "FidelityValidateF5Mixed": ("f5-mixed", 6, 6),
}


def base_command():
    return [
        str(OMNET / "bin" / "opp_run"),
        "-n", f"{FLORA/'src'}:{FLORA/'simulations'}:{SAMPLES/'inet'/'src'}",
        "-l", str(FLORA / "src" / "flora"),
        "-l", str(SAMPLES / "inet" / "src" / "INET"),
    ]


def run_all():
    VALIDATION.mkdir(parents=True, exist_ok=True)
    for index, config in enumerate(EXPECTED):
        log_path = VALIDATION / f"f{index}.log"
        command = base_command() + [
            "-u", "Cmdenv", "-c", config, "-r", "0", str(INI)
        ]
        with log_path.open("w", encoding="utf-8") as log:
            completed = subprocess.run(
                command, cwd=EXAMPLES, stdout=log, stderr=subprocess.STDOUT)
        if completed.returncode:
            raise RuntimeError(
                f"{config} failed with exit code {completed.returncode}; see {log_path}")
    for config, stem in DUTY_CONFIGS.items():
        log_path = VALIDATION / f"{stem}.log"
        command = base_command() + [
            "-u", "Cmdenv", "-c", config, "-r", "0", str(INI)
        ]
        with log_path.open("w", encoding="utf-8") as log:
            completed = subprocess.run(
                command, cwd=EXAMPLES, stdout=log, stderr=subprocess.STDOUT)
        if completed.returncode:
            raise RuntimeError(
                f"{config} failed with exit code {completed.returncode}; see {log_path}")
    for config, (stem, _, _) in POPULATION_CONFIGS.items():
        log_path = VALIDATION / f"{stem}.log"
        command = base_command() + [
            "-u", "Cmdenv", "-c", config, "-r", "0", str(INI)
        ]
        with log_path.open("w", encoding="utf-8") as log:
            completed = subprocess.run(
                command, cwd=EXAMPLES, stdout=log, stderr=subprocess.STDOUT)
        if completed.returncode:
            raise RuntimeError(
                f"{config} failed with exit code {completed.returncode}; see {log_path}")


def parse_scalar(path):
    attributes = {}
    parameters = {}
    scalar_values = {}
    mobile = []
    channels = []
    with path.open(encoding="utf-8", errors="replace") as source:
        for raw in source:
            line = raw.rstrip("\n")
            if line.startswith("attr "):
                parts = line.split(maxsplit=2)
                if len(parts) == 3:
                    attributes[parts[1]] = parts[2]
            elif line.startswith("par "):
                parts = line.split(maxsplit=3)
                if len(parts) != 4:
                    continue
                module, name, value = parts[1:]
                parameters[(module, name)] = value
                if name == "mobile" and ".loRaNodes[" in module:
                    mobile.append(value)
                if name == "useAllEu868Channels" and module.endswith(".SimpleLoRaApp"):
                    channels.append(value)
            elif line.startswith("scalar "):
                for name in (
                    "backgroundNoiseModelCode", "requiredSnrGateEnabled",
                    "airtimeModelCode", "GW_dutyCycleModelCode",
                    "txCurrentLookupModelCode", "adrAlgorithmCode", "adrStepSizeDb",
                    "measurementEnergyConsumed", "finalPowerConsumption",
                    "GW_forwardedDown", "GW_droppedDC", "GW_downlinkTimeOnAir",
                    "GW_dutyCycleHoldoff",
                ):
                    marker = f" {name} "
                    if marker in line:
                        scalar_values.setdefault(name, set()).add(
                            float(line.rsplit(maxsplit=1)[1]))
    return attributes, parameters, scalar_values, mobile, channels


def one_value(values, name):
    found = values.get(name, set())
    return next(iter(found)) if len(found) == 1 else None


def audit():
    rows = []
    failures = []
    for index, (config, expected) in enumerate(EXPECTED.items()):
        path = VALIDATION / f"f{index}.sca"
        if not path.exists():
            failures.append(f"{path.name}: missing")
            continue
        attrs, parameters, values, mobile, channels = parse_scalar(path)
        raw_text = path.read_text(encoding="utf-8", errors="replace")
        raw_lines = raw_text.splitlines()
        def has_scalar_unit(name, unit):
            return any(f" {name} " in line and following.strip() == f"attr unit {unit}"
                       for line, following in zip(raw_lines, raw_lines[1:]))
        noise, snr, toa, duty, current = expected
        server = "LoRaNetworkTest.networkServer.app[0]"
        checks = {
            "configuration": attrs.get("configname") == config,
            "identity": (attrs.get("runnumber"), attrs.get("repetition"),
                         attrs.get("seedset")) == ("0", "0", "0"),
            "nodes": parameters.get(("LoRaNetworkTest", "numberOfNodes")) == "12",
            "reference_adr": one_value(values, "adrAlgorithmCode") == 1,
            "ttn_step": one_value(values, "adrStepSizeDb") == 2.5,
            "noise": one_value(values, "backgroundNoiseModelCode") == noise,
            "snr_gate": one_value(values, "requiredSnrGateEnabled") == snr,
            "toa": one_value(values, "airtimeModelCode") == toa,
            "duty": one_value(values, "GW_dutyCycleModelCode") == duty,
            "current": one_value(values, "txCurrentLookupModelCode") == current,
            "three_channels": len(channels) == 12 and set(channels) == {"false"},
            "static": len(mobile) == 12 and set(mobile) == {"false"},
            "energy": bool(values.get("measurementEnergyConsumed")),
            "power": bool(values.get("finalPowerConsumption")),
            "energy_unit": has_scalar_unit("measurementEnergyConsumed", "J"),
            "power_unit": has_scalar_unit("finalPowerConsumption", "W"),
        }
        failed = [name for name, passed in checks.items() if not passed]
        if failed:
            failures.append(f"{path.name}: {', '.join(failed)}")
        rows.append({
            "stage": f"F{index}",
            "configuration": config,
            "noise_code": one_value(values, "backgroundNoiseModelCode"),
            "snr_gate_code": one_value(values, "requiredSnrGateEnabled"),
            "toa_code": one_value(values, "airtimeModelCode"),
            "duty_code": one_value(values, "GW_dutyCycleModelCode"),
            "tx_current_code": one_value(values, "txCurrentLookupModelCode"),
            "adr_step_db": one_value(values, "adrStepSizeDb"),
            "status": "FAIL" if failed else "PASS",
        })

    for config, stem in DUTY_CONFIGS.items():
        path = VALIDATION / f"{stem}.sca"
        if not path.exists():
            failures.append(f"{path.name}: missing")
            continue
        attrs, _, values, _, _ = parse_scalar(path)
        forwarded = one_value(values, "GW_forwardedDown")
        toa = one_value(values, "GW_downlinkTimeOnAir")
        holdoff = one_value(values, "GW_dutyCycleHoldoff")
        expected_code = 0 if stem == "duty-legacy" else 1
        checks = {
            "configuration": attrs.get("configname") == config,
            "duty_code": one_value(values, "GW_dutyCycleModelCode") == expected_code,
            "forwarded": forwarded is not None and forwarded > 0,
            "holdoff": holdoff is not None and holdoff > 0,
            "toa": toa is not None and toa > 0,
        }
        if stem == "duty-toa":
            checks["one_percent"] = abs(toa / holdoff - 0.01) < 1e-12
        failed = [name for name, passed in checks.items() if not passed]
        if failed:
            failures.append(f"{path.name}: {', '.join(failed)}")

    for config, (stem, expected_mobile, expected_static) in POPULATION_CONFIGS.items():
        path = VALIDATION / f"{stem}.sca"
        if not path.exists():
            failures.append(f"{path.name}: missing")
            continue
        attrs, _, values, mobile, channels = parse_scalar(path)
        checks = {
            "configuration": attrs.get("configname") == config,
            "population": (mobile.count("true"), mobile.count("false")) ==
                (expected_mobile, expected_static),
            "three_channels": len(channels) == 12 and set(channels) == {"false"},
            "reference_adr": one_value(values, "adrAlgorithmCode") == 1,
            "ttn_step": one_value(values, "adrStepSizeDb") == 2.5,
        }
        failed = [name for name, passed in checks.items() if not passed]
        if failed:
            failures.append(f"{path.name}: {', '.join(failed)}")

    VALIDATION.mkdir(parents=True, exist_ok=True)
    report_csv = VALIDATION / "fidelity-testbed-validation.csv"
    if rows:
        with report_csv.open("w", newline="", encoding="utf-8") as destination:
            writer = csv.DictWriter(destination, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    report = VALIDATION / "fidelity-testbed-validation.txt"
    if failures:
        text = "FIDELITY ABLATION VALIDATION FAILED\n" + "\n".join(failures) + "\n"
        code = 2
    else:
        text = (
            "FIDELITY ABLATION VALIDATION PASSED\n"
            "Cumulative treatments: 6/6\n"
            "Duty-cycle behavior: recovered fixed waits and actual-ToA 1% paths exercised\n"
            "Populations: static 0/12, mobile 12/0, mixed 6/6\n"
            "Reference: lorawanReference, NStep divisor 2.5 dB, three EU868 channels\n"
            "Common population: 12 stationary devices\n"
            "Energy contract: post-warm-up joules plus instantaneous final watts present\n"
        )
        code = 0
    report.write_text(text, encoding="utf-8")
    print(text, end="")
    print(f"CSV: {report_csv}")
    return code


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--run", action="store_true", help="execute all short paths before audit")
    args = parser.parse_args()
    try:
        if args.run:
            run_all()
        return audit()
    except RuntimeError as error:
        print(f"FIDELITY ABLATION VALIDATION FAILED\n{error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
