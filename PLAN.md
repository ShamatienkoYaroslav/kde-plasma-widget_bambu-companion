# PLAN.md — Bambu Companion implementation roadmap

KDE Plasma 6 System Tray widget (C++/Qt6/KF6 + QML) that shows and controls a
list of the user's Bambu Lab 3D printers. Inspired by
[ypMrg/omarchy-bambu-companion](https://github.com/ypMrg/omarchy-bambu-companion)
(single-printer, LAN-only, monitoring-only Quickshell widget) but broader:
**multi-printer, LAN + Bambu Cloud, monitoring + print control**, with a camera
view as a later phase.

See [`CLAUDE.md`](./CLAUDE.md) for architecture and build instructions.

## How this document is used

Each checklist item below becomes one [OpenSpec](https://github.com/Fission-AI/OpenSpec)
change, worked one at a time via `/opsx:propose "<item>"` → review → `/opsx:apply`
→ `/opsx:archive`. Work phases in order; within a phase, earlier items are
dependencies for later ones unless noted otherwise. Check items off as their
OpenSpec change is archived.

Locked-in scope decisions (do not re-litigate):
- **Multi-printer**: a list of printers, not just one.
- **Connectivity**: both LAN (local MQTT/FTPS, access code) and Bambu Cloud
  (account login), selectable per printer or as a global default, with
  LAN-preferred-then-cloud fallback supported.
- **Control**: pause/resume/stop/skip-objects, not monitoring-only — with a
  confirmation dialog before the destructive `stop` command.
- **Camera live view**: planned, but as a later phase (Phase 4), not part of
  the initial monitoring/control work.
- **Printer discovery**: manual entry only (host/IP, serial, access code for
  LAN; account login + device picker for Cloud). No SSDP auto-discovery.

## Repository layout (target end-state)

```
src/core/              PrinterProfile, PrinterStatus, PrinterCommand,
                        PrinterConnection (interface), PrinterRegistry,
                        ConnectionFactory
src/transport/lan/      MqttClient (libmosquitto + TLS), LanPrinterConnection,
                        FtpsFileClient (libcurl), LanCameraSource (phase 4)
src/transport/cloud/    CloudAuthClient, CloudDeviceDirectory, CloudPrinterConnection
src/protocol/           BambuReportParser, BambuCommandBuilder (shared LAN + Cloud)
src/security/           SecretStore (KWallet), CertificateProbe, CertificateTrustStore
src/notifications/      PrintNotifier (KNotification)
src/qmlplugin/          PrinterListModel, PrinterController (QML_ELEMENT, built via
                        ecm_add_qml_module)
package/                metadata.json + contents/ui/*.qml + contents/config/*.qml
                        (the installable Plasma 6 Applet KPackage)
tests/                  protocol/, core/, fixtures/ (sample printer report JSON)
po/                     translations
```

## Build system

- Top-level CMake using ECM: `find_package(ECM REQUIRED NO_MODULE)`,
  `include(ECMQmlModule)`.
- `find_package(Qt6 COMPONENTS Core Network Qml Quick)`.
- `find_package(KF6 COMPONENTS CoreAddons I18n Config ConfigWidgets KCMUtils
  Package Notifications Wallet Archive)`.
- `pkg_check_modules(... IMPORTED_TARGET libmosquitto libcurl)` — neither has a
  reliable CMake config-package across distros; use pkg-config.
- Applet packaging via `kpackage_install_package(package
  io.github.shamatienkoyaroslav.bambucompanion plasmoids plasma)`.
- QML plugin via `ecm_add_qml_module` / `ecm_finalize_qml_module`, with
  `QML_ELEMENT`/`QML_SINGLETON` macros directly in C++ headers — no manual
  `qmlRegisterType()` calls.

## Protocol reference (carry into implementation)

Sources: [Doridian/OpenBambuAPI `mqtt.md`](https://github.com/Doridian/OpenBambuAPI/blob/main/mqtt.md),
[coelacant1/Bambu-Lab-Cloud-API](https://github.com/coelacant1/Bambu-Lab-Cloud-API).
Both are community reverse-engineering references, not official Bambu Lab docs.

**LAN MQTT** — `mqtts://<printer-ip>:8883`, username `bblp`, password = the
printer's LAN access code, self-signed TLS cert (pin it TOFU-style). Subscribe
`device/{serial}/report`, publish `device/{serial}/request`. On connect, publish
once: `{"pushing":{"sequence_id":"0","command":"pushall","version":1,"push_target":1}}`
— don't poll this repeatedly, P1-series printers stream deltas afterward and the
parser must merge deltas into existing state, not overwrite wholesale.

**LAN control commands** — all published to `device/{serial}/request`:
- `{"print":{"sequence_id":"<n>","command":"pause","param":""}}` (same shape for
  `resume`/`stop`)
- `{"print":{"sequence_id":"<n>","command":"skip_objects","timestamp":<unix_ts>,"obj_list":[<id>,...]}}`
- Correlate command acknowledgement via the echoed `sequence_id` on the
  `.../report` topic (`"result":"success"` or a failure + `"reason"`).

**Bambu Cloud (unofficial — verify empirically, may drift without notice)**:
- Auth: `POST https://api.bambulab.com/v1/user-service/user/login` with
  `{"account":"<email>","password":"<password>"}`. If the response indicates
  `"loginType":"verifyCode"`, request a code via
  `POST /v1/user-service/user/sendemail/code` (`{"email":"<email>","type":"codeLogin"}`)
  then re-POST login with `{"account":"<email>","code":"<code>"}` to get an
  `accessToken`. China region uses `api.bambulab.cn` with a different flow.
- Device list: `GET https://api.bambulab.com/v1/iot-service/api/user/bind`
  with `Authorization: Bearer <token>` → device entries include `dev_id`,
  `name`, `online`, and often the device's own `dev_access_code` (useful for
  opportunistic LAN fallback).
- Relay MQTT: `<region>.mqtt.bambulab.com:8883` (e.g. `us.mqtt.bambulab.com`),
  **publicly-trusted** TLS cert (no TOFU pinning needed, unlike LAN). Exact
  username/password frame for the relay connection is the least-confirmed part
  of this whole plan — verify against a real account early in Phase 3 rather
  than assuming the reverse-engineered shape is exact.
- Document this API as unofficial/reverse-engineered in the README and in the
  cloud-login UI copy — Bambu Lab could change or break it without notice.

**FTPS (plate-preview thumbnails)** — implicit TLS, port 990, user `bblp` /
access code. Qt6 dropped FTP support entirely from `QNetworkAccessManager`; use
libcurl, which handles the TLS-session-reuse-across-control-and-data-channel
quirk many FTPS servers (including Bambu's) require.

## Testing approach

Unit-test `protocol/` and `core/` logic against fixture JSON (Qt Test, run via
`ctest`) — this is pure logic and fully CI-able. `transport/` and UI work is
verified manually in a live Plasma session against a real printer (and, for
Cloud, a real Bambu account) — headless Plasma + real-printer testing isn't
practical in CI.

---

## Phase 0 — Project scaffolding

Goal: an empty-but-buildable, installable, loadable-in-Plasma applet skeleton;
CI green.

- [x] Top-level CMake build system (`CMakeLists.txt`, `cmake/`), CI skeleton
      (`.github/workflows/ci.yml`), `LICENSE`, `README.md` stub (with the
      cloud-API disclaimer), `CONTRIBUTING.md`.
- [x] `package/metadata.json` + `package/contents/ui/main.qml` (bare
      `PlasmoidItem` placeholder) + `package/CMakeLists.txt` using
      `kpackage_install_package`. Use `X-Plasma-NotificationAreaCategory:
      "Hardware"` and `X-Plasma-API-Minimum-Version: "6.0"` (verified against
      current `plasma-workspace` applets — no need for the legacy
      `X-Plasma-NotificationArea` key in Plasma 6).
- [x] `src/qmlplugin/` stub: a trivial `QML_ELEMENT` `PrinterListModel` with
      dummy data, built via `ecm_add_qml_module`, proving the plugin→applet
      wiring end-to-end before any real protocol code exists.
- [x] `tests/CMakeLists.txt` with one trivial `QtTest`-based smoke test to
      prove `ctest` works.

Archived as OpenSpec change
[`2026-08-15-phase-0-project-scaffolding`](./openspec/changes/archive/2026-08-15-phase-0-project-scaffolding/)
(CI-passing-on-a-PR left open pending a git remote).

Verify: builds; `kpackagetool6 --type Plasma/Applet -i package` installs it;
widget appears in "Add Widgets" and, via `X-Plasma-NotificationAreaCategory`,
is auto-discovered by the system tray with placeholder content rendering; CI
green.

## Phase 1 — LAN monitoring MVP (read-only)

Goal: add a real LAN printer manually, see its live status (state, progress,
temps, fans, speed, WiFi signal, layer/Z) in the tray popup. No control
commands yet, no cloud.

- [x] `src/core/PrinterProfile`, `PrinterStatus`, `PrinterConnection`
      (abstract interface — include the `virtual CameraSource*
      cameraSource() { return nullptr; }` extension point now, implemented in
      Phase 4), `PrinterRegistry` (LAN-only for now), `ConnectionFactory`
      (returns `LanPrinterConnection` only for now).
- [x] `src/transport/lan/MqttClient` — libmosquitto wrapper: TLS connect
      (`mosquitto_tls_insecure_set`, since printer certs are self-signed and
      already TOFU-pinned by our own probe), subscribe/publish helpers, Qt
      signals (`connected()`, `disconnected(reason)`, `messageReceived(QByteArray)`).
- [x] `src/security/CertificateProbe` (QSslSocket-based TLS handshake to read
      the peer cert and compute its SHA-256 fingerprint) and
      `CertificateTrustStore` (persisted pinned fingerprints, KConfig-backed).
      Wire the TOFU flow before the first real MQTT connect to a given printer.
- [x] `src/protocol/BambuReportParser` (JSON report → `PrinterStatus`,
      handling both full and delta/partial reports by merging into existing
      state) and `BambuCommandBuilder::pushAll()`.
- [x] `src/transport/lan/LanPrinterConnection` — owns an `MqttClient`,
      publishes `pushall` on connect, parses incoming messages, emits
      `statusUpdated`.
- [x] `src/security/SecretStore` (KWallet) for the LAN access code.
- [x] `src/qmlplugin/PrinterListModel` (real, backed by `PrinterRegistry`),
      `PrinterController` (`Q_INVOKABLE addLanPrinter(host, serial,
      accessCode, mqttPort)`).
- [x] `package/contents/ui/FullRepresentation.qml`, `PrinterListItem.qml`,
      `PrinterDetailView.qml`, `AddPrinterDialog.qml` (LAN fields only),
      `CertificateConfirmDialog.qml` (TOFU fingerprint confirmation UX,
      modeled on SSH host-key prompts).
- [x] `package/contents/config/ConfigPrinters.qml`, `config/main.xml`
      (global settings only at this point).

Verify: unit tests for `BambuReportParser` against fixture JSON (captured
from a real printer's `report` topic, or from OpenBambuAPI sample payloads),
including delta-merge behavior; unit tests for `CertificateTrustStore`
accept/mismatch logic; manual test against a real printer in a live Plasma
session — add it, confirm the TOFU dialog appears once, see live status
updates, verify reconnect after a printer reboot.

Archived as OpenSpec change
[`2026-08-15-phase-1-lan-monitoring-mvp`](./openspec/changes/archive/2026-08-15-phase-1-lan-monitoring-mvp/).

## Phase 2 — LAN control commands

Goal: pause/resume/stop/skip-objects from the tray popup, with confirmation
before the destructive `stop` action.

- [x] `src/core/PrinterCommand` (enum `Pause`/`Resume`/`Stop`/`SkipObjects` +
      payload data) and `BambuCommandBuilder::pause()/resume()/stop()/
      skipObjects(objectIds)` per the payload shapes above.
- [x] `LanPrinterConnection::sendCommand()` — publishes to
      `device/{serial}/request`, correlates the echoed `sequence_id` on
      `.../report` back to the originating command, emits `commandAcked(id,
      success, reason)`. (Sequence-id/timeout matching factored into a
      standalone, independently-tested `PendingCommandTracker`.)
- [x] `PrinterController` invokables: `pause(printerId)`, `resume(printerId)`,
      `stop(printerId)`, `skipObjects(printerId, ids)`.
- [x] `package/contents/ui/ConfirmActionDialog.qml` (used for `stop` only;
      pause/resume fire immediately), wired into `PrinterDetailView.qml`.
- [x] Skip-objects UI: if the current print's plate/object list isn't
      reliably present in the local report payload, scope this to a manual
      "enter object IDs" fallback rather than blocking the whole feature on
      plate-object discovery.

Verify: unit tests on `BambuCommandBuilder` payload shapes and on
sequence-id ack correlation (against a fake connection); manual
pause/resume/stop against a real printer. Skip-objects needs an active
multi-plate print to validate meaningfully — document as manual-only.

Code complete, all unit tests pass, CI green (fixed two pre-existing CI gaps
along the way: missing Phase-1 dependencies in the workflow, and a headless-
container Qt platform-plugin crash needing `QT_QPA_PLATFORM=offscreen`).
**Manual hardware verification (pause/resume/stop/skip-objects against a real
printer, and confirming the ack-payload-shape assumption in the archived
change's `design.md`) is still pending** — neither configured printer was
reachable in LAN mode during this session. Revisit when one is.

Archived as OpenSpec change
[`2026-08-15-phase-2-lan-control-commands`](./openspec/changes/archive/2026-08-15-phase-2-lan-control-commands/).

## Phase 2.5 — LAN plate-preview thumbnails

Goal: show the 2D plate preview thumbnail for the currently-printing file.

- [ ] `src/transport/lan/FtpsFileClient` (libcurl, implicit TLS
      `ftps://<host>:990`, user `bblp`, password = access code) — list and
      download the current gcode/3mf file.
- [ ] 3mf/gcode thumbnail extraction (3mf is a zip; use `KF6::Archive`
      rather than vendoring a zip library) — add `Archive` to the KF6
      `find_package` components.
- [ ] Disk-cache extracted thumbnails (`QStandardPaths::CacheLocation`),
      keyed by filename + mtime, to avoid re-downloading every poll.

Verify: unit test 3mf-thumbnail extraction against a sample `.3mf` fixture;
manual check against a real printer's current print file.

## Phase 3 — Bambu Cloud support

Goal: log into a Bambu account, list bound devices, add printers by picking
from that list, monitor/control via the cloud MQTT relay for printers not
reachable locally (or by user preference).

- [x] **Spike first**: a small manual verification pass confirming the exact
      cloud MQTT username/password auth frame and topic conventions against a
      real Bambu account, before building the rest of this phase — this is
      the least-confirmed part of the whole protocol reference above.
      (Done differently than planned: built against the best-documented
      shape first, then verified live through the applet's own UI rather
      than a separate pre-implementation spike — see the archived change's
      `design.md` for why. The verification happened; the auth frame it
      confirmed turned out wrong — see below.)
- [x] `src/transport/cloud/CloudAuthClient` — `login(email, password)`, 2FA
      branch (`twoFactorRequired` signal → `submitVerificationCode(code)`),
      `logout()`. Token stored via `SecretStore`. Since Bambu doesn't
      document a refresh-token endpoint, re-login on token expiry rather
      than assuming one exists.
- [x] `src/transport/cloud/CloudDeviceDirectory` — fetches and parses the
      bound-device list into `PrinterProfile` candidates for an "add cloud
      printer" picker.
- [x] `src/transport/cloud/CloudPrinterConnection` — implements
      `PrinterConnection` against the relay MQTT broker, reusing
      `BambuReportParser`/`BambuCommandBuilder`.
- [x] Extend `PrinterProfile`/`ConnectionFactory`/`PrinterRegistry` for
      `ConnectionMode` (`LanOnly`, `CloudOnly`, `PreferLanThenCloud`) —
      fallback logic lives in `PrinterRegistry`, not inside either connection
      class, so LAN and Cloud stay independently simple and testable.
- [x] `package/contents/ui/CloudLoginDialog.qml` (email/password + 2FA step +
      explicit "unofficial API" disclaimer), extend `AddPrinterDialog.qml`
      with a "pick from my Bambu account" path.

Verify: unit tests for `CloudAuthClient`'s state machine against a mocked
HTTP backend (no real network call needed in CI); manual end-to-end test
against a real Bambu Cloud account for login/2FA/device-list/relayed
status+control — not automatable in CI.

Code complete, all unit tests pass, CI green. Confirmed working end-to-end
against a real account: login, 2FA, device listing, adding cloud printers.
**Known open gap: live cloud MQTT status/control does not work.** The
account uid needed for the MQTT relay's username couldn't be determined —
the access token is an opaque string (not a JWT, as first assumed), and a
follow-up `/my/info` endpoint guess 404s; neither the login response nor the
device-list response carry a uid anywhere. Degrades safely (`Error` state,
no crash/hang), with permanent diagnostic logging in place (HTTP statuses
and response key names, never values) to pick this back up without
re-guessing from scratch. See the archived change's `design.md` Risks
section for full detail.

Archived as OpenSpec change
[`2026-08-18-phase-3-bambu-cloud-support`](./openspec/changes/archive/2026-08-18-phase-3-bambu-cloud-support/).

## Cross-cutting — Cloud-account-driven printer sync

Not tied to a specific phase number: a UX pivot away from manual printer
entry entirely. Once a Bambu Cloud account is logged in, the printer list
is fully account-sourced — no "Add Printer" UI (neither the LAN form nor
the Cloud device picker). `PrinterRegistry` auto-fetches and adds the
account's devices at startup (if already logged in) and right after login;
logging out (or starting up already logged out with leftover printers)
clears the whole configured printer list, LAN and Cloud alike, since
there's no account to be the source of truth for it otherwise. The empty
state is login-state-aware (login prompt vs. "no printers on this
account"), illustrated with Bambu Lab's own `P1S_cover.png` from their
open-source (AGPL-3.0) BambuStudio repo — see
`package/contents/images/NOTICE.md` for provenance.

Same known gap as Phase 3 still applies: cloud-sourced printers show
`Unknown`/`Error` status until the account uid problem is solved, since
that blocks the cloud MQTT relay connection regardless of how the printer
got added to the list.

Archived as OpenSpec change
[`2026-08-18-cloud-auto-sync-printers`](./openspec/changes/archive/2026-08-18-cloud-auto-sync-printers/).

## Phase 4 — Camera live view (LAN only)

Goal: live JPEG stream from a printer's local camera (proprietary framed-JPEG
protocol on port 6000), shown in `PrinterDetailView.qml`.

- [ ] Reverse-engineering verification pass on the port-6000 framing/auth
      protocol (cross-check against `bambulabs_api`'s and Home Assistant's
      Bambu Lab integration implementations) before writing any code — this
      is more obscure than the MQTT/REST surfaces already documented above.
- [ ] `src/transport/lan/LanCameraSource`, implementing the `CameraSource`
      extension point added to `PrinterConnection` back in Phase 1; override
      it in `LanPrinterConnection` only.
- [ ] Frame delivery to QML (`QQuickImageProvider` or a `VideoOutput`-fed
      frame sink) exposed via `PrinterListModel`/`PrinterController`.
- [ ] `package/contents/ui/CameraView.qml`.

Verify: manual only, against a real printer with the camera enabled; add a
frame-parsing unit test only if/once the binary header format is nailed down
during the verification pass.

## Phase 5 — Polish / packaging

Goal: ready for real-world distribution.

- [ ] `src/notifications/PrintNotifier` (KNotification) wired to
      `PrinterStatus` transitions — print finished, print failed/error code
      present, printer went unexpectedly offline, filament runout if
      surfaced in the report — plus an installed `.notifyrc` declaring the
      event IDs.
- [ ] `package/contents/config/ConfigGeneral.qml` completed (poll interval,
      notification toggles, default connection mode) backed by KConfigXT.
- [ ] Translations (`po/`, `ki18n_install`).
- [ ] AppStream metainfo XML (KDE Store / distro catalog discoverability).
- [ ] Packaging files as stretch goals: Fedora COPR `.spec`, AUR `PKGBUILD`.
- [ ] Accessibility pass: keyboard navigation in the popup, screen-reader
      labels.
- [ ] README finalized: LAN vs Cloud setup instructions, the TOFU security
      model explained, the cloud-API-is-unofficial disclaimer front and
      center.
- [ ] Full manual soak test: multiple printers (mixed LAN/Cloud), printer
      offline/online transitions, wallet locked/unlocked, Plasma restart
      persistence of the printer list and reconnection behavior.
