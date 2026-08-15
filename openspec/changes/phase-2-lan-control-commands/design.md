## Context

See `proposal.md` for motivation. Phase 1 (archived) established
`PrinterConnection`/`LanPrinterConnection`/`PrinterRegistry`/`PrinterController`
and live status monitoring, but deliberately left `PrinterConnection` without
a `sendCommand()` method. This design covers adding that method and the
sequence-id-based acknowledgement flow it needs.

## Goals / Non-Goals

**Goals:**
- Pause/resume/stop/skip-objects all go through one `sendCommand()` path on
  `PrinterConnection`, so Phase 3's `CloudPrinterConnection` can implement the
  same contract later without touching `PrinterRegistry`/`PrinterController`/UI.
- Every sent command eventually resolves to success or failure from the UI's
  perspective — never silently hangs forever.

**Non-Goals:**
- No attempt to discover the current print's actual plate/object list for a
  richer skip-objects picker — manual ID entry only, per `PLAN.md`.
- No retry logic for failed/timed-out commands — the user can just retry the
  action themselves; automatic retry of a `stop` command in particular would
  be actively unsafe to guess at.

## Decisions

**Sequence-id generation and correlation**: `LanPrinterConnection` keeps a
private monotonically-increasing counter (`quint64 m_nextSequenceId`,
starting at 1) and delegates the actual pending/timeout bookkeeping to a new
standalone `PendingCommandTracker` (`src/transport/lan/`) — a plain
`QObject` mapping sequence id to a `QTimer`, with `track(sequenceId,
timeoutMs)` and `resolve(sequenceId, success, reason)`, emitting `acked(...)`
on either path. `LanPrinterConnection::sendCommand()` allocates the next id,
builds the payload, calls `track()`, and publishes; its `acked` signal is
forwarded directly to `PrinterConnection::commandAcked` (signal-to-signal
connect). Every incoming report message is checked (before being handed to
`BambuReportParser`) for a `print.sequence_id`, which is passed to
`resolve()` unconditionally — a no-op if it isn't actually pending (e.g. it's
`"0"` from our own `pushall` request); the community-documented protocol
(cited in `PLAN.md`) indicates the printer echoes the command's `sequence_id`
back with a `result` field, which determines the success/failure passed to
`resolve()`. Pulling this into its own class (rather than keeping the
`QHash`/`QTimer` bookkeeping as private `LanPrinterConnection` members, as
originally sketched above) makes it directly unit-testable —
`tests/pendingcommandtrackertest.cpp` exercises the resolve/timeout/no-op/
idempotency behavior without a real MQTT connection or a fake
`PrinterConnection`.

**Protocol uncertainty is explicit and defensive**: unlike the full status
report schema (validated live against a real printer in Phase 1), the exact
shape of a command acknowledgement has not yet been confirmed against real
hardware — `PLAN.md` carries it forward from the same reverse-engineered
community docs, without the same level of confirmation Phase 1's status
fields got. `LanPrinterConnection` is written so an unrecognized or
never-arriving ack degrades to a **timeout failure**, not a silent hang or a
false success — the safe default when the exact ack shape is uncertain. The
manual verification step in `tasks.md` includes watching real MQTT traffic to
confirm (and adjust, if needed) the matching logic against actual hardware.

**`PrinterCommand`**: a small value type (`enum class Type { Pause, Resume,
Stop, SkipObjects }` plus `QList<int> objectIds`, only meaningful for
`SkipObjects`). `BambuCommandBuilder` gains `pause()`, `resume()`, `stop()`,
and `skipObjects(const QList<int> &ids)`, each returning the JSON payload
`QByteArray` and taking the sequence id to embed, mirroring `pushAll()`'s
existing shape from Phase 1.

**Confirmation UX for `stop`**: a generic `ConfirmActionDialog.qml` (title +
message + confirm/cancel), opened by `PrinterDetailView.qml` only for the
stop action; pause/resume/skip-objects call `PrinterController` directly with
no intermediate dialog (skip-objects still needs its own small input dialog
for the object-id list, but that's an input prompt, not a
destructive-action confirmation).

## Risks / Trade-offs

- **[Risk]** The assumed ack shape (`print.sequence_id` + `print.result`
  echoed on the report topic) may not match real firmware exactly →
  **Mitigation**: unmatched/never-arriving acks resolve as a timeout failure
  rather than hanging or falsely reporting success; the manual verification
  step captures real traffic to confirm or correct this before considering
  the feature done. If the real shape differs, only the matching logic in
  `LanPrinterConnection`'s message handler needs to change — the
  `sendCommand()`/`commandAcked` contract on `PrinterConnection` stays the
  same.
- **[Trade-off]** Skip-objects requires the user to already know the numeric
  object IDs (no picker) → accepted per `PLAN.md`; revisit if a later phase
  finds the plate/object list reliably available in the local report.
