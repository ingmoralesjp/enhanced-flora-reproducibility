#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Check publication files and nested archives for excluded manuscript material."""

import io
import json
from pathlib import Path, PurePosixPath
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BLOCKED = {
    "paper", "papers", "manuscript", "manuscripts", "latex",
    "enhanced-flora-platform-paper", "enhanced-flora-publication-report",
    "enhanced-flora-change-report",
}
TECHNICAL_TEX = {
    "evidence/source-comparison/01_CAMBIOS_FLoRa_1_1_A_ENHANCED.tex",
    "archives/P3_propagacion_rc11_verificada_20261108_v1-source-only.zip!propagacion-rc11-v1/REPORT.tex",
}


def inspect_payload(label, data, state, depth=0):
    assert depth <= 6, "Unexpected archive nesting"
    name = label.split("!")[-1]
    parts = PurePosixPath(name).parts
    assert not name.startswith("/") and ".." not in parts
    state["entries_checked"] += 1
    if BLOCKED.intersection(part.lower() for part in parts):
        state["findings"].append(label)
    suffix = PurePosixPath(name).suffix.lower()
    if suffix in {".tex", ".latex"} and b"\\documentclass" in data:
        if label not in TECHNICAL_TEX:
            state["findings"].append(label)
        else:
            state["technical_reports"].append(label)
    if PurePosixPath(name).name.lower() in {"main.pdf", "manuscript.pdf", "paper.pdf"}:
        state["findings"].append(label)
    if suffix == ".pdf":
        state["pdfs_checked"].append(label)
    if name.endswith(".zip"):
        with zipfile.ZipFile(io.BytesIO(data)) as source:
            for info in source.infolist():
                if not info.is_dir():
                    inspect_payload(label + "!" + info.filename, source.read(info), state, depth + 1)
    elif name.endswith((".tar.gz", ".tgz", ".tar")):
        with tarfile.open(fileobj=io.BytesIO(data), mode="r:*") as source:
            for member in source.getmembers():
                if member.isfile():
                    inspect_payload(label + "!" + member.name, source.extractfile(member).read(), state, depth + 1)


def audit(root=ROOT):
    state = {"entries_checked": 0, "findings": [], "technical_reports": [], "pdfs_checked": []}
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root)
        if path.is_file() and ".git" not in relative.parts and "__pycache__" not in relative.parts:
            inspect_payload(relative.as_posix(), path.read_bytes(), state)
    assert not state["findings"], state["findings"]
    return state


if __name__ == "__main__":
    result = audit()
    print(json.dumps({"status": "PASS", "entries_checked": result["entries_checked"],
                      "manuscript_findings": result["findings"],
                      "technical_reports_retained": len(result["technical_reports"]),
                      "technical_or_figure_pdfs_checked": len(result["pdfs_checked"])}))
