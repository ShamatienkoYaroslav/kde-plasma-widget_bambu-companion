## Context

Greenfield repo — no build system or code exists yet. See `proposal.md` for
motivation. `PLAN.md` and `CLAUDE.md` already commit this project to a Plasma 6
Applet KPackage + a separately-built QML plugin, CMake/ECM, and specific
libraries per later phase (libmosquitto, libcurl, KWallet, etc.) — this design
only covers what Phase 0 actually needs to stand up.

## Goals / Non-Goals

**Goals:**
- A KPackage + QML-plugin pairing that follows current, verified Plasma 6
  conventions (not carried-over Plasma 5 patterns).
- A build that a contributor can `cmake --build` and `kpackagetool6 -i` with no
  manual steps beyond installing system dependencies.
- CI that catches build breakage on every push.

**Non-Goals:**
- No printer protocol, MQTT/FTPS, KWallet, or KConfigXT work — that starts in
  Phase 1. The stub `PrinterListModel` in this phase holds hardcoded dummy
  data, not anything read from disk or network.
- No packaging for distribution (AppStream metainfo, `.spec`/`PKGBUILD`) — that
  is Phase 5.
- No visual/UX design pass on the placeholder QML — it exists only to prove
  the plugin-to-applet wiring works.

## Decisions

**Plasma 6 system-tray registration keys**: use
`"X-Plasma-NotificationAreaCategory": "Hardware"` and
`"X-Plasma-API-Minimum-Version": "6.0"` in `metadata.json`, and omit the legacy
Plasma-5 `X-Plasma-NotificationArea` boolean key. Confirmed against the current
in-tree `metadata.json` of `plasma-workspace`'s own applets (`devicenotifier`,
`clipboard`), which use exactly this pair and omit the legacy key. Allowed
category values are strictly `ApplicationStatus` | `Hardware` |
`SystemServices`; `Hardware` fits since this applet monitors physical devices.

**QML plugin wiring**: use `ecm_add_qml_module()` / `ecm_finalize_qml_module()`
(from ECM's `ECMQmlModule` include) with `QML_ELEMENT` declared directly on the
C++ class (via `#include <qqmlregistration.h>`), rather than a hand-written
`qmldir` plus manual `qmlRegisterType()` calls. This matches the current
pattern used by `kdeconnect-kde`'s `declarativeplugin`, a real, current,
out-of-tree-buildable Plasma 6 add-on with a C++ QML backend. `qmltyperegistrar`
(invoked by `ecm_add_qml_module`) generates the `qmldir`/`.qmltypes` instead of
us maintaining them by hand — less to keep in sync as types are added in later
phases.

**Plugin install location vs. KPackage**: the compiled QML plugin installs to
the standard Qt QML module path (`${KDE_INSTALL_QMLDIR}/io/github/shamatienkoyaroslav/bambucompanion/`),
*not* inside `package/contents/`. `main.qml` reaches it with a normal
`import io.github.shamatienkoyaroslav.bambucompanion` — QML's own import
resolution finds it there. This keeps the KPackage (installed via
`kpackage_install_package`) purely declarative/QML, with all compiled code in
one place.

**Namespace**: `io.github.shamatienkoyaroslav.bambucompanion` for both the
KPackage `KPlugin.Id` and the QML module URI, per `CLAUDE.md` — `org.kde.*` is
reserved for KDE-maintained software and this is a third-party project.

**Minimal dependency set for this phase only**: `find_package(Qt6 COMPONENTS
Core Qml Quick)` and `find_package(KF6 COMPONENTS CoreAddons Package)`. Defer
`Network`, `I18n`, `Config`, `ConfigWidgets`, `KCMUtils`, `Notifications`,
`Wallet`, `Archive`, and the `libmosquitto`/`libcurl` pkg-config checks to the
phases that actually introduce that functionality (per `PLAN.md`) — adding
unused `find_package` calls now would make Phase 0 fail to build on systems
missing packages it doesn't yet need.

**CI base image**: a Fedora-based container (closest to the maintainer's dev
environment, so failures reproduce locally) installing
`extra-cmake-modules qt6-qtbase-devel qt6-qtdeclarative-devel
kf6-kcoreaddons-devel kf6-kpackage-devel`, then `cmake --build` +
`ctest --output-on-failure`. Broader distro coverage (Arch/Debian) is deferred
until later phases add distro-sensitive dependencies (libmosquitto, libcurl)
where packaging availability actually differs.

## Risks / Trade-offs

- **[Risk]** Plasma 6 system-tray metadata conventions could differ subtly
  across Plasma point releases (the ecosystem is still stabilizing
  post-Plasma-5-migration) → **Mitigation**: the keys chosen are copied
  directly from currently-shipping `plasma-workspace` applets, and the task
  list includes a manual verification step (installing and checking the
  widget actually appears in Add Widgets / System Tray) in a real Plasma 6
  session rather than trusting the docs alone.
- **[Risk]** No automated GUI test exists for "does the tray icon actually
  render" → **Mitigation**: accepted for this phase; CI only proves the build
  succeeds and the trivial Qt Test runs. Manual verification is the
  documented final step for this change (and remains the pattern for all
  UI-facing work per `CLAUDE.md`'s testing conventions).
- **[Trade-off]** Keeping `find_package` minimal now means later phases will
  each need to extend `CMakeLists.txt` incrementally rather than declaring
  everything up front → accepted, since it keeps Phase 0 buildable on a
  minimal dependency set and avoids CI failing for reasons unrelated to this
  phase's actual scope.
