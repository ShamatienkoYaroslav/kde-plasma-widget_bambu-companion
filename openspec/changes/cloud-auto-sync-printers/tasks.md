## 1. Auto-sync in `PrinterRegistry`

- [x] 1.1 Add a `CloudDeviceDirectory *m_cloudDeviceDirectory` member to
      `src/core/PrinterRegistry`, constructed once in the constructor
      (`this`-parented).
- [x] 1.2 In the constructor, after `loadPersistedPrinters()`: connect
      `m_cloudDeviceDirectory`'s `devicesReady` to a new private
      `syncCloudDevices(const QList<CloudDeviceInfo> &)`; connect
      `CloudAuthClient::instance()`'s `loginSucceeded` to call
      `m_cloudDeviceDirectory->fetchDevices()`; and, if
      `CloudAuthClient::instance().isLoggedIn()` already, call
      `fetchDevices()` immediately.
- [x] 1.3 Implement `syncCloudDevices()`: for each `CloudDeviceInfo`, skip if
      any existing profile's `serial` already matches `device.devId`;
      otherwise call the existing `addCloudPrinter(device.devId,
      device.name)` path. Never removes a profile absent from the fetched
      list.
- [x] 1.4 Implement `clearAllPrinters()` on `PrinterRegistry`: remove every
      currently-configured profile (LAN and Cloud) via the existing
      `removePrinter()`. Connect `CloudAuthClient::loggedOut` to it in the
      constructor, so logging out clears the whole list. Also call it
      directly from the constructor (after `loadPersistedPrinters()`) when
      the account is not logged in and one or more profiles were loaded, so
      leftover printers from before this behavior existed don't linger
      indefinitely in an already-logged-out session.

## 2. Trim now-unused QML-facing surface

- [x] 2.1 Remove `PrinterController::addLanPrinter()` (`.h`/`.cpp`) — no
      longer called from any QML.
- [x] 2.2 Remove `CloudAccountController::fetchDevices()`/
      `addCloudPrinter()` and the `devicesReady`/`fetchFailed` signals
      (`.h`/`.cpp`) — the device picker they served no longer exists; sync
      now happens in `PrinterRegistry` directly against
      `CloudDeviceDirectory`, not through this QML-facing class.
- [x] 2.3 Update `tests/printerlistmodeltest.cpp`'s
      `reflectsAddedPrinter()` (which currently calls
      `PrinterController::addLanPrinter`) to call
      `PrinterRegistry::instance().addLanPrinter(...)` directly instead,
      since the QML-facing method is being removed.

## 3. UI: remove Add Printer, update empty state

- [x] 3.1 Delete `package/contents/ui/AddPrinterDialog.qml`.
- [x] 3.2 Update `main.qml`: remove the `AddPrinterDialog` instantiation and
      its `cloudLoginRequested` wiring; remove the "Add Printer…"
      `ToolButton` from the list header.
- [x] 3.3 Update `main.qml`'s empty-state `ColumnLayout` (currently "No
      printers added yet" + an Add Printer button): when
      `!CloudAccountController.loggedIn`, show a message + a "Log into Bambu
      Cloud" button that calls `cloudLoginDialog.open()` directly; when
      `CloudAccountController.loggedIn` and the list is still empty, show a
      "No printers found on your Bambu account" message with no button.
- [x] 3.4 Add a small, persistent way to reach `CloudLoginDialog`/log out
      once printers already exist (so a logged-in user isn't stuck with no
      way to log out, and a logged-out user with existing LAN printers can
      still log in): a header `ToolButton` toggling between "Log into Bambu
      Cloud…" (calls `cloudLoginDialog.open()`) and "Log out of Bambu
      Cloud" (calls `CloudAccountController.logout()`) based on
      `CloudAccountController.loggedIn`.

## 4. Verification

- [x] 4.1 Build locally: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake
      --build build && ctest --output-on-failure --test-dir build`.
- [x] 4.2 Reinstall: `sudo cmake --install build` + `kpackagetool6 --type
      Plasma/Applet -u package`, restarting `plasmashell` if needed.
- [x] 4.3 Manually: with no Bambu account logged in and zero printers,
      confirm the popup shows the login prompt (no Add button anywhere);
      log in and confirm the account's devices get added automatically
      without any picker step; restart `plasmashell` while still logged in
      and confirm sync runs again on startup without duplicating already-
      synced printers; log out and confirm every printer (LAN and Cloud) is
      removed, its config group and any KWallet secret deleted; log back in
      and confirm the account's current devices get re-synced.
- [ ] 4.4 Confirm CI still passes.
