# Observation 1 — provenance and missing-evidence ledger

This ledger separates what can be verified from the existing 900-run campaign
from what cannot be reconstructed after the fact. It is part of the handoff,
not an assertion that every request in observation 1 has been satisfied.

## Identity that can be checked

The frozen release is `enhanced-flora-0.2.0-dev-rc11.tar.gz` (SHA-256
`544414b2c6d0f9a3954154f4bb06289b646c0f3a09326f3d0d50953ea9cb6bd0`).
Its own `MANIFEST.sha256` checks every packaged file. The included historical
`experiments/enhanced-flora-platform/results/fidelity-ablation-corrected-v0.2.0-dev/state/campaign-manifest.json`
is SHA-256 `1bab8d90da8df01458c65a939a7f2c5ac6d769b54185272bcb471848f774dc3a`.
The latter records the executed `libflora.so` SHA-256
`a83fbfd8ee4c9557fd1b0491fb390ceaf880eeed1bd7cbd4d95108cee4bbfcb5`,
runner hash and a **selected** source hash set. The set includes the entry
ablation INIs and the MAC/radio sources; it is not an original build-time
hash of every `.cc`, `.h`, `.ned`, and XML file. A release manifest generated
afterward cannot retrospectively prove that every packaged byte built the
historical library. The executed binary hash is stronger identity evidence
than a fresh compilation, which is not expected to reproduce binary bytes
across environments.

`docs/installation.md` in the release records OMNeT++ 6.0pre11 build
`210423-26668997ef`, INET 4.3.2 local identifier
`inet-4.3.2-23d74062fc`, Clang 10 release, commands, and binary/library
hashes. The full INET source is an external build dependency, not included in
the FLoRa tar. Two inherited NED defaults that matter here are from the
pinned local INET tree: `CsmaCaMac.ned` SHA-256
`94ce1e6fe045a10095ae99a7aa569707067e3e033d846421285d4d8786a41c19`
and `RandomWaypointMobility.ned` SHA-256
`f70ebe256510d0ba69a11a7b5306b37c322f826ee80af22c3a2ea685fc95dba1`.
Their exact files are included as dependency snapshots in the observation-1
bundle; they do not replace installing/building the pinned INET dependency.

The corrected-release verifier passed on a fresh extraction, reporting 1,134
release files, 900 accepted Enhanced runs and a **separate** official
reference with 92 accepted and eight failed attempts. These are integrity
and coverage checks, not numerical PHY validation. The portable rerunner's
one-job dry run passed; it did not execute a new simulation.

## Existing tests versus missing tests

| Evidence requested | Located in the frozen release | Interpretation |
| --- | --- | --- |
| MAC one-send regression | `tests/run_mac_one_send_regression.py`, `tests/0001-assert-one-mac-send-per-event.patch`, `tests/evidence/mac-one-send-rc4.json` | Ten recorded short paths; supports that narrow MAC invariant. |
| F0--F5 configuration and short-run validator | `analysis/validate_fidelity_ablation_testbed.py` | Script exists; its corrected-source execution receipts are not archived here. |
| Independent formula/component vectors and numeric output for F1--F5 | Not located in the frozen release | Cannot claim the manuscript's independent numerical tests are reproduced by this package. |
| Original compile stdout/stderr log | Not located in the frozen release | `docs/installation.md` provides commands and hashes, but is not the original build log. |
| Warm-up stabilization / repetition precision analysis | Not located in the frozen release | The 2-day warm-up and five repetitions are configuration facts, not justified choices here. |

Do not substitute older `0.1.0-dev` test receipts for the corrected
`0.2.0-dev` campaign. Do not turn a new test run on a rebuilt library into a
historical test receipt. If new checks are later run, version their source,
command, result, tolerance, timestamp, and environment separately.

## Consequence for the numbered workflow

Observation 1 has an inspectable source/configuration archive and an honest
gap ledger, but its requested original formula/component outputs and original
compiler log are **not fully delivered**. The next numbered request should
not begin merely because this package exists. First either recover the
historical evidence, create clearly labeled new validation on the frozen
sources and reconcile the manuscript wording, or explicitly accept the
remaining evidence limits for the submission. That decision belongs within
the verification of observation 1.
