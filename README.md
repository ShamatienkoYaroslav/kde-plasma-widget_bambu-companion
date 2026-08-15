# Bambu Companion

A KDE Plasma 6 System Tray widget that shows and controls a list of your Bambu
Lab 3D printers — live status, temperatures, print progress, plate-preview
thumbnails, and pause/resume/stop, connecting either directly over your LAN or
through Bambu Cloud.

Inspired by [ypMrg/omarchy-bambu-companion](https://github.com/ypMrg/omarchy-bambu-companion)
(a single-printer, LAN-only, monitoring-only widget for Omarchy/Hyprland), but
built natively for KDE Plasma in C++/QML, with multi-printer support, Bambu
Cloud connectivity, and print control commands.

This project is under active development — see [`PLAN.md`](./PLAN.md) for the
full roadmap and current status.

## Status

Early scaffolding stage (Phase 0 of the roadmap): the applet installs and
appears in the system tray, but does not yet talk to any real printer. See
[`PLAN.md`](./PLAN.md) for what's implemented and what's next.

## Connecting to your printers

Bambu Companion supports two ways to reach a printer, selectable per printer:

- **LAN** — connects directly to the printer's local MQTT and FTPS services
  using its IP/hostname, serial number, and LAN access code (found in the
  printer's network settings). No Bambu account involved. Printers use
  self-signed TLS certificates, which are pinned trust-on-first-use (like an
  SSH host key) rather than silently trusted.
- **Bambu Cloud** — logs into your Bambu Lab account and relays
  monitoring/control through Bambu's cloud service. **This uses an
  unofficial, reverse-engineered API that Bambu Lab does not document or
  support** ([Doridian/OpenBambuAPI](https://github.com/Doridian/OpenBambuAPI),
  [coelacant1/Bambu-Lab-Cloud-API](https://github.com/coelacant1/Bambu-Lab-Cloud-API)).
  It can change or break without notice.

## Building

Requires Qt6, KDE Frameworks 6 (KF6), and extra-cmake-modules (ECM).

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --output-on-failure --test-dir build
```

## Installing (for development/testing)

Two install steps are needed — `kpackagetool6` only installs the declarative
widget content; the compiled QML plugin needs a regular CMake install into
Qt's system QML import path (typically requires root):

```sh
sudo cmake --install build                      # installs the compiled QML plugin
kpackagetool6 --type Plasma/Applet -i package    # first install of the widget
kpackagetool6 --type Plasma/Applet -u package    # after changes to package/
```

Then add "Bambu Companion" from Plasma's "Add Widgets" dialog, or enable it
under System Tray → Entries.

## Contributing

See [`CONTRIBUTING.md`](./CONTRIBUTING.md).

## License

GPL-3.0-or-later — see [`LICENSE`](./LICENSE).
