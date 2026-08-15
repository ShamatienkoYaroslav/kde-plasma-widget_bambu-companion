## 1. Build system foundation

- [x] 1.1 Add top-level `CMakeLists.txt`: `find_package(ECM REQUIRED NO_MODULE)`,
      set `CMAKE_MODULE_PATH`, `include(KDEInstallDirs)`,
      `include(KDECMakeSettings)`, `include(ECMQmlModule)`,
      `find_package(Qt6 COMPONENTS Core Qml Quick)`,
      `find_package(KF6 COMPONENTS CoreAddons Package)`, `add_subdirectory(src)`,
      `add_subdirectory(package)`, `add_subdirectory(tests)`.
- [x] 1.2 Add `cmake/` directory (empty placeholder is fine — no custom
      `Find*.cmake` modules needed yet).
- [x] 1.3 Add `.gitignore` for `build/` and other CMake/Qt build artifacts.

## 2. QML plugin stub (`src/qmlplugin/`)

- [x] 2.1 Add `src/CMakeLists.txt` that adds the `qmlplugin` subdirectory.
- [x] 2.2 Add `src/qmlplugin/PrinterListModel.h`/`.cpp` — a `QAbstractListModel`
      subclass with `QML_ELEMENT`, populated in its constructor with 2-3
      hardcoded dummy printer name/status rows (no real data source yet).
- [x] 2.3 Add `src/qmlplugin/CMakeLists.txt` using `ecm_add_qml_module(...
      URI "io.github.shamatienkoyaroslav.bambucompanion" VERSION 1.0)`,
      `target_sources` with `PrinterListModel.cpp`, `target_link_libraries`
      against `Qt::Quick`, `KF6::CoreAddons`, and
      `ecm_finalize_qml_module(... DESTINATION ${KDE_INSTALL_QMLDIR})`.

## 3. Plasma Applet package (`package/`)

- [x] 3.1 Add `package/metadata.json` with
      `KPlugin.Id: "io.github.shamatienkoyaroslav.bambucompanion"`,
      `KPlugin.Name`, `KPlugin.Description`, `KPlugin.Icon`,
      `KPackageStructure: "Plasma/Applet"`,
      `X-Plasma-API-Minimum-Version: "6.0"`,
      `X-Plasma-NotificationAreaCategory: "Hardware"`.
- [x] 3.2 Add `package/contents/ui/main.qml` — a `PlasmoidItem` whose
      `compactRepresentation` shows a placeholder icon and whose
      `fullRepresentation` is a `ListView` bound to a `PrinterListModel`
      instance (`import io.github.shamatienkoyaroslav.bambucompanion`),
      rendering each dummy row's name/status as a `Label`.
- [x] 3.3 Add `package/CMakeLists.txt` using
      `kpackage_install_package(. io.github.shamatienkoyaroslav.bambucompanion
      plasmoids plasma)`.

## 4. Tests

- [x] 4.1 Add `tests/CMakeLists.txt` wiring a `QtTest`-based executable into
      `ctest` (via `ecm_add_test` or equivalent).
- [x] 4.2 Add one trivial smoke test (e.g. asserts `PrinterListModel` reports
      the expected dummy row count) proving the plugin's C++ code and the
      test harness both build and run correctly.

## 5. Repo meta files & CI

- [x] 5.1 Add `LICENSE` — GPL-3.0-or-later (standard choice for KDE Plasma
      applets, keeps derivatives open).
- [x] 5.2 Add `README.md` covering: what the project is, build/install
      instructions, and the LAN-vs-Cloud + "Bambu Cloud API is
      unofficial/reverse-engineered" disclaimers from `CLAUDE.md`.
- [x] 5.3 Add `CONTRIBUTING.md` pointing at the OpenSpec workflow in
      `CLAUDE.md`.
- [x] 5.4 Add `.github/workflows/ci.yml`: Fedora-based container job that
      installs `extra-cmake-modules qt6-qtbase-devel qt6-qtdeclarative-devel
      kf6-kcoreaddons-devel kf6-kpackage-devel`, runs
      `cmake -B build -DCMAKE_BUILD_TYPE=Debug`, `cmake --build build`, and
      `ctest --output-on-failure --test-dir build`.

## 6. Verification

- [x] 6.1 Build locally: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake
      --build build && ctest --output-on-failure --test-dir build`.
- [x] 6.2 Install the package: `kpackagetool6 --type Plasma/Applet -i package`
      (or `-u` if already installed).
- [x] 6.3 In a live Plasma 6 session, confirm "Bambu Companion" appears in the
      "Add Widgets" dialog and in System Tray → Entries; add it to the tray;
      confirm the compact icon renders and the popup shows the dummy printer
      rows with no errors/warnings in `journalctl --user -f` or
      `plasmashell` output.
- [ ] 6.4 Confirm CI passes on the opened PR/branch.
