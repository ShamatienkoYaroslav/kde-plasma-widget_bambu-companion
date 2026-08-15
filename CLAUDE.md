# CLAUDE.md

Guidance for Claude Code (or any AI agent) working in this repository.

## What this project is

Bambu Companion is a **KDE Plasma 6 System Tray widget** that shows and controls a
list of the user's Bambu Lab 3D printers — live status, temperatures, progress,
plate-preview thumbnails, pause/resume/stop, and (later) a camera view. It's
inspired by [ypMrg/omarchy-bambu-companion](https://github.com/ypMrg/omarchy-bambu-companion)
(a Quickshell/Hyprland widget in QML+Ruby, LAN-only, single-printer, monitoring-only)
but broader in scope: multi-printer, LAN **and** Bambu Cloud connectivity, and print
control commands.

Full phased roadmap: see [`PLAN.md`](./PLAN.md). This project is built **one phase
task at a time via OpenSpec** — see "Development process" below before starting any
implementation work.

## Tech stack

- **C++20**, **Qt6** (Core, Network, Qml, Quick), **KDE Frameworks 6** (CoreAddons,
  I18n, Config, ConfigWidgets, KCMUtils, Package, Notifications, Wallet, Archive)
- **QML** for all UI (tray glyph, popup, config pages)
- **CMake + ECM** (extra-cmake-modules) build system
- **libmosquitto** for MQTT (not Qt6's QtMqtt — it's not packaged on Debian/Ubuntu;
  libmosquitto is available everywhere)
- **libcurl** for FTPS plate-preview retrieval (Qt6 removed FTP support entirely)
- **KWallet** for secrets (LAN access codes, Bambu Cloud tokens) — never store
  secrets via KConfigXT/plain config files
- Packaged as a **Plasma 6 Applet** (a KPackage under `package/`) paired with a
  compiled QML plugin (`src/qmlplugin/`, built via `ecm_add_qml_module`)

Namespace everything `io.github.shamatienkoyaroslav.bambucompanion` — **not**
`org.kde.*`, which is reserved for KDE-maintained software.

## Architecture

```
src/core/            PrinterProfile, PrinterStatus, PrinterCommand,
                      PrinterConnection (abstract interface), PrinterRegistry,
                      ConnectionFactory
src/transport/lan/    MqttClient (libmosquitto + TLS), LanPrinterConnection,
                      FtpsFileClient (libcurl), LanCameraSource (phase 4)
src/transport/cloud/  CloudAuthClient, CloudDeviceDirectory, CloudPrinterConnection
src/protocol/         BambuReportParser, BambuCommandBuilder (shared by LAN + Cloud)
src/security/         SecretStore (KWallet), CertificateProbe, CertificateTrustStore
                      (LAN trust-on-first-use certificate pinning)
src/notifications/    PrintNotifier (KNotification)
src/qmlplugin/         PrinterListModel, PrinterController (QML_ELEMENT types)
package/               metadata.json + contents/ui/*.qml + contents/config/*.qml
                      (the installable Plasma Applet KPackage)
tests/                 protocol/, core/, fixtures/ (sample printer report JSON)
po/                    translations
```

**Key design point:** `PrinterConnection` is an abstract interface with two
implementations, `LanPrinterConnection` (local MQTT over TLS, self-signed
certs pinned TOFU-style) and `CloudPrinterConnection` (Bambu Cloud's
unofficial/reverse-engineered REST auth + relay MQTT, publicly-trusted certs).
Both share `BambuReportParser`/`BambuCommandBuilder` so the two transports are
behaviorally identical from the UI's point of view. `PrinterRegistry` owns the
printer list and each printer's connection mode (LAN-only / Cloud-only /
prefer-LAN-then-cloud), handling fallback logic itself so neither connection
implementation needs to know about the other.

## Build & test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --output-on-failure --test-dir build
```

Manual UI testing (unit tests can't exercise Plasma shell integration or real
printers). Two separate install steps are needed — `kpackagetool6` only
installs the declarative `package/` content; the compiled QML plugin
(`bambucompanionplugin`) needs a regular CMake install into Qt's system QML
import path, which typically requires root:

```sh
sudo cmake --install build                      # installs the compiled QML plugin
kpackagetool6 --type Plasma/Applet -i package    # first install of the KPackage
kpackagetool6 --type Plasma/Applet -u package    # after changes to package/
```

After the first `sudo cmake --install build`, only `cmake --build build` +
`sudo cmake --install build` are needed when the plugin's C++ changes; only
`kpackagetool6 -u package` is needed when only `package/contents` changes.

Then add the widget from Plasma's "Add Widgets" dialog / system tray settings.

## Development process — OpenSpec, one task at a time

This repo uses [OpenSpec](https://github.com/Fission-AI/OpenSpec) for spec-driven
development. **Do not start implementing a feature ad hoc.** Instead:

1. Pick the next unstarted task from `PLAN.md`'s phase task list (work phases in
   order; within a phase, follow the listed component order).
2. Run `/opsx:propose "<task description>"` to generate the change's planning
   artifacts (proposal, spec deltas, design, tasks) — this step is planning-only,
   it does not touch project code.
3. Review the generated artifacts under `openspec/changes/<change-id>/`.
4. Run `/opsx:apply` (or ask to apply the change) to implement it.
5. Run `/opsx:archive` once the change is implemented, tested, and merged, to
   fold its spec deltas into `openspec/specs/` and archive the change.

`openspec/config.yaml` holds the project-wide tech-stack/architecture context
that gets fed to every artifact-generation step — keep it in sync with this file
if the architecture changes.

## Security & scope notes

- **LAN transport** connects directly to a printer's local MQTT (port 8883) and
  FTPS (port 990) services using the printer's serial number and LAN access
  code. Printers use self-signed TLS certificates; we pin them trust-on-first-use
  (like SSH host keys) rather than skipping verification silently.
- **Cloud transport** is built against Bambu Lab's **unofficial, reverse-engineered**
  cloud API (no public documentation from Bambu Lab). It may change or break
  without notice — this must stay visible in the README and in the cloud-login UI,
  not just buried in code comments.
- Never persist LAN access codes or cloud tokens outside KWallet.
