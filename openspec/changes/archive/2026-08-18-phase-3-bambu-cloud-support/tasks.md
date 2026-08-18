## 1. Cloud auth (`src/transport/cloud/`)

- [x] 1.1 Add `src/transport/cloud/CloudAuthClient.h`/`.cpp`: `QObject` using
      `QNetworkAccessManager`; `Q_INVOKABLE`-free plain API `login(email,
      password)`, `submitVerificationCode(code)`, `logout()`,
      `bool isLoggedIn() const`, `QString accessToken() const`. Signals:
      `loginSucceeded()`, `twoFactorRequired()`, `loginFailed(QString
      reason)`.
      (Singleton, like `PrinterRegistry`/`SecretStore` — `CloudDeviceDirectory`
      and `CloudPrinterConnection` need to share the one account's state.
      Response parsing extracted into a static, directly-testable
      `parseLoginResponse()` rather than only living inside the network
      callback.)
- [x] 1.2 Implement the login POST + `loginType: "verifyCode"` branch per
      `design.md`; on success, store the token via `SecretStore` under a new
      `cloud-token` key (extend `SecretStore` with
      `storeCloudToken`/`cloudToken`/`removeCloudToken`, mirroring the
      existing LAN-access-code methods).
      (Also implemented the `sendemail/code` request that actually triggers
      Bambu to email the verification code, per `PLAN.md`'s fuller protocol
      reference — `design.md`'s summary of this step was abbreviated but
      didn't intend to skip it.)
- [x] 1.3 Add redacted diagnostic logging (HTTP status + response JSON with
      `accessToken`/`refreshToken`/`password`/`code` values replaced with
      `"<redacted>"`) to a local file under `QStandardPaths::CacheLocation`,
      per `design.md`'s safe-verification approach — never log raw
      credentials or tokens.
- [x] 1.4 On any subsequent cloud HTTP call receiving `401`, clear the stored
      token and emit a "needs login again" signal rather than guessing at a
      refresh flow.
      (Implemented as `CloudAuthClient::handleHttpStatus()`, a shared,
      directly-testable method other cloud transport classes call when they
      see a 401 on an authenticated request.)

## 2. Device directory

- [x] 2.1 Add `src/transport/cloud/CloudDeviceDirectory.h`/`.cpp`:
      `fetchDevices()` — `GET .../api/user/bind` with the stored bearer
      token, parses `dev_id`/`name`/`online` into a
      `QList<CloudDeviceInfo>` (small value type), emits `devicesReady(...)`
      or `fetchFailed(reason)`. Parsing extracted into a static
      `parseDevices()`, same testable-without-network pattern as
      `CloudAuthClient::parseLoginResponse()`.

## 3. `ConnectionMode` + registry/factory wiring

- [x] 3.1 Extend `src/core/PrinterProfile.h`: `enum class ConnectionMode {
      LanOnly, CloudOnly, PreferLanThenCloud }` field, defaulting to
      `LanOnly` (existing profiles keep working unchanged).
- [x] 3.2 Extend `PrinterRegistry`'s persistence (`persistProfile`/
      `loadPersistedPrinters`) to read/write the new field (default to
      `LanOnly` when absent, for backward compatibility with Phase 1/2
      config files).
      (Also relaxed `loadPersistedPrinters`'s validation, which previously
      required a non-empty `Host` unconditionally — Cloud-only profiles have
      no host — to only require it for non-Cloud-only modes.)
- [x] 3.3 Update `ConnectionFactory::create()` to be mode-aware: `LanOnly` →
      `LanPrinterConnection`; `CloudOnly` → `CloudPrinterConnection`;
      `PreferLanThenCloud` → `LanPrinterConnection` initially.
- [x] 3.4 Add fallback logic in `PrinterRegistry::startConnection()`: for a
      `PreferLanThenCloud` profile, if the LAN connection's state settles
      into `ConnectionState::Error`, construct and start a
      `CloudPrinterConnection` for the same profile, swapping it in as that
      printer's active connection (disconnecting/discarding the failed LAN
      one).
      (Factored the connect-signals-and-start step shared by both the normal
      start path and the fallback path into `wireAndStart()`; a
      `QSet<QUuid>` guards against falling back more than once per printer.)
- [x] 3.5 Add `PrinterRegistry::addCloudPrinter(devId, name)` (parallel to
      `addLanPrinter`), creating a profile with `ConnectionMode::CloudOnly`
      (LAN/fallback mode selection deferred to a later UI refinement — Cloud
      printers added via the picker default to Cloud-only for now).

## 4. `CloudPrinterConnection`

- [x] 4.1 Add `src/transport/cloud/CloudPrinterConnection.h`/`.cpp`:
      implements `PrinterConnection` (`start()`, `stop()`,
      `connectionState()`, `status()`, `sendCommand()` — no
      `confirmCertificateTrust()` override, inherits the no-op default).
      Owns an `MqttClient` connected to `<region>.mqtt.bambulab.com:8883`
      (region hardcoded to `us` per `design.md`) using the best-known
      user-id-derived auth frame; on connect, subscribes/publishes the same
      `device/{serial}/...` topics as LAN. Also extended `MqttClient` with a
      `useSystemCaTrust` option (default off, LAN unaffected) so the cloud
      relay's publicly-trusted certificate is actually validated rather than
      reusing LAN's insecure mode.
- [x] 4.2 Reuse `BambuReportParser::merge()` for incoming reports and a
      `PendingCommandTracker` instance for command-ack correlation,
      identically to `LanPrinterConnection` (extract any now-shared plumbing
      into a small helper only if it stays a straightforward reuse — don't
      force a premature base-class refactor if the two implementations
      diverge in how they wire it up).
      (The `sendCommand()` payload-building switch was identical in both
      classes, so extracted into `BambuCommandBuilder::build()`; the
      report-merge/ack-check block stays duplicated per-class as allowed
      above, since connection setup around it differs.)
- [x] 4.3 Update `ConnectionFactory::create()` (from 3.3) to actually
      construct `CloudPrinterConnection` for `CloudOnly`/fallback cases.

## 5. QML-facing controller

- [x] 5.1 Add to `PrinterController` (or a new `CloudAccountController`
      `QML_SINGLETON` if `PrinterController` is getting crowded — decide
      based on how large it gets): `Q_INVOKABLE login(email, password)`,
      `submitVerificationCode(code)`, `logout()`, `Q_INVOKABLE
      fetchCloudDevices()`, `Q_INVOKABLE addCloudPrinter(devId, name)`.
      Signals relaying `CloudAuthClient`/`CloudDeviceDirectory`'s outcomes to
      QML (`loginSucceeded`, `twoFactorRequired`, `loginFailed`,
      `cloudDevicesReady`, `cloudFetchFailed`).
      (Went with a new `CloudAccountController` singleton — account-level
      concerns are conceptually distinct from `PrinterController`'s
      per-printer actions, matching the existing model/controller split.
      Signals are named `devicesReady`/`fetchFailed` without a `cloud` prefix
      since the whole class is already cloud-scoped;
      `fetchDevices()`/`login()`/etc. likewise unprefixed for the same
      reason.)

## 6. UI

- [x] 6.1 Add `package/contents/ui/CloudLoginDialog.qml`: email/password
      fields, a verification-code step shown only when
      `twoFactorRequired` fires, and a visible, permanent notice that this
      uses an unofficial/unsupported Bambu Cloud API (per the
      `cloud-account` spec's disclosure requirement).
- [x] 6.2 Extend `AddPrinterDialog.qml` with a "Log into Bambu Cloud…" entry
      point (opens `CloudLoginDialog`) and, once logged in, a "pick from my
      account" list (from `fetchCloudDevices()`) alongside the existing
      manual LAN fields — both paths stay in the same dialog rather than
      forking into two separate flows.
      (Added a two-button toggle at the top switching between the LAN form
      and the Cloud picker. Required promoting `CloudAccountController::
      isLoggedIn()` from a plain invokable to a NOTIFYing `loggedIn`
      property, since a QML binding referencing an invokable call isn't
      reactive — needed for the picker to actually update when login
      completes.)

## 7. Tests

- [x] 7.1 Add `tests/cloudauthclienttest.cpp`: login success, `verifyCode`
      branch, login failure, and 401-clears-token behavior, all against a
      mocked/injected HTTP backend (no real network call) — e.g. by
      constructing `CloudAuthClient` with an injectable
      `QNetworkAccessManager` or reply factory, whichever is simplest given
      Qt's testing patterns for `QNetworkAccessManager`.
      (No mocked `QNetworkAccessManager` needed — `parseLoginResponse()` was
      already extracted as a static, directly-testable function for exactly
      this reason. The 401 case tests `handleHttpStatus()`'s post-condition,
      which holds regardless of whether KWallet is actually available, same
      caveat as `printerregistrytest.cpp`.)
- [x] 7.2 Add `tests/connectionfactorytest.cpp` (or extend
      `printerregistrytest.cpp`): a `PrinterProfile` with each
      `ConnectionMode` produces the expected connection type; a
      `PreferLanThenCloud` profile whose LAN connection reports `Error`
      results in a `CloudPrinterConnection` becoming active.
      (Did both: `connectionfactorytest.cpp` for the fast/deterministic
      type-dispatch checks; a new test in `printerregistrytest.cpp` for the
      fallback path, using a loopback address nothing listens on for a fast
      connection-refused failure. Required adding a `mode` parameter to
      `PrinterRegistry::addLanPrinter()` — `PreferLanThenCloud` had no way to
      actually be created before this, since `addLanPrinter` hardcoded
      `LanOnly`; without it, the fallback requirement in the
      `printer-management` spec delta would be unreachable and untestable.
      The parameter defaults to `LanOnly`, so no existing call site changes.)
- [x] 7.3 Wire new test sources into `tests/CMakeLists.txt`.

## 8. Verification

- [x] 8.1 Build locally: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake
      --build build && ctest --output-on-failure --test-dir build`.
      (9/9 tests pass, including under a simulated headless environment
      matching CI.)
- [x] 8.2 Reinstall: `sudo cmake --install build` + `kpackagetool6 --type
      Plasma/Applet -u package`, restarting `plasmashell` if needed.
      (Also found and fixed a real QML bug this pass: `CloudLoginDialog` was
      declared as a nested child inside `AddPrinterDialog`'s
      `Kirigami.OverlaySheet`, which sweeps default children into the outer
      sheet's flickable content — badly mis-sizing a nested Popup-derived
      type. Fixed by hoisting `CloudLoginDialog` to a sibling in `main.qml`,
      matching the already-working pattern for `AddPrinterDialog`/
      `CertificateConfirmDialog`, with `AddPrinterDialog` emitting a
      `cloudLoginRequested()` signal instead of opening it directly.)
- [x] 8.3 Manually, through the applet's own `CloudLoginDialog` (never by
      typing credentials into this conversation): log into a real Bambu
      account; confirm the 2FA step appears if the account requires it;
      confirm the device list populates; add a printer from it; confirm live
      status appears via the cloud relay. If login or the MQTT relay
      connection fails, share the redacted diagnostic log (task 1.3) rather
      than raw credentials/tokens, so the auth-frame assumption in
      `design.md` can be corrected.
      (Confirmed working end-to-end against a real account: login, 2FA code
      step, device list populated, 6 printers added and persisted correctly
      with `Mode=CloudOnly`. The redacted diagnostic log confirmed the real
      response shape matches `design.md`'s assumption almost exactly
      — `accessToken`/`loginType`/`refreshToken` fields as expected. Note:
      the log path resolves under `~/.cache/plasmashell/` rather than a
      dedicated app cache dir, since the plugin runs inside plasmashell's
      process — expected/acceptable, not a bug.

      **Follow-up finding: live cloud MQTT status does NOT work.** All
      6 added printers showed Offline/Error. Root-caused via added
      diagnostic logging (kept permanently in the code, not stripped after):
      the access token is an opaque string, not a JWT (the original
      assumption); the follow-up `my/info` endpoint guess for the account
      uid 404s; neither the login response nor the device-list response
      carry a uid anywhere. So the `"u_<uid>"` cloud MQTT username scheme is
      unconfirmed, not just the uid lookup. Along the way, found and fixed a
      real bug where a failed uid lookup could clear a valid session's
      token (see `design.md`'s Risks — `handleHttpStatus()` is no longer
      called from the uid-lookup path). User chose to pause further
      endpoint-guessing and accept this as a documented, open gap rather
      than continue burning rebuild/install/restart cycles on speculative
      endpoints — see `design.md`'s updated Risks entry.
- [x] 8.4 If a `PreferLanThenCloud` printer is available, verify the fallback
      path: disconnect it from LAN (or use a printer only reachable via
      Cloud) and confirm it comes up via the Cloud connection instead.
      **Blocked by 8.3's finding**: the fallback code path itself is
      exercised by the automated `preferLanThenCloudFallsBackAfterLanFails`
      test (task 7.2), but a live UI-level demonstration would just show the
      same Cloud MQTT auth failure documented above, not a meaningful
      additional confirmation. Revisit once cloud MQTT auth is resolved.
- [x] 8.5 Confirm CI still passes (the new unit tests need no real network
      access or credentials).
