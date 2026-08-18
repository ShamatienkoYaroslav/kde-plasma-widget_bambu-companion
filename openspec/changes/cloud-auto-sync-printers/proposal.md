## Why

Phases 1–3 let a user add printers manually (LAN form, or a picker inside
"Add Printer" for cloud-account devices). Once a user is logged into their
Bambu Cloud account, this is redundant friction — the account already knows
which printers they own. The applet should instead just show them
automatically: check login state on startup, and whenever the account is
logged in (at startup or right after a fresh login), fetch its device list
and add any printer not already configured. This removes the "Add Printer"
button and the LAN manual-entry flow entirely.

## What Changes

- **Remove** `package/contents/ui/AddPrinterDialog.qml` and the "Add
  Printer…" entry points (list-header button, empty-state button) — no more
  manual LAN entry, no more manual device picker.
- **Add** automatic cloud sync in `PrinterRegistry`: on construction (i.e.
  applet/app startup) and whenever `CloudAuthClient::loginSucceeded` fires,
  fetch the logged-in account's bound devices and add any whose serial isn't
  already a configured printer. Never removes a printer that drops off the
  account's device list — removal stays a manual action via the existing
  config page.
- **Change** the full representation's empty state: shows a "Log into Bambu
  Cloud" prompt (opening `CloudLoginDialog` directly) when logged out;
  when logged in with zero synced printers, shows an informational "no
  printers found on your account" message instead (no button — sync already
  happened/happens automatically).
- **Remove** now-unused QML-facing surface: `PrinterController::addLanPrinter`,
  and `CloudAccountController::fetchDevices()`/`addCloudPrinter()`/
  `devicesReady`/`fetchFailed` (the picker they served no longer exists; the
  sync logic moves into `PrinterRegistry` in C++, not QML-driven).
- **Add** clear-on-logout: logging out of Bambu Cloud removes every
  configured printer (LAN and Cloud alike), since the printer list is now
  entirely account-sourced — a logged-out state has no source of truth for
  which printers exist.
- **Keep unchanged**: `LanPrinterConnection`, LAN TOFU trust flow,
  `PrinterRegistry::addLanPrinter()` (the core method — still used
  internally/by tests). `ConfigPrinters.qml`'s remove action remains the
  only *manual* way to remove a single printer; logout now also clears the
  whole list automatically.

## Capabilities

### Modified Capabilities
- `printer-management`: removes "User can add a LAN printer" and "User can
  add a printer from their Bambu Cloud account" (both were manual-entry UI
  requirements); adds "The configured printer list automatically syncs with
  a logged-in Bambu Cloud account's devices" and "Logging out of Bambu
  Cloud clears the configured printer list".
- `tray-applet-shell`: the empty-state scenario of "Full representation
  renders live printer list content" changes from "prompt inviting the user
  to add a printer" to a login-state-aware empty state (log-in prompt vs.
  no-printers-on-account message).

## Impact

- Affected code: `package/contents/ui/AddPrinterDialog.qml` (deleted),
  `package/contents/ui/main.qml` (empty state, header, dialog wiring),
  `src/core/PrinterRegistry` (owns the auto-sync trigger + add-if-new logic),
  `src/qmlplugin/PrinterController` and `src/qmlplugin/CloudAccountController`
  (trimmed to only what's still QML-reachable).
- No new dependencies. No changes to `LanPrinterConnection`,
  `CloudPrinterConnection`, `BambuReportParser`, or `BambuCommandBuilder`.
- Existing LAN printers already configured before this change are affected:
  they are left alone while the account stays logged in, but are deleted
  (config + KWallet secret) the next time the user logs out of Bambu Cloud.
