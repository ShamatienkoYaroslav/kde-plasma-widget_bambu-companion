# tray-applet-shell Specification

## Purpose

Provides the installable Plasma 6 System Tray applet shell that later phases
attach real printer-monitoring content to — its packaging, system-tray
discoverability, and placeholder compact/full representations.

## Requirements

### Requirement: Applet installs as a Plasma 6 Applet package
The system SHALL provide an installable Plasma 6 Applet package (KPackage)
identified as `io.github.shamatienkoyaroslav.bambucompanion` that `kpackagetool6`
can install and update.

#### Scenario: Installing the package
- **WHEN** a user runs `kpackagetool6 --type Plasma/Applet -i package` against
  this project's `package/` directory
- **THEN** the package installs without error and becomes available to add as
  a widget in Plasma

#### Scenario: Updating an installed package
- **WHEN** a user runs `kpackagetool6 --type Plasma/Applet -u package` after
  the package is already installed
- **THEN** the installed package is updated in place without error

### Requirement: Applet is discoverable in the System Tray
The system SHALL declare itself as a system-tray applet (notification-area
category) so `org.kde.plasma.systemtray` automatically lists it as an item the
user can enable, and so it also appears in Plasma's "Add Widgets" dialog.

#### Scenario: Widget appears in Add Widgets
- **WHEN** the package is installed and the user opens Plasma's "Add Widgets"
  dialog
- **THEN** "Bambu Companion" appears as an available widget

#### Scenario: Widget appears in System Tray entries
- **WHEN** the package is installed and the user opens the System Tray's
  "Entries" configuration
- **THEN** "Bambu Companion" is listed as an item that can be shown or hidden
  in the tray

### Requirement: Compact representation renders without error
The system SHALL render a compact (tray icon) representation that does not
crash or error even before any printer data exists.

#### Scenario: Compact representation with no printers configured
- **WHEN** the applet is added to the system tray with no printers configured
- **THEN** the compact representation renders an icon with no errors or
  warnings in Plasma's log

### Requirement: Full representation renders placeholder printer list content
The system SHALL render a full (popup) representation that displays list
content sourced from the printer list data model, showing placeholder entries
when no real printer connection exists yet.

#### Scenario: Opening the popup with placeholder data
- **WHEN** the user clicks the tray icon to open the full representation
- **THEN** the popup opens and displays the placeholder printer list entries
  provided by the underlying data model, with no errors or warnings in
  Plasma's log
