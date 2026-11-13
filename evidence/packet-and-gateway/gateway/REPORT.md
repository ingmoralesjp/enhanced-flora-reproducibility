# Observation 5 — actual gateway occupancy

All four controlled executions and all 120 reused factorial traces pass admission/RF reconciliation and the declared scheduling checks. The RF union is measured directly; no PDR or energy value is imputed from it.

| Load/header | Requests | Accepted | Rejected | RF occupancy (%) | Scheduled ToA/holdoff (%) | Actual RF ToA/holdoff (%) | Spacing violations |
|---|---:|---:|---:|---:|---:|---:|---:|
| GatewayHighExplicit | 5800 | 138 | 5662 | 0.954445 | 1.000000 | 1.000000 | 0 |
| GatewayHighImplicit | 5800 | 138 | 5662 | 0.860237 | 1.000000 | 0.901295 | 0 |
| GatewayLowExplicit | 7 | 7 | 0 | 0.165367 | 1.000000 | 1.000000 | 0 |
| GatewayLowImplicit | 7 | 7 | 0 | 0.147072 | 1.000000 | 0.889365 | 0 |

Budget: 1% configured globally for one gateway. Controlled measurement window: seconds 600–3600 (3000 s). Low-load interval: 400 s; high-load interval: 0.5 s. Requests stop at 3500 s, so the observed-window rate differs from the active injection rate.

With an explicit header, high-load occupancy is 0.954445%, whereas the scheduled airtime/holdoff ratio is 1%. With an implicit header, identical admission decisions give lower occupancy, 0.860237%, because the scheduler retains its explicit-header duration assumption. Low-load occupancy is 0.165367% and 0.147072% respectively. Thus the ratio is not measured occupancy.

The corrected G1 factorial cells show zero actual-RF 1% start-spacing violations. Corrected-airtime G1/D1 traces contain 23104 all-horizon scheduled/RF duration mismatches. Legacy G0/D1 has 2194 (A0) and 1039 (A1) adjacent-start violations against an actual-RF 1% budget across the 15 runs of each treatment, despite small aggregate occupancy. Those are expected failures of the legacy budget approximation, not failed simulation runs.

The observed correction is conservative for implicit-header frames, not an overspending of the gateway budget. It remains a real mismatch of scheduler and physical assumptions and limits wording such as “identical actual-frame airtime”. No sources or results from previous items have been changed.

Frequency alternation does not create independent duty resources: the native MAC has one waiting flag and release timer. RF interval union, schedule admission and start-spacing checks are reported separately. See README.md and per-run CSVs for units, scope, clipping and counter-window definitions.
