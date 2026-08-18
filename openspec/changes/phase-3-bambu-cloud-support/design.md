## Context

See `proposal.md` for motivation. Phases 1–2 (archived) built
`PrinterConnection`/`LanPrinterConnection`/`PrinterRegistry`/`PrinterController`
and, critically, wrote `printer-status-monitoring` and `printer-control`
transport-agnostically on purpose. This design covers the second
`PrinterConnection` implementation (`CloudPrinterConnection`) and the account
login flow it depends on — the least-documented surface in the whole project
(`PLAN.md` flags the cloud MQTT auth frame as unconfirmed, unlike the LAN
protocol, which Phase 1 validated against real hardware).

## Goals / Non-Goals

**Goals:**
- A user can log into one Bambu Cloud account, see their bound devices, and
  add one as a printer that monitors/controls exactly like a LAN printer from
  the UI's perspective.
- `CloudPrinterConnection` satisfies the exact same `PrinterConnection`
  contract `LanPrinterConnection` does — no UI or `PrinterRegistry` changes
  needed beyond `ConnectionMode` selection and fallback.
- Real Bambu account credentials never appear in this conversation, in logs
  committed to the repo, or anywhere outside KWallet.

**Non-Goals:**
- No multi-account support — one logged-in Bambu account at a time, matching
  `PLAN.md`'s scope.
- No plate-preview thumbnails, no camera.
- No token refresh endpoint usage — Bambu doesn't document one; re-login on
  expiry instead (per `PLAN.md`).

## Decisions

**Verifying the cloud protocol without exposing credentials**: rather than a
separate pre-implementation "spike" requiring credentials in this
conversation, `CloudAuthClient`/`CloudDeviceDirectory`/`CloudPrinterConnection`
are built against the best-documented shape from `PLAN.md`'s cited sources
(`coelacant1/Bambu-Lab-Cloud-API`, `Doridian/OpenBambuAPI`), then verified the
same way Phase 1 verified the LAN protocol: manually, live, by the user,
through the applet's own UI. The login/password fields are entered directly
into `CloudLoginDialog.qml` — never typed into this chat. If something fails,
`CloudAuthClient` logs enough to diagnose the *shape* of the problem (HTTP
status code, response JSON with `accessToken`/`refreshToken`/`password`
values redacted before logging) to a local file the user can inspect and
paste relevant (redacted) excerpts from, rather than requiring raw
credentials or tokens at any point. This keeps the real-hardware-verification
principle from Phase 1/2 while treating account credentials with more care
than a printer's LAN access code.

**Cloud auth flow (`CloudAuthClient`)**: `POST
https://api.bambulab.com/v1/user-service/user/login` with `{"account":
email, "password": password}` via `QNetworkAccessManager`. Three outcomes:
(a) success → response includes `accessToken`; store it via `SecretStore`
under a new `cloud-token` key (no per-printer scoping — one account). (b)
`loginType: "verifyCode"` → also `POST
.../v1/user-service/user/sendemail/code` with `{"email": email, "type":
"codeLogin"}` (this is what actually triggers Bambu to email the code — the
login response alone doesn't send it), then emit `twoFactorRequired`; UI
prompts for the emailed code; `submitVerificationCode(code)` re-POSTs the
login endpoint with `{"account": email, "code": code}`. (c) failure → emit
`loginFailed(reason)`. The password itself is only ever held in memory for
the duration of the request, never persisted. Response parsing is a static,
pure `parseLoginResponse(httpStatus, body)` — directly unit-testable with
synthetic responses, no mocked `QNetworkAccessManager` needed. On any
subsequent `401` from a cloud API call, `CloudAuthClient::handleHttpStatus()`
(also called by `CloudDeviceDirectory`/`CloudPrinterConnection`, since they
share this one account's session) clears the stored token and emits
`loggedOut()` rather than guessing at an undocumented refresh flow.

**Device directory**: `CloudDeviceDirectory::fetchDevices()` calls `GET
https://api.bambulab.com/v1/iot-service/api/user/bind` with `Authorization:
Bearer <token>`, parsing each entry's `dev_id` (used as `PrinterProfile.serial`
— matches the LAN serial per the cited references, avoiding a schema fork
between LAN and Cloud profiles), `name`, and `online` status into candidates
for `AddPrinterDialog.qml`'s "pick from my account" list.

**`CloudPrinterConnection`**: connects to `<region>.mqtt.bambulab.com:8883`
(hardcoded to the `us` region for now — region selection/detection is a
documented open question, not a blocker for initial support) with a
**publicly-trusted** certificate, so no TOFU flow (unlike LAN, `confirmCertificateTrust`
stays the inherited no-op default). The exact username/password frame is the
biggest unconfirmed piece of this whole phase — cited sources describe
user-id-derived credentials rather than a raw access code; implemented as
`username: "u_<user_id>"` / `password: <access token>`, with the user id
best-effort-decoded from the access token's own JWT payload claims
(`CloudAuthClient::userId()`, tried under a few possible claim names) rather
than a separate API call, and adjusted based on the manual verification pass
described above if wrong. Topic conventions (`device/{serial}/report`,
`device/{serial}/request`) are assumed to carry over unchanged from LAN,
since the cloud broker is understood to relay the same device protocol.
`sendCommand()`/report parsing reuse `BambuCommandBuilder`/`BambuReportParser`
unchanged, and command-ack correlation reuses `PendingCommandTracker` exactly
as `LanPrinterConnection` does (it's already transport-agnostic — no MQTT
specifics baked in). If the auth frame assumption is wrong, the MQTT
connection simply won't authenticate, `CloudPrinterConnection` reports
`ConnectionState::Error`, and nothing crashes or hangs — same
defensive-degradation posture as Phase 2's ack timeout design.

**`ConnectionMode` and fallback**: `PrinterProfile` gains `enum class
ConnectionMode { LanOnly, CloudOnly, PreferLanThenCloud }`, defaulting to
`LanOnly` (so every Phase 1/2 printer profile keeps behaving identically —
no migration needed for existing config). `ConnectionFactory::create()`
becomes mode-aware: `LanOnly`/`CloudOnly` construct the matching connection
directly; `PreferLanThenCloud` constructs `LanPrinterConnection` first, and
`PrinterRegistry` (not either connection class) watches for that connection
settling into `ConnectionState::Error` and, only then, constructs a
`CloudPrinterConnection` for the same profile as a fallback — keeping both
connection classes ignorant of each other, per the Phase 1 architecture's
original intent. `PrinterRegistry::addLanPrinter()` gained an optional `mode`
parameter (default `LanOnly`, so existing call sites are unaffected) —
without it, `PreferLanThenCloud` would have no entry point at all yet (the UI
only offers `addLanPrinter` for manual LAN entry and `addCloudPrinter` for
the account picker, both hardcoding their own mode), leaving the fallback
requirement in the `printer-management` spec delta unreachable. Exposing
`PreferLanThenCloud` in `AddPrinterDialog.qml` itself stays deferred, per
`PLAN.md`'s scope for this phase.

**System CA trust for the relay's publicly-trusted certificate**: libmosquitto
has no "use the system default CA store" call — it needs an explicit
`cafile`/`capath`. `MqttClient::connectToHost()` gained a `useSystemCaTrust`
parameter (default `false`, preserving `LanPrinterConnection`'s existing
insecure-mode behavior unchanged); when `true`, it tries a fixed list of
common per-distro CA bundle paths (Debian/Ubuntu/Arch, Fedora/RHEL,
openSUSE, Alpine) and falls back to insecure mode with a code comment
flagging the gap if none exist — see Risks below.

## Risks / Trade-offs

- **[Risk]** The cloud MQTT auth frame is the least-confirmed part of the
  entire protocol reference used throughout this project → **Mitigation**:
  defensive failure (Error state, not a crash/hang) plus a redaction-safe
  local diagnostic log, so the real shape can be nailed down from live
  testing without the risk of leaking credentials into this conversation or
  version control. **Update from manual verification against a real
  account**: this risk materialized. The access token turned out to be an
  opaque string, not a JWT (the original assumption in this doc, since
  corrected in code). The follow-up assumption — fetching the account uid
  from a `my/info` endpoint — was also wrong (confirmed 404). Neither the
  login response nor the device-list response (`GET .../api/user/bind`,
  otherwise fully working — login, 2FA, device listing, and adding a cloud
  printer all confirmed working end-to-end) carry an account-level uid
  anywhere. So the `"u_<uid>"` cloud MQTT username scheme itself is now
  unconfirmed, not just the uid lookup mechanism — live cloud MQTT
  status/control remains a known, open gap. The community references this
  project cites may predate whatever API version real accounts are now on.
  Degrades safely as designed: `CloudPrinterConnection` reports
  `ConnectionState::Error` rather than crashing or hanging, and the
  diagnostic logging (kept permanently, not removed) captures enough
  (HTTP statuses, response key names — never values) to pick this back up
  without needing a fresh round of guessing from scratch.
- **[Risk]** Hardcoding the `us` MQTT region will break for non-US accounts
  (e.g. `cn.mqtt.bambulab.com` for China-region accounts) → **Mitigation**:
  documented as a known limitation; the login response may carry a region
  hint worth checking during manual verification, deferred to a follow-up
  change if needed rather than guessed at now.
- **[Risk]** Bambu Cloud token lifetime is undocumented (could be hours or
  months) → **Mitigation**: re-login on any `401`, rather than trying to
  predict expiry; no functional difference to the user beyond an occasional
  re-login prompt.
- **[Trade-off]** Single-account only → accepted per `PLAN.md`; a household
  with multiple Bambu accounts would need to pick one, revisit only if
  requested.
- **[Risk]** The fixed CA-bundle-path list for system trust validation could
  miss an unusual distro, silently falling back to insecure mode for the
  cloud relay (a real, if narrow, security gap on such systems) →
  **Mitigation**: none automatic yet; flagged clearly in code comments as a
  known limitation to revisit (e.g. deriving the path from Qt's own SSL
  backend, or shipping a bundled CA file) rather than left undocumented.
