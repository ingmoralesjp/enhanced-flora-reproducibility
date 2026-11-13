#!/usr/bin/env python3
"""Build an isolated factorial source/configuration from the verified item-2 tree."""
import argparse
import difflib
import hashlib
import itertools
import json
from pathlib import Path
import shutil


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source-root", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    source, out = args.source_root.resolve(), args.output.resolve()
    if out.exists(): p.error("Use a new output directory")
    out.mkdir(parents=True)
    shutil.copy2(source / "Makefile", out / "Makefile")
    shutil.copytree(source / "src", out / "src", ignore=shutil.ignore_patterns("*.so", "*.o", "*.d", "out", "Makefile", "__pycache__"))
    shutil.copytree(source / "simulations", out / "simulations")
    patches = []
    def edit(relative, replacements):
        path = out / relative
        before = path.read_text()
        after = before
        for old, new in replacements:
            if after.count(old) != 1:
                raise RuntimeError(f"Patch is not unique in {relative}: {old[:60]}")
            after = after.replace(old, new, 1)
        path.write_text(after)
        patches.extend(difflib.unified_diff(before.splitlines(True), after.splitlines(True), fromfile=relative, tofile=relative))

    edit("src/AuditTrace.cc", [
        ('    std::string run;\n', '    std::string run;\n    bool adrOnly = false;\n'),
        ('        run = id ? id : "unspecified";', '        run = id ? id : "unspecified";\n        const char *profile = std::getenv("FLORA_AUDIT_PROFILE");\n        adrOnly = profile && std::string(profile) == "adr";'),
        ('    long bytes = -1;\n', '    long bytes = -1;\n    bool downlink = false;\n'),
        ('        const bool up = frame->getReceiverAddress().isBroadcast();',
         '        const bool up = frame->getReceiverAddress().isBroadcast();\n        out.downlink = !up;'),
        ('    if (!w.events.is_open()) return;\n    const auto time = omnetpp::simTime();',
         '    if (!w.events.is_open()) return;\n    if (w.adrOnly) {\n        const std::string name = event;\n        const bool keep = name.find("ADR_") == 0 || name.find("GW_DOWNLINK_") == 0 ||\n            ((name == "PHY_TX_START" || name == "PHY_TX_END") && std::string(component->getFullPath()).find(".loRaGW[") != std::string::npos) ||\n            ((name == "ED_RX_SUCCESS" || name == "ED_RX_FAILED") && id.downlink);\n        if (!keep) return;\n    }\n    const auto time = omnetpp::simTime();'),
        ('    if (!enabled()) return;\n    auto& w = writer();\n    const auto warmup =',
         '    if (!enabled() || writer().adrOnly) return;\n    auto& w = writer();\n    const auto warmup ='),
    ])
    edit("src/LoRa/LoRaMac.cc", [
        ('#include "AuditTrace.h"', '#include "AuditTrace.h"\n#include "LoRaApp/LoRaAppPacket_m.h"'),
        ('void LoRaMac::finish()\n{',
         'void LoRaMac::finish()\n{\n    long auditPendingMeasurement = 0;\n    auto auditCount = [&](const Packet *packet) {\n        if (!packet) return;\n        const auto header = packet->peekAtFront<LoRaMacFrame>();\n        const auto data = packet->peekDataAt<LoRaAppPacket>(header->getChunkLength());\n        if (data->getOptions().getSentAt() >= getSimulation()->getWarmupPeriod())\n            auditPendingMeasurement++;\n    };\n    auditCount(currentTxFrame);\n    for (int i = 0; i < txQueue->getNumPackets(); ++i) auditCount(txQueue->getPacket(i));\n    recordScalar("auditPendingQueuePackets", txQueue->getNumPackets());\n    recordScalar("auditPendingCurrentFrame", currentTxFrame ? 1 : 0);\n    recordScalar("auditPendingMeasurementPackets", auditPendingMeasurement);\n    recordScalar("auditFinalMacState", fsm.getState());'),
    ])
    edit("src/LoRaPhy/LoRaTransmitter.h", [
        ('    mutable long transmissionCount = 0;',
         '    mutable long transmissionCount = 0;\n    mutable long auditMeasurementTxCount = 0;\n    mutable omnetpp::simtime_t auditMeasurementAirtime;'),
    ])
    edit("src/LoRaPhy/LoRaTransmitter.cc", [
        ('    transmissionCount++;',
         '    if (startTime >= getSimulation()->getWarmupPeriod()) {\n        auditMeasurementTxCount++;\n        auditMeasurementAirtime += airtime.total();\n    }\n    transmissionCount++;'),
        ('void LoRaTransmitter::finish()\n{',
         'void LoRaTransmitter::finish()\n{\n    recordScalar("auditMeasurementTxCount", auditMeasurementTxCount);\n    recordScalar("auditMeasurementAirtime", auditMeasurementAirtime.dbl(), "s");'),
    ])

    common = '''# Item-3 factorial; original item-2 source is retained separately.
include fidelity-ablation.ini

[Config AuditFactorialBase]
extends = FidelityF5EnhancedStatic
seed-set = ${repetition}
num-rngs = 4
**.loRaNodes[*].SimpleLoRaApp.rng-0 = 0
**.loRaNodes[*].SimpleLoRaApp.rng-1 = 3
**.LoRaMedium.pathLoss.rng-0 = 1
**.ipv4Delayer.rng-0 = 2
**.timeToFirstPacket = exponential(1000s, 1)
**.timeToNextPacket = exponential(1000s, 1)
**.loRaNodes[*].mobile = false
**.clientAdrMode = "network"
**.backgroundNoiseModel = "thermal"
**.enforceRequiredSnr = true
**.txCurrentLookupModel = "dbmNearest"
**.heartbeatInterval = 0s
**.useAllEu868Channels = false
**.gatewayDutyCycle = 0.01
'''
    combinations = []
    for a, g, d in itertools.product((0, 1), repeat=3):
        name = f"AuditA{a}G{g}D{d}"
        combinations.append({"config": name, "airtime": a, "gateway": g, "adaptation": d})
        common += f'''\n[Config {name}]
extends = AuditFactorialBase
**.airtimeModel = "{'semtech' if a else 'legacyFixedPayload'}"
**.dutyCycleModel = "{'toa' if g else 'legacySfConstants'}"
**.networkServer.**.evaluateADRinServer = {'true' if d else 'false'}
**.evaluateADRinNode = {'true' if d else 'false'}
'''
    (out / "simulations/examples/reviewer-factorial.ini").write_text(common)
    (out / "factorial.patch").write_text("".join(patches))
    files = sorted(x for x in out.rglob("*") if x.is_file() and x.suffix in (".cc", ".h", ".ned", ".msg", ".ini", ".xml"))
    manifest = {"parent_instrumentation_manifest_sha256": sha(source / "instrumentation-manifest.json"),
        "combinations": combinations, "nodes": [100, 500, 1000], "repetitions": 5,
        "seeds": [0, 1, 2, 3, 4], "total_executions": 120,
        "horizon_seconds": 1036800, "warmup_seconds": 172800,
        "source_sha256": {str(x.relative_to(out)): sha(x) for x in files},
        "patch_sha256": sha(out / "factorial.patch"),
        "source_behavior_note": "Original MAC timeout path retained; only factors and passive metrics are varied"}
    (out / "factorial-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"status": "PREPARED", "combinations": 8, "planned_runs": 120, "output": str(out)}))


if __name__ == "__main__":
    main()
