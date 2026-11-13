# Reviewer item 2 — packet and energy trajectories

Status: nine probes launched on 2026-10-29, with up to eight concurrent jobs.
F2, F3 and F4 each use 100, 500 and 1000 static EDs, repetition 0, one GW,
12-day horizon and two-day warm-up. The source/configuration behavior is
preserved and the passive additions are documented in `EVENT_DICTIONARY.md`.

`results/equivalence-v2/status.json` reports exact scalar equality for all ten
existing short configurations against the historical campaign library, with
tracing off and on. All six 100- and 500-ED full-horizon probes also pass
packet, downlink and energy reconciliation in `results/analysis-v2/`.
The initial 100-ED analyses remain in `analysis-v1` for continuity. Version 2
also reports terminal queue ages and last observed MAC admission states.

## Reproduction

Extract the frozen source supplied with item 1 and create a new testbed:

```sh
python3 -B prepare_instrumentation.py --source-root /path/to/extracted/rc11 \
  --output /path/to/new/testbed/flora
ln -s /path/to/built/inet /path/to/new/testbed/inet
```

Initialize OMNeT++ 6.0pre11 (`. ./setenv -f` in a non-interactive shell), then
run `make makefiles` and `make MODE=release -j4` from the new FLoRa directory.
The original library hash and the compiled instrumented library hash are
recorded separately. Use `run_probes.py equivalence` before `campaign`; the
campaign refuses to run if that receipt is absent or its identities differ.
The CLI `--help` lists required locations. Use a new output folder for each
identity; `--resume` verifies existing accepted outputs by hash.

Accepted runs have `accepted.json`, `run.sca`, `run.log`, `events.csv` and
`events.csv.energy.csv`. Only complete runs with the configured termination
marker are accepted. The first analysis is independently checkable:

```sh
python3 -B analyze_traces.py /path/to/accepted/run/trace-on \
  --output /path/to/new/analysis
```

The analyzer emits packet, ADR and energy ledgers and `summary.json`, including
residues and boundary-pending counts. Original traces and source are retained.
Source variants and unsuccessful preparation attempts have distinct folders;
the tested campaign source is `testbed/flora-v2`.

Item 3 starts after the nine accepted probes have been analyzed, the delivery
verified and its findings reviewed. A simulation-completion marker alone does
not close item 2.
