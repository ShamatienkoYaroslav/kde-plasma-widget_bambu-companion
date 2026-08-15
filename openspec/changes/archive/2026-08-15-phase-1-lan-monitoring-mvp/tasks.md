## 1. Build system: new dependencies

- [x] 1.1 Update top-level `CMakeLists.txt`: add `find_package(KF6 COMPONENTS
    Wallet Config)` (extending the existing `CoreAddons Package` list),
      add `find_package(Qt6 COMPONENTS Network)` (extending `Core Qml Quick`),
      add `find_package(PkgConfig REQUIRED)` +
      `pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)`.

## 2. Core domain types (`src/core/`)

- [x] 2.1 Add `src/core/PrinterProfile.h`/`.cpp`: id (`QUuid`), name, host,
      serial, mqtt port (default 8883). Plain value type, no QML annotations.
- [x] 2.2 Add `src/core/PrinterStatus.h`/`.cpp`: state, progress percentage,
      current/total layer, nozzle/bed temps (current+target), chamber temp,
      fan speeds, speed profile, WiFi signal, estimated remaining time; all
      fields optional/defaulted so a freshly-created status reads as
      "unknown" rather than zeroed.
- [x] 2.3 Add `src/core/PrinterConnection.h`: abstract interface — `start()`,
      `stop()`, `state()`, signals `stateChanged`, `statusUpdated`,
      `certificateTrustNeeded` (LAN-specific, default no-op emit point), and
      the `virtual CameraSource *cameraSource() { return nullptr; }`
      extension point for Phase 4. No `sendCommand()` yet (Phase 2).
- [x] 2.4 Add `src/core/ConnectionFactory.h`/`.cpp`: `create(const
    PrinterProfile &)` — for now always returns a `LanPrinterConnection`.
- [x] 2.5 Add `src/core/PrinterRegistry.h`/`.cpp`: process-wide C++ singleton
      (`PrinterRegistry::instance()`), owns the `QList<PrinterProfile>` and
      one `PrinterConnection` per profile (via `ConnectionFactory`), exposes
      `addLanPrinter(...)`, `printers()`, and per-printer status/state
      accessors plus change signals.

## 3. Config persistence & secrets (`src/core/`, `src/security/`)

- [x] 3.1 `PrinterRegistry` persists non-secret `PrinterProfile` fields via
      `KSharedConfig::openConfig()`, one `[Printer <uuid>]` group per printer;
      loads all persisted profiles on construction and reconnects each.
- [x] 3.2 Add `src/security/SecretStore.h`/`.cpp`: KWallet wrapper —
      `storeLanAccessCode(QUuid, QString)`, `lanAccessCode(QUuid)`, opening
      the wallet asynchronously and using folder `"BambuCompanion"`.
      (Implemented as a synchronous open instead of async — see code comment
      in `SecretStore.cpp`; callers need the result immediately and the
      observable behavior — KWallet-backed, no plain-config leakage — is
      unaffected.)
- [x] 3.3 Wire `PrinterRegistry::addLanPrinter(...)` to write the access code
      through `SecretStore`, never through `KSharedConfig`.

## 4. LAN certificate trust (`src/security/`)

- [x] 4.1 Add `src/security/CertificateProbe.h`/`.cpp`: `QSslSocket`-based
      TLS handshake to `<host>:<port>`, `QueryPeer` verify mode, computes the
      peer certificate's SHA-256 fingerprint, emits a result signal.
- [x] 4.2 Add `src/security/CertificateTrustStore.h`/`.cpp`: `KConfig`-backed
      map of `serial -> fingerprint`; `isPinned`, `isTrusted(serial,
    fingerprint)`, `trust(serial, fingerprint)`.
- [x] 4.3 Wire the TOFU flow into `LanPrinterConnection::start()`: probe
      first; if unpinned or mismatched, emit `certificateTrustNeeded` and
      wait for explicit acceptance (via `PrinterController`) before opening
      the real MQTT session; on mismatch of an already-pinned fingerprint,
      refuse to connect until re-confirmed.

## 5. MQTT transport (`src/transport/lan/`)

- [x] 5.1 Add `src/transport/lan/MqttClient.h`/`.cpp`: libmosquitto wrapper
      using `mosquitto_loop_start()`; TLS connect with
      `mosquitto_tls_insecure_set(true)`; `subscribe(topic)`,
      `publish(topic, payload)`; Qt signals `connected()`,
      `disconnected(QString reason)`, `messageReceived(QString topic,
    QByteArray payload)`, all safely emitted from the mosquitto thread per
      `design.md`'s threading decision.
- [x] 5.2 Add `src/transport/lan/LanPrinterConnection.h`/`.cpp`: implements
      `PrinterConnection`; owns an `MqttClient`; on connect, subscribes to
      `device/{serial}/report` and publishes the `pushall` request; on each
      message, runs the payload through `BambuReportParser` and emits
      `statusUpdated`; detects disconnects and auto-reconnects.

## 6. Report parsing (`src/protocol/`)

- [x] 6.1 Add `src/protocol/BambuReportParser.h`/`.cpp`: stateless —
      `PrinterStatus merge(const PrinterStatus &previous, const QJsonObject
    &report)`; copies forward any field not present in `report`; ignores
      unknown/unexpected fields rather than erroring.
- [x] 6.2 Add `src/protocol/BambuCommandBuilder.h`/`.cpp` with just
      `pushAll()` for now (returns the `{"pushing":{...,"command":"pushall",...}}`
      JSON payload) — command-building for pause/resume/stop is Phase 2.

## 7. QML-facing types (`src/qmlplugin/`)

- [x] 7.1 Replace the Phase 0 stub `PrinterListModel` with a real
      `QML_SINGLETON` implementation backed by `PrinterRegistry::instance()`
      — roles for name, status, progress, temps, etc.; updates rows as
      `statusUpdated`/profile-list-changed signals arrive from the registry.
      Add it to `bambucompanioncore` (for tests) AND compile it a second time
      directly into `bambucompanionplugin` (same duplication pattern as
      Phase 0), per `design.md`.
- [x] 7.2 Add `src/qmlplugin/PrinterController.h`/`.cpp`: `QML_SINGLETON`,
      `Q_INVOKABLE addLanPrinter(QString name, QString host, QString serial,
    QString accessCode, int mqttPort)`, `Q_INVOKABLE
    confirmCertificateTrust(QUuid printerId, QString fingerprint, bool
    accept)`. Same compile-twice treatment as `PrinterListModel`.
- [x] 7.3 Update `src/qmlplugin/CMakeLists.txt` and `tests/CMakeLists.txt`
      accordingly (new sources in `bambucompanioncore`; the two QML-facing
      types duplicated into the plugin target as noted above).

## 8. UI (`package/contents/ui/`, `package/contents/config/`)

- [x] 8.1 Update `main.qml`: `fullRepresentation` now binds to the real
      `PrinterListModel` singleton; shows an "Add printer" prompt/button when
      the model is empty instead of the old hardcoded rows.
- [x] 8.2 Add `AddPrinterDialog.qml`: form for name/host/serial/access
      code/port (LAN fields only), calls
      `PrinterController.addLanPrinter(...)`, validates required fields
      before submitting per the `printer-management` spec.
- [x] 8.3 Add `CertificateConfirmDialog.qml`: shows the printer name and
      certificate fingerprint, calls
      `PrinterController.confirmCertificateTrust(...)` on accept/reject.
- [x] 8.4 Add `PrinterListItem.qml`: one row's summary (name, state,
      progress) for the popup's `ListView` delegate.
- [x] 8.5 Add `PrinterDetailView.qml`: full status view for a selected
      printer (temps, fans, speed profile, WiFi signal, layer/Z, remaining
      time).
- [x] 8.6 Add `package/contents/config/ConfigPrinters.qml` +
      `config/main.xml`: lists configured printers (remove action only for
      now — editing deferred); no secrets ever displayed in this UI.
      (Required adding `PrinterRegistry::removePrinter`/`SecretStore::
    removeLanAccessCode`/`PrinterController::removePrinter` — a small,
      necessary extension of tasks 2.5/3.2/7.2 to back this UI action, not
      called for by any spec requirement since removal isn't itself
      spec'd, only referenced by this task's own UI description.)

## 9. Tests (`tests/`)

- [x] 9.1 Add `tests/fixtures/`: sample full and delta Bambu MQTT report
      JSON payloads (based on the community-documented schema cited in
      `PLAN.md`/`design.md`), clearly commented as best-effort pending
      cross-check against a real printer.
- [x] 9.2 Add `tests/bambureportparsertest.cpp`: full report parses into a
      complete `PrinterStatus`; a delta report updates only the fields it
      contains and leaves the rest unchanged (covers both
      `printer-status-monitoring` requirements).
- [x] 9.3 Add `tests/certificatetruststoretest.cpp`: unpinned fingerprint is
      untrusted; pinning then matching succeeds; pinning then a different
      fingerprint is rejected.
- [x] 9.4 Add `tests/printerregistrytest.cpp`: adding a printer persists its
      non-secret fields (using a temporary/isolated `KConfig` for the test)
      and asserts the access code never appears in the config file contents.
      (`PrinterRegistry` is a process-wide singleton, so an in-process
      "destroy and reload" round-trip isn't feasible to test directly; the
      read path — `loadPersistedPrinters()` — uses the identical group/key
      layout this test asserts was written, and the actual cross-restart
      round-trip is covered by manual verification step 10.3.)
- [x] 9.5 Wire new test sources into `tests/CMakeLists.txt` via `ecm_add_test`
      calls linking `bambucompanioncore` (+ `Qt::Network` where needed for
      `CertificateProbe`-adjacent tests, `KF6::Wallet`/`KF6::Config` where
      needed).

## 10. Verification

- [x] 10.1 Build locally: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake
    --build build && ctest --output-on-failure --test-dir build`.
- [x] 10.2 Reinstall: `sudo cmake --install build` (plugin) and
      `kpackagetool6 --type Plasma/Applet -u package` (KPackage), restarting
      `plasmashell` if needed per the Phase 0 install caveats documented in
      `CLAUDE.md`.
- [x] 10.3 In a live Plasma 6 session, against a real Bambu Lab printer on
      the LAN: add it via the "Add printer" dialog; confirm the TOFU
      certificate dialog appears exactly once and the connection completes;
      confirm live status (state/progress/temps/fans/speed/WiFi/layer)
      appears and updates as the printer's state changes; restart
      `plasmashell` and confirm the printer profile persists and
      reconnects without re-prompting for trust (fingerprint unchanged);
      power off/on the printer and confirm the connection recovers.
- [x] 10.4 Confirm CI still passes (build + unit tests; no real printer
      needed for CI).
