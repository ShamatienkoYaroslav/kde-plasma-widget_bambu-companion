## Why

Phase 1 made the widget read-only: it shows live printer status but can't act
on it. Phase 2 of `PLAN.md` adds print control — pause, resume, stop, and
skip-objects — over the same LAN MQTT connection, with the `PrinterConnection`
abstraction extended so Phase 3's Cloud transport can implement the same
`sendCommand()` contract later without changes here.

## What Changes

- Add `sendCommand()` to the `PrinterConnection` abstract interface (absent in
  Phase 1 by design) and implement it in `LanPrinterConnection`: publishes to
  `device/{serial}/request`, correlates the echoed `sequence_id` on the
  `.../report` topic back to the originating command, and reports
  success/failure via a new `commandAcked` signal.
- Add `PrinterCommand` (pause/resume/stop/skip-objects, with skip-objects
  carrying an object-id list) and extend `BambuCommandBuilder` with
  `pause()`/`resume()`/`stop()`/`skipObjects(ids)` payload builders.
- Expose `pause(printerId)`, `resume(printerId)`, `stop(printerId)`, and
  `skipObjects(printerId, ids)` on `PrinterController` for the UI.
- Add UI: pause/resume act immediately; `stop` requires confirmation via a new
  `ConfirmActionDialog.qml` (destructive-action guardrail from `PLAN.md`);
  skip-objects uses a manual "enter object IDs" fallback rather than reading
  the plate/object list from the local report (not reliably present there).
- **No Bambu Cloud, no plate-preview thumbnails, no camera** — still LAN-only,
  per `PLAN.md`'s phase boundaries.

## Capabilities

### New Capabilities
- `printer-control`: sending pause/resume/stop/skip-objects commands to a
  connected printer, correlating command success/failure back to the UI, and
  requiring explicit confirmation before the destructive `stop` command.

### Modified Capabilities
(none — printer-management and printer-status-monitoring are unaffected;
`sendCommand()` is a new interface member, not a change to their existing
requirements)

## Impact

- Affected code: `src/core/PrinterConnection.h` (new interface member),
  `src/core/PrinterCommand.h` (new), `src/protocol/BambuCommandBuilder`
  (extended), `src/transport/lan/LanPrinterConnection` (implements
  `sendCommand()` + ack correlation), `src/qmlplugin/PrinterController`
  (new invokables), `package/contents/ui/` (`ConfirmActionDialog.qml`,
  `PrinterDetailView.qml` wiring for the new action buttons).
- No new external dependencies.
