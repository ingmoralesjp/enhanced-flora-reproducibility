# Enhanced FLoRa — reproducibility artifact

Research artifact for **Cross-Layer Changes in FLoRa: Implementation and
Effects on LoRaWAN Simulation Outcomes**.

This is a research extension of FLoRa, **not an official FLoRa release**.
The reference is FLoRa v1.1.0, commit
`51018f80fdafc562d8f9fa022e551f835c8969ee`, by Michał Słabicki and contributors.

## What is here

| Directory | Material |
| --- | --- |
| `simulator/` | Unchanged source/configuration files copied from the frozen Enhanced rc11 archive, with the original notices. |
| `archives/` | Source-only redistribution of rc11, original component tests and a manuscript-filtered propagation bundle. |
| `data/historical/` | 900 Enhanced runs, 900 gateway records, 495,000 device records, and 100 separate official attempts, with dictionaries/exporters. |
| `data/mechanisms/` | Additional 900-run mechanism export and its policy/measurement definitions. |
| `evidence/` | Source comparison, component results, configuration, energy, current lookup, RNG/precision, packet and gateway diagnostics, separated by version. |
| `tools/` | Portable integrity checks and independent regeneration of 180 summaries and 300 paired contrasts. |
| `docs/` | Scope, reproduction instructions, dependencies, limitations, and publication instructions. |

The archived implementation and corrected candidates are **not** combined.
F0 is the internal Enhanced control, not unmodified official FLoRa 1.1.
All historical Enhanced populations use network-mode `lorawanReference`,
not a BADR-versus-legacy comparison. Official failed attempts have no imputed
PDR or energy. See `docs/SCOPE.md` before interpreting the outputs.

## Start here — no simulator required

Python 3.10 or newer; these commands use only the standard library:

```sh
python3 tools/verify.py
python3 tools/reproduce_tables.py --output ../enhanced-recomputed-results
python3 tools/verify.py --reanalyze-propagation
```

Choose a new output directory: regeneration refuses an existing destination.
These commands do not modify archived data or run network simulations.
Verification checks integrity and reported accounting, **not universal model
validity**. Four archived MAC/identity test failures remain documented;
candidate passes are a separate result.

## Building or rerunning simulations

See `docs/REPRODUCING.md`. The native historical stack is OMNeT++ 6.0pre11,
INET 4.3.2 and Clang 10. Exact build identities and separately versioned
portability controls are retained in the evidence. Do not silently substitute
another dependency tree or interpret a version label as a complete source hash.

The source-only archive preserves the production source, configuration and
test bytes from rc11. Manuscript directories were omitted from both the main
archive and its nested copy in the propagation bundle. `PUBLICATION_FILTER.json`
records the original archive identity, redistribution hash, omitted paths and
retained file hashes. The original unfiltered archive is preserved locally.
The much larger native compendium and
raw traces are not silently represented by compact CSVs. Their distribution
state and hashes are recorded in `release-assets.json`. Until release URLs
are verified, this local checkout is **not** a claim that those assets have
been published. Historical compiler logs and a complete historical build-input
manifest were not recovered; new verification does not reconstruct them.

## Licensing and citation

FLoRa-derived software retains **LGPL-3.0-or-later**, subject to existing
per-file notices. `LICENSE.md` contains LGPLv3; `COPYING` contains GPLv3.
OMNeT++, INET and other third-party files retain their own licenses.

The authors' experiment data and newly authored documentation are available
under **CC BY 4.0**, except materials with their own notices. This does not
relicense third-party code, license texts or the manuscript. See `NOTICE.md`
and `data/LICENSE.md`. Cite this artifact using `CITATION.cff`, the associated
paper, and the original FLoRa work.

## Publication state

Public repository: <https://github.com/ingmoralesjp/enhanced-flora-reproducibility>.
Clone it with:

```sh
git clone https://github.com/ingmoralesjp/enhanced-flora-reproducibility.git
cd enhanced-flora-reproducibility
```

The artifact version is `v1.0.0-rc2`; the simulator snapshot is rc11.
Those are distinct identifiers. Check `PUBLICATION_STATUS.json` for the actual
repository state and the GitHub Releases page for versioned downloads.
The large native compendium remains a separately listed, unpublished asset;
no DOI has been minted. This repository and its release archives contain no
research-paper manuscripts or draft-paper projects. Technical verification
reports and standalone result figures remain part of the experiment evidence.
Run `python3 tools/check_publication_contents.py` to check the publication
tree and nested archives for excluded manuscript content.
