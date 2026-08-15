## Context

See `proposal.md` for motivation. Phase 0 (archived) established the CMake/ECM
build, the Plasma 6 Applet KPackage, and a compiled QML plugin with a stub
`PrinterListModel`. This design covers how that gets extended with a real LAN
transport, real status parsing, and real persistence — the pieces needed to
satisfy the `printer-management` and `printer-status-monitoring` capabilities
and the `tray-applet-shell` modification.

## Goals / Non-Goals

**Goals:**
- One real LAN printer can be added, trusted (TOFU), monitored live, and
  survives a Plasma restart.
- The domain/protocol/transport code is unit-testable without a live printer
  or a running Plasma session (fixture-driven).
- The design leaves the `PrinterConnection` seam clean enough that Phase 2
  (control commands) and Phase 3 (Cloud transport) don't require rearchitecting
  it — only extending it.

**Non-Goals:**
- No control commands (pause/resume/stop) — `LanPrinterConnection` only reads
  reports and publishes `pushall` in this phase; `sendCommand()` doesn't exist
  yet.
- No Bambu Cloud, no plate-preview thumbnails, no camera.
- No support for multiple simultaneous instances of this applet coexisting
  with *independent* state — see Risks for why this isn't actually a gap.

## Decisions

**MQTT client threading**: use `mosquitto_loop_start()` (libmosquitto's own
background thread) rather than pumping `mosquitto_loop()` from a `QTimer` on
the Qt main thread. Mosquitto's C callbacks (on-message, on-connect,
on-disconnect) then fire on that background thread; `MqttClient` translates
each callback directly into a Qt signal emission. This is safe without extra
locking because Qt signal emission is thread-safe by design — a receiver
living on a different thread (the Qt main/GUI thread, where `LanPrinterConnection`
lives) automatically gets the emission queued via `Qt::AutoConnection`.
Alternative considered: manual `mosquitto_loop()` polling on a `QTimer` — 
rejected because it either adds polling latency or busy-loops, whereas
mosquitto's own thread already blocks efficiently on the socket.

**LAN TLS trust model**: certificate probing is a separate step from the
actual MQTT session. `CertificateProbe` opens a plain `QSslSocket` to
`<host>:<mqttPort>`, completes a TLS handshake with `QSslSocket::QueryPeer`
verification (chain validation is expected to fail — the cert is self-signed
— we only want the peer certificate itself), and computes its SHA-256
fingerprint. That fingerprint is compared against `CertificateTrustStore`
(a `KConfig`-backed map of `serial -> fingerprint`). Only after the user
accepts a new fingerprint (or it already matches a pinned one) does
`LanPrinterConnection` open the real `MqttClient` session, which itself uses
`mosquitto_tls_insecure_set(true)` (skipping libmosquitto's own chain
validation, since we've already independently vetted this exact certificate).
This mirrors the "SSH host key" UX pattern and matches how the reference
project (`omarchy-bambu-companion`) and other Bambu tooling handle self-signed
printer certs. Alternative considered: trust-and-store on first connect with
no confirmation dialog — rejected because it silently accepts a
possibly-attacker-controlled certificate on an untrusted network.

**Printer identity and config persistence**: each `PrinterProfile` gets a
stable `QUuid` generated at creation time, independent of host/serial (so a
printer that gets a new DHCP lease or gets factory-reset can, in a later
phase, be re-pointed without losing its identity/history). Non-secret fields
(name, host, serial, mqtt port) persist via `KSharedConfig::openConfig()`,
one `[Printer <uuid>]` group per printer. The LAN access code persists
separately via `SecretStore` (KWallet), keyed `lan-access-code/<uuid>` in a
`"BambuCompanion"` folder — never written to the plain config file. This
split is required by the `printer-management` spec's "access code is stored
securely" requirement and by `CLAUDE.md`'s existing KWallet convention.

**QML exposure shape**: `PrinterRegistry` is a plain C++ singleton
(`PrinterRegistry::instance()`), *not* itself QML-registered. Two QML-facing
`QML_SINGLETON` types sit in front of it: `PrinterListModel` (read-only
`QAbstractListModel`, exposes each printer's live status as rows for the
popup's `ListView`) and `PrinterController` (write/action facade,
`Q_INVOKABLE addLanPrinter(...)`, and later `pause`/`resume`/etc. in Phase 2).
Both are constructed once per QML engine and both read from/act on the same
underlying `PrinterRegistry::instance()`, so state stays consistent
regardless of how many times Plasma's `Loader` recreates the popup's QML
tree. Alternative considered: make `PrinterListModel` own the registry
directly (as Phase 0's stub did) — rejected because a `Loader`-recreated
model would otherwise lose in-memory connection state (or require
re-persisting/re-reading everything on every popup open).

**Report parsing and delta merge**: `BambuReportParser` holds no state itself
— it takes the *previous* `PrinterStatus` and a raw JSON payload and returns
an updated `PrinterStatus`, copying forward any field not present in the
payload. `LanPrinterConnection` owns the "current" `PrinterStatus` per
printer and feeds it through the parser on every message. Fixture-based unit
tests cover both a full report and a delta report building on a prior full
report. Exact field names are taken from the community-documented Bambu
report schema (`Doridian/OpenBambuAPI`) already cited in `PLAN.md`; since
that schema is reverse-engineered and can vary slightly by printer
model/firmware, the parser is written defensively (missing/unexpected fields
are ignored rather than treated as errors) and the fixtures should be
cross-checked against a real printer's report during manual verification.

**Static-library split (extends the Phase 0 pattern)**: all new non-QML
classes (`PrinterProfile`, `PrinterStatus`, `PrinterConnection`,
`PrinterRegistry`, `ConnectionFactory`, `MqttClient`, `LanPrinterConnection`,
`CertificateProbe`, `CertificateTrustStore`, `SecretStore`,
`BambuReportParser`) are added to the `bambucompanioncore` static library
introduced in Phase 0 — no QML registration needed, so no duplication.
Only `PrinterListModel` and `PrinterController` (the two `QML_SINGLETON`
types) need to be compiled a second time directly into the
`bambucompanionplugin` QML module target, following the same pattern Phase 0
established for `PrinterListModel`.

**New dependencies**: `pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET
libmosquitto)` (portable via pkg-config per the earlier project-wide
research — not packaged as a CMake config on most distros), plus
`find_package(KF6 COMPONENTS Wallet Config)` and `find_package(Qt6 COMPONENTS
Network)` (for `QSslSocket`). `ConfigWidgets`/`KCMUtils` are deferred to
whichever task actually wires up `package/contents/config/ConfigPrinters.qml`
if it turns out to need them beyond plain QML.

## Risks / Trade-offs

- **[Risk]** The Bambu report JSON schema is reverse-engineered, not official
  — a firmware update could add/rename/restructure fields → **Mitigation**:
  `BambuReportParser` ignores unknown fields and never throws on a missing
  expected field (defaults stay at "unknown" rather than crashing); fixtures
  are clearly labeled as best-effort and meant to be refreshed against real
  printer output during manual verification.
- **[Risk]** `mosquitto_tls_insecure_set(true)` disables libmosquitto's own
  certificate chain validation entirely → **Mitigation**: this is only safe
  because `CertificateProbe`/`CertificateTrustStore` independently pin the
  exact certificate fingerprint *before* that session opens, and a
  fingerprint mismatch on any later connection blocks reconnection until the
  user re-confirms — equivalent security posture to SSH's `known_hosts`.
- **[Trade-off]** Multiple simultaneous instances of this applet (e.g. added
  to two different panels at once) share one `PrinterRegistry::instance()`
  process-wide singleton rather than having independent state → accepted as a
  non-issue: Plasma runs all applet instances for one user in the same
  `plasmashell` process/QML engine set, so sharing one registry is actually
  the *correct* behavior (one printer list, consistently shown wherever the
  applet appears), not a limitation.
- **[Risk]** libmosquitto's background thread (from `mosquitto_loop_start`)
  emitting Qt signals that trigger UI-facing slots (e.g. opening the TOFU
  dialog) → **Mitigation**: covered by Qt's automatic cross-thread queued
  connections; no manual mutex/lock needed as long as `MqttClient` doesn't
  also expose non-signal state that's read directly from the GUI thread
  without synchronization. Enforce this in code review: `MqttClient`'s public
  surface should be signals-out, method-calls-in only, never raw shared
  fields.

## Migration Plan

No user-facing migration — Phase 0 shipped no persistent state (the stub
model held no config), so there's nothing to migrate away from. Once printer
profiles exist under this phase's config schema, any later phase (e.g. Phase
3 adding `ConnectionMode`) must treat this schema as the baseline to extend,
not replace.
