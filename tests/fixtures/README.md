# Fixtures

Synthetic Bambu Lab MQTT `report` payloads, shaped after the
community-documented (reverse-engineered) schema cited in `PLAN.md` and
`design.md` (`Doridian/OpenBambuAPI`). Bambu Lab does not publish this
schema officially, and it can vary by printer model/firmware, so these are
best-effort and should be cross-checked against a real printer's `report`
topic output during manual verification (see `PLAN.md`'s Phase 1 verification
step) rather than treated as authoritative.

- `full_report.json` — a full status report, as sent on connect / after a
  `pushall` request.
- `delta_report.json` — a partial (delta) report containing only a changed
  nozzle temperature, as Bambu printers stream after the initial full report.
