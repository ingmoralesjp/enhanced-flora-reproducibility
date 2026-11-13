# Scope and provenance

The primary dataset has 900 accepted Enhanced executions: six cumulative
stages, three populations, ten densities and five repetitions per cell.
There are 180 cells and 150 complete six-stage blocks. Pair by `pair_id`,
population, device count and repetition, not by a raw seed value alone.

The official reference has 100 attempts, 92 accepted and eight failed.
It remains separate from the Enhanced pairs. Means among completed official
runs are conditional on completion; failed attempts have no imputed results.

F0 is an internal control within Enhanced. F1 changes noise; F2 adds the SNIR
gate; F3 changes physical-frame airtime; F4 changes gateway scheduling; F5
changes the power/current selector. Common changes, including propagation,
MAC/application behavior and measurement, are outside those five switches.
Their effects cannot be isolated by interpreting F5 minus F0 as upstream minus
Enhanced.

The archived receiver threshold and MAC progress limitations are retained.
Four archived identity/MAC tests fail; corrected-candidate passes and explicitly
initialized receiver controls are separate versions, not new labels for the
900 executions. Numerical conformance does not imply hardware/channel
calibration or a fully corrected LoRaWAN implementation.

Delivery and end-device energy cover days 2–12 of a 12-day run. Historical
airtime/gateway counters cover all 12 days. Queue-pending packets are not PHY
losses, inactive counters do not prove absence of losses, and mean-of-node
airtime is not transmission-weighted airtime. Instrumented probes retain their
reported residuals. No complete historical event ledger is invented.

Historical traffic/mobility/channel draws share RNG0. Seed matching does not
guarantee identical exogenous trajectories after event sequences diverge.
Pointwise five-pair Student-t intervals are not multiplicity-adjusted tests.
The separate ten-repetition sensitivity design does not enlarge the original
sample size or establish universal warm-up/precision sufficiency.

Only one gateway, up to 1000 end devices and at most ten repetitions are in
scope. No experimental hardware validation or multi-gateway claim is made.
The historical binary hash is not a full source-tree hash. Original complete
compiler logs and a complete original build-input manifest were not recovered.
