## Context

See `proposal.md` for motivation. Phase 3 (archived) added `CloudAuthClient`/
`CloudDeviceDirectory`/`CloudAccountController`, with the device list only
ever used inside `AddPrinterDialog.qml`'s manual picker. This design moves
that fetch-and-add logic out of QML and into `PrinterRegistry`, triggered
automatically instead of by a button.

## Goals / Non-Goals

**Goals:**
- No manual "Add Printer" UI at all — LAN form and Cloud picker both gone.
- Auto-sync runs at the two points that matter: app startup (if already
  logged in) and immediately after a fresh login — no periodic polling.
- Sync only *adds* printers, never removes one on its own (no periodic
  reconciliation while logged in).
- The printer list is entirely account-scoped: logging out of Bambu Cloud
  clears the whole configured printer list (LAN and Cloud alike), since
  without a logged-in account there is no source of truth for who owns
  which printer.

**Non-Goals:**
- No periodic/background re-sync while the popup is closed — only the two
  trigger points above job in this phase; a "manual refresh" affordance can
  be added later if the account's device list changes and a restart is
  inconvenient.
- Does not resolve the Phase 3 cloud-MQTT-auth gap (`design.md` in
  `archive/2026-08-18-phase-3-bambu-cloud-support/`) — auto-synced printers
  will still show `Error`/offline until that's fixed. This change is purely
  about *how printers get added to the list*, not about making cloud MQTT
  connectivity work.
- No confirmation prompt before logout clears the list — logout is already
  an explicit user action reachable only from the header toggle; a second
  confirmation step was judged unnecessary friction. Revisit if this proves
  surprising in practice.

## Decisions

**Sync logic lives in `PrinterRegistry`, not QML.** `PrinterRegistry` gains
a `CloudDeviceDirectory` member (owned, constructed once) and, in its
constructor: (a) after `loadPersistedPrinters()`, if
`CloudAuthClient::instance().isLoggedIn()`, calls `fetchDevices()`; (b)
connects `CloudAuthClient::loginSucceeded` to also call `fetchDevices()`.
`CloudDeviceDirectory::devicesReady` is connected to a new private
`syncCloudDevices(const QList<CloudDeviceInfo> &)` method: for each device,
if no existing profile's `serial` matches `device.devId`, call the existing
`addCloudPrinter()` path (unchanged — still persists + starts a connection
for the new printer). This keeps the logic testable and independent of
whether/how many times the popup has been opened, unlike the old
QML-Connections-triggered approach in `AddPrinterDialog.qml`.

**Startup means `PrinterRegistry` construction**, which (per Phase 1/2/3
live testing) happens as soon as the applet's `main.qml` loads inside
`plasmashell` — not deferred until the user opens the popup. This matches
"on app started" from the request without needing a separate Plasmoid
lifecycle hook.

**Matching key is `serial` (== the cloud device's `dev_id`)**, the same
field already used to identify a printer everywhere else in the codebase
(LAN and Cloud profiles both populate it) — no new identity concept needed.

**Removed QML-facing surface**: `PrinterController::addLanPrinter()` (only
caller was `AddPrinterDialog.qml`) and `CloudAccountController::
fetchDevices()`/`addCloudPrinter()`/`devicesReady`/`fetchFailed` (only
caller was the same dialog's device picker). The underlying C++
`PrinterRegistry::addLanPrinter()`/`addCloudPrinter()` methods stay — the
first remains a legitimate internal/test API (and the mechanism by which
already-configured LAN printers keep working), the second is now called
directly by `syncCloudDevices()` instead of via QML.

**Empty-state UI** reads `CloudAccountController.loggedIn` (already a
NOTIFYing property from Phase 3) to choose between a login prompt and a
"no printers on this account" message — no new C++ surface needed for this
part.

**Logout clears the printer list.** `PrinterRegistry` connects
`CloudAuthClient::loggedOut` to a new private `clearAllPrinters()` method,
which calls the existing `removePrinter()` for every currently-configured
profile — LAN and Cloud alike. `removePrinter()` already stops the
connection, deletes the persisted config group, and removes any LAN access
code from KWallet, so no new teardown logic is needed; `clearAllPrinters()`
just iterates all profile ids and calls it for each. This applies to
printers configured before this change too (e.g. pre-existing LAN
printers) — the account-sourced model means there is no printer identity
independent of a logged-in account once this change ships. The constructor
applies the same rule at startup: after `loadPersistedPrinters()`, if the
account is *not* logged in and any profiles were loaded, `clearAllPrinters()`
runs immediately rather than only on the next explicit logout — otherwise a
user who already had leftover printers from before this behavior existed
(or from earlier manual testing) would keep seeing them indefinitely, since
no `loggedOut` signal would ever fire for a session that never logs in.

## Risks / Trade-offs

- **[Trade-off]** No way to add a printer that isn't in a logged-in Bambu
  account (e.g. a LAN-only printer whose owner doesn't want a Bambu account
  linked at all) → accepted per the user's explicit choice; revisit with a
  dedicated advanced/manual entry point later if requested.
- **[Risk]** Auto-syncing on every startup means a large account (many
  printers) triggers a fetch on every Plasma session start → acceptable:
  it's a single lightweight HTTP GET, same call `AddPrinterDialog.qml`
  already made on-demand before.
- **[Trade-off]** Logging out is destructive: it deletes every configured
  printer, including LAN printers that predate this change and have no UI
  path to be re-added manually (that path was removed). Accepted per the
  user's explicit instruction — the printer list is meant to be entirely
  derived from the logged-in account, so a logged-out state should show no
  printers rather than stale ones. If a printer's LAN access code/host was
  only ever entered manually and it isn't also bound to the Bambu account,
  logging out permanently loses that configuration (the secret is deleted
  from KWallet along with the profile).
