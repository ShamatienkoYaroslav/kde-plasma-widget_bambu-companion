## Why

Phase 0 delivered an installable applet shell with hardcoded placeholder
printer data. The widget is not yet useful — it can't talk to a real printer.
Phase 1 of `PLAN.md` adds the first real capability: connect to a Bambu Lab
printer over the local network, parse its live MQTT status reports, and show
that status in the tray popup. This is the foundation every later phase
(control commands, thumbnails, cloud, camera) builds on.

## What Changes

- Add the core domain layer: `PrinterProfile` (a configured printer's
  connection details), `PrinterStatus` (a value type for parsed printer
  state), `PrinterConnection` (abstract interface — LAN today, Cloud in
  Phase 3), `PrinterRegistry` (owns the printer list, persists non-secret
  profile fields via `KSharedConfig`, LAN-only for now), `ConnectionFactory`
  (currently only builds `LanPrinterConnection`).
- Add LAN transport: `MqttClient` (libmosquitto wrapper with TLS),
  `LanPrinterConnection` (subscribes to `device/{serial}/report`, publishes
  `pushall` on connect).
- Add LAN security: `CertificateProbe` + `CertificateTrustStore` (TOFU pinning
  of the printer's self-signed TLS certificate, with a confirmation dialog),
  `SecretStore` (KWallet-backed storage of the LAN access code).
- Add `BambuReportParser` (JSON → `PrinterStatus`, merging partial/delta
  reports into existing state rather than overwriting).
- Replace the Phase 0 stub `PrinterListModel` with one backed by
  `PrinterRegistry`'s real, live-updating printer list; add `PrinterController`
  (`Q_INVOKABLE addLanPrinter(...)`) as the QML-facing entry point for adding
  printers.
- Add real UI: an "Add printer" dialog (LAN fields only), a certificate
  trust-confirmation dialog, and a detail view showing a printer's live state,
  progress, temperatures, fan speeds, speed profile, WiFi signal, and
  layer/Z progress.
- **No control commands** (pause/resume/stop) and **no Bambu Cloud** in this
  phase — those are Phase 2 and Phase 3.

## Capabilities

### New Capabilities
- `printer-management`: adding a LAN printer (host, serial, access code),
  first-connection certificate trust confirmation, and persisting the
  configured printer list (non-secret fields via config, access code via
  KWallet) across Plasma restarts.
- `printer-status-monitoring`: parsing live MQTT status reports into
  structured printer state (state, progress, temperatures, fans, speed
  profile, WiFi signal, layer/Z) and keeping it current as delta reports
  arrive, including reconnecting after the printer or connection drops.

### Modified Capabilities
- `tray-applet-shell`: the "Full representation renders placeholder printer
  list content" requirement is replaced by "Full representation renders live
  printer list content" — the popup now renders each configured printer's
  real live status (via `printer-status-monitoring`) instead of hardcoded
  placeholder rows, while still handling the case of zero printers configured
  gracefully (prompting the user to add one, rather than showing fake data).

## Impact

- Affected code: `src/core/`, `src/transport/lan/`, `src/security/`,
  `src/protocol/`, `src/qmlplugin/` (real model + controller replacing the
  Phase 0 stub), `package/contents/ui/` (new dialogs + detail view),
  `package/contents/config/` (printers config page).
- New dependencies: `libmosquitto` (pkg-config), `KF6::Wallet`,
  `KF6::Config`/`KF6::ConfigWidgets`/`KF6::KCMUtils`, `Qt6::Network` (for
  `QSslSocket`-based certificate probing). None of these were needed in
  Phase 0's minimal dependency set.
- No cloud, no control commands, no plate-preview thumbnails, no camera —
  explicitly out of scope per `PLAN.md`'s phase boundaries.
