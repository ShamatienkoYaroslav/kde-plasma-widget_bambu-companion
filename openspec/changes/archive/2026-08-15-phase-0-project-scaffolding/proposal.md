## Why

The repository currently contains only planning artifacts (`CLAUDE.md`, `PLAN.md`,
OpenSpec tooling) and no buildable code. Before any printer-monitoring logic can
be added (Phase 1+), the project needs a buildable CMake/ECM project, an
installable Plasma 6 Applet shell that Plasma's system tray actually discovers,
and a CI job that keeps the build green — this is Phase 0 of `PLAN.md`.

## What Changes

- Add the top-level CMake + ECM build system (Qt6/KF6 `find_package` calls,
  subdirectory wiring) with no printer/business logic yet.
- Add `package/` — a Plasma 6 Applet KPackage (`metadata.json`,
  `contents/ui/main.qml`) namespaced `io.github.shamatienkoyaroslav.bambucompanion`,
  registered so `org.kde.plasma.systemtray` auto-discovers it, showing placeholder
  content for now.
- Add `src/qmlplugin/` — a compiled QML module (built via `ecm_add_qml_module`)
  exposing a stub `PrinterListModel` (`QML_ELEMENT`) with dummy data, proving the
  C++ plugin ↔ applet QML wiring works end-to-end before real protocol code
  exists.
- Add `tests/` with a trivial Qt Test smoke test wired into `ctest`.
- Add `.github/workflows/ci.yml`, `LICENSE`, `README.md` (with the LAN/Cloud and
  unofficial-cloud-API disclaimers), `CONTRIBUTING.md`.

## Capabilities

### New Capabilities
- `tray-applet-shell`: the installable Plasma 6 Applet package — its system-tray
  discoverability/registration, and its compact/full representations rendering
  placeholder printer-list content sourced from the stub QML plugin.

### Modified Capabilities
(none — first capability introduced in this project)

## Impact

- Affected code: entire repo (new — first buildable code added).
- New build/runtime dependencies: ECM, Qt6 (Core, Qml, Quick), KDE Frameworks 6
  (CoreAddons, Package, at minimum), pinned via top-level `CMakeLists.txt`.
  MQTT (libmosquitto), FTPS (libcurl), and the remaining KF6 components
  (I18n, Config, ConfigWidgets, KCMUtils, Notifications, Wallet, Archive) are
  deferred to later phases per `PLAN.md` and are not added yet.
- No existing behavior is modified (nothing exists yet to modify).
