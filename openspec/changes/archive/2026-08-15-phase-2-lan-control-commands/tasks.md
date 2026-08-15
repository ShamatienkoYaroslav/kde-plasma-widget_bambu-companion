## 1. Command data & payload builders

- [x] 1.1 Add `src/core/PrinterCommand.h`: `enum class Type { Pause, Resume,
      Stop, SkipObjects }` plus a small struct carrying `Type type` and
      `QList<int> objectIds` (only meaningful for `SkipObjects`).
- [x] 1.2 Extend `src/protocol/BambuCommandBuilder.h`/`.cpp`: add
      `pause(sequenceId)`, `resume(sequenceId)`, `stop(sequenceId)`,
      `skipObjects(sequenceId, objectIds)`, each returning the
      `{"print": {...}}` JSON payload per `design.md`.

## 2. `PrinterConnection` interface + LAN implementation

- [x] 2.1 Add `virtual QString sendCommand(const PrinterCommand &command) = 0;`
      to `src/core/PrinterConnection.h` (returns the generated sequence id),
      and `Q_SIGNALS: void commandAcked(const QString &sequenceId, bool
      success, const QString &reason);`.
- [x] 2.2 Implement `LanPrinterConnection::sendCommand()`: allocates the next
      sequence id, builds the payload via `BambuCommandBuilder`, publishes to
      `device/{serial}/request`, records a pending entry with a 10s timeout
      `QTimer`, returns the sequence id.
- [x] 2.3 In `LanPrinterConnection`'s message handler, check incoming report
      payloads for a `print.sequence_id` matching a pending entry before
      passing the payload to `BambuReportParser`; on match, resolve via
      `print.result` and emit `commandAcked`. On the pending entry's timeout
      firing first, resolve as a failure with a "timed out" reason and emit
      `commandAcked`.

## 3. QML-facing controller

- [x] 3.1 Add to `src/qmlplugin/PrinterController`: `Q_INVOKABLE void
      pause(const QString &printerId)`, `resume(...)`, `stop(...)`,
      `Q_INVOKABLE void skipObjects(const QString &printerId, const
      QList<int> &objectIds)`, each calling
      `PrinterRegistry::instance().sendCommand(...)` (new pass-through method
      on `PrinterRegistry` mirroring `confirmCertificateTrust`'s pattern).
- [x] 3.2 Add `Q_SIGNALS: void commandFailed(const QString &printerId, const
      QString &reason);` on `PrinterController`, relayed from
      `PrinterRegistry`'s relay of each connection's `commandAcked` signal
      (success case needs no signal — status updates already reflect it).

## 4. UI

- [x] 4.1 Add `package/contents/ui/ConfirmActionDialog.qml`: generic
      title/message + confirm/cancel `Kirigami.OverlaySheet`, reusable for
      any destructive action (only `stop` uses it in this phase).
- [x] 4.2 Wire `PrinterDetailView.qml`: "Pause"/"Resume" buttons call
      `PrinterController` directly (shown based on current print state);
      "Stop" button opens `ConfirmActionDialog` and only calls
      `PrinterController.stop(...)` on confirm; a small "Skip objects…"
      action opens an object-id entry prompt (comma-separated numeric IDs)
      and calls `PrinterController.skipObjects(...)`.
      (Object-id prompt implemented as its own `SkipObjectsDialog.qml`,
      following the existing one-dialog-per-file pattern.)
- [x] 4.3 Show `PrinterController.commandFailed` feedback in the popup (e.g.
      a transient `Kirigami.InlineMessage` or notification-style banner in
      `PrinterDetailView.qml`) per the "command outcome is reported back to
      the user" requirement.

## 5. Tests

- [x] 5.1 Add `tests/bambucommandbuildertest.cpp`: `pause()`/`resume()`/
      `stop()`/`skipObjects()` each produce the expected JSON shape
      (command name, sequence id, object-id list where applicable).
- [x] 5.2 Add a fake/test-double `PrinterConnection` (or extend
      `LanPrinterConnection` test coverage if simpler) verifying sequence-id
      ack correlation: a matching `print.sequence_id`/`result` report resolves
      the pending command with the right success/failure; an unmatched
      pending command times out and resolves as a failure. (Use a short
      timeout override for the test rather than waiting the full 10s.)
      (Implemented by extracting the sequence-id/timeout matching logic from
      `LanPrinterConnection` into a standalone `PendingCommandTracker` class
      — `design.md` updated to match — so it's directly unit-testable
      without a fake `PrinterConnection` or real MQTT/network;
      `tests/pendingcommandtrackertest.cpp` covers resolve-before-timeout,
      unknown-id-is-a-no-op, timeout-resolves-as-failure, and
      resolve-is-idempotent.)
- [x] 5.3 Wire new test sources into `tests/CMakeLists.txt`.

## 6. Verification

- [x] 6.1 Build locally: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake
      --build build && ctest --output-on-failure --test-dir build`.
- [ ] 6.2 Reinstall: `sudo cmake --install build` + `kpackagetool6 --type
      Plasma/Applet -u package`, restarting `plasmashell` if needed.
- [ ] 6.3 Against a real printer: pause an active print and confirm it pauses
      (both on the printer and in the widget's displayed state); resume and
      confirm it resumes; trigger stop, confirm the confirmation dialog
      appears, cancel it (nothing happens), then confirm it and confirm the
      print actually stops. Capture the printer's real MQTT report traffic
      around a sent command (e.g. via `mosquitto_sub` against the printer) to
      confirm or correct the ack-matching logic from `design.md` against real
      hardware.
      **Blocked in this session**: the two configured printers (`BL 1`/`BL 2`,
      confirmed working from Phase 1's TOFU/persistence testing) are not
      currently reachable in LAN mode, so nothing is in a Printing/Paused
      state and the action buttons correctly don't appear. Confirmed the UI
      behaves correctly for the "nothing to act on" case; the actual
      pause/resume/stop/ack round-trip against hardware — and the ack-shape
      assumption in `design.md` — remain unverified pending a printer
      actually reachable and printing over LAN.
- [ ] 6.4 Skip-objects: manual-only, needs an active multi-plate print to
      validate meaningfully — document result (or that it wasn't possible to
      test in this session) rather than skipping silently.
      **Blocked in this session** for the same reason as 6.3.
- [x] 6.5 Confirm CI still passes.
      (Found and fixed two real, pre-existing CI gaps in the process: the
      workflow's dnf install list was never updated for Phase 1's
      libmosquitto/KWallet/KConfig dependencies, and every QTest binary
      aborted in the headless container without `QT_QPA_PLATFORM=offscreen`
      — verified the fix locally by reproducing the abort with a fully
      clean env before pushing. CI is green as of run 31888026536.)
