# Reproduction levels

## 1. Compact data and numerical tables

Run `python3 tools/verify.py`, then
`python3 tools/reproduce_tables.py --output ../new-results`.
Only Python >=3.10 is required. Device sums, counts, pairing, original export
hashes and separate official acceptance are checked. New files are written to
a new directory; archived values are never overwritten.

Stage summaries use five accepted runs per cell. The paired output has five
adjacent and five cumulative contrasts per population/density: 300 rows.
PDR is in percentage points, energy per delivery in mJ, energy per device in J,
and historical mean-of-node airtime in ms. Ratios are computed within runs
before averaging. All reported intervals are pointwise Student-t intervals
with four degrees of freedom (t=2.7764451051977987).

## 2. Numerical propagation verification

Run `python3 tools/verify.py --reanalyze-propagation`. It extracts the original
propagation bundle into a temporary directory and runs its own manifest check
and independent oracle on a disposable copy of the raw outputs. It does not
load the packaged test library or execute a network simulation. The original
bundle also documents how to compile the unchanged production component in a
new directory with matching dependencies.

## 3. Native simulator builds and selected replays

The rc11 source-only redistribution tar is
`archives/enhanced-flora-0.2.0-dev-rc11-source-only.tar.gz`; the browsable
`simulator/` copies retain the same code/configuration bytes. Its packaging
hash differs from the original because manuscript directories were omitted.
See `PUBLICATION_FILTER.json` for the mapping and unchanged source-file hashes.
Native replays require matching OMNeT++
6.0pre11, INET 4.3.2 and Clang 10 dependencies. Do not use the old absolute
Makefile paths as installation instructions. Extract the frozen tar to a new
directory and follow its `docs/installation.md` and rerunner instructions.

The larger native reproduction compendium is an additional release asset,
not a file that Git silently fetches. Its state and SHA-256 are listed in
`release-assets.json`. When that asset is actually published, its portable
`reproduce/` commands build clean dependencies and run six selected 12-day
controls, followed by separate candidate/component checks. Until a verified
URL is populated, availability of that release asset remains pending.

Re-extracting all historical scalars or complete packet ledgers requires the
separate raw archives identified in the supplied manifests. Compact CSVs,
six replays and component checks do not stand in for every raw scalar/event.

## 4. Current paper

This checkout regenerates the numerical summaries and contrasts. Manuscript
drafts, paper PDFs and paper LaTeX projects are excluded from the public tree
and its nested archives. Technical verification reports and standalone result
figures document the software and experiments. No article DOI is assigned here.
