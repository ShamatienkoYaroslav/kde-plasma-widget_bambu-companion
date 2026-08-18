## Why

Phases 1–2 only reach printers on the local network. Many users' printers
aren't always LAN-reachable (different network, printer not in LAN-only
mode, travel), and the project's own testing has been blocked by exactly
this. Phase 3 of `PLAN.md` adds Bambu Cloud as a second transport: log into
a Bambu account, list the account's printers, and monitor/control them
through Bambu's cloud MQTT relay — reusing every transport-agnostic piece
already built (`PrinterConnection`, `BambuReportParser`,
`BambuCommandBuilder`, `printer-status-monitoring`, `printer-control`)
rather than rebuilding them.

## What Changes

- Add `src/transport/cloud/CloudAuthClient`: email/password login against
  Bambu's cloud API, with a 2FA (email verification code) branch, token
  storage via `SecretStore`, and re-login (not refresh — undocumented) on
  expiry.
- Add `src/transport/cloud/CloudDeviceDirectory`: fetches the account's bound
  device list for the "add cloud printer" picker.
- Add `src/transport/cloud/CloudPrinterConnection`: implements
  `PrinterConnection` (including `sendCommand()`) against the cloud MQTT
  relay, reusing `BambuReportParser`/`BambuCommandBuilder` unchanged — no
  certificate TOFU flow (the relay uses a publicly-trusted certificate).
- Extend `PrinterProfile` with a `ConnectionMode` (`LanOnly`, `CloudOnly`,
  `PreferLanThenCloud`); `ConnectionFactory`/`PrinterRegistry` pick and, for
  `PreferLanThenCloud`, fall back between `LanPrinterConnection` and
  `CloudPrinterConnection` — fallback logic lives in `PrinterRegistry`, not
  either connection class.
- Add UI: `CloudLoginDialog.qml` (email/password, 2FA step, explicit
  "unofficial API" disclaimer) and a "pick from my Bambu account" path in
  `AddPrinterDialog.qml`.
- **Spike first**: before building `CloudPrinterConnection`, manually verify
  the cloud MQTT relay's auth frame and topic conventions against a real
  Bambu account — this is the least-confirmed part of the whole protocol
  reference in `PLAN.md`/`design.md`, unlike the LAN protocol which Phase 1
  validated live. See `design.md` for how this is done without putting real
  account credentials in this conversation.
- **No plate-preview thumbnails, no camera** — still out of scope per
  `PLAN.md`'s phase boundaries.

## Capabilities

### New Capabilities
- `cloud-account`: logging into a Bambu Cloud account (with 2FA support),
  secure token storage, logout, and keeping the unofficial-API status visible
  to the user.

### Modified Capabilities
- `printer-management`: adds the ability to add a printer by picking it from
  a logged-in Bambu account's device list (alongside the existing manual LAN
  entry), and adds the `ConnectionMode` concept governing whether a given
  printer connects via LAN, Cloud, or LAN-with-Cloud-fallback.

## Impact

- Affected code: `src/transport/cloud/` (new), `src/core/PrinterProfile.h`
  (new `ConnectionMode` field), `src/core/ConnectionFactory`,
  `src/core/PrinterRegistry` (mode-aware connection creation + fallback),
  `src/qmlplugin/PrinterController` (cloud login invokables),
  `package/contents/ui/` (`CloudLoginDialog.qml`,
  `AddPrinterDialog.qml` extended).
- New dependency: `Qt6::Network` is already linked (used by
  `CertificateProbe`); cloud HTTP calls reuse `QNetworkAccessManager`, no new
  library needed.
- `printer-status-monitoring` and `printer-control` are **not modified** —
  both were already written transport-agnostically in Phases 1–2
  specifically so `CloudPrinterConnection` could satisfy their existing
  requirements without changes.
