# printer-management Specification

## Purpose

Lets a user add, trust, and persist a Bambu Lab printer reachable on their
local network, so the applet knows which printers to connect to and how.

## Requirements

### Requirement: User can add a LAN printer
The system SHALL let the user add a printer profile by providing a
host/IP address, serial number, LAN access code, and (optionally, defaulting
to 8883) an MQTT port.

#### Scenario: Adding a printer with valid connection details
- **WHEN** the user submits the "Add printer" dialog with a host, serial
  number, and access code
- **THEN** a new printer profile is created and the system attempts to
  connect to it over LAN

#### Scenario: Submitting incomplete connection details
- **WHEN** the user submits the "Add printer" dialog with a missing host,
  serial number, or access code
- **THEN** the dialog rejects submission and indicates which field is missing,
  without creating a printer profile

### Requirement: First connection to a printer requires certificate trust confirmation
The system SHALL, before completing the first MQTT connection to a given
printer, present the printer's TLS certificate fingerprint to the user for
explicit confirmation (trust-on-first-use), and SHALL refuse to connect
without that confirmation.

#### Scenario: First connection prompts for trust confirmation
- **WHEN** the system connects to a newly added printer for the first time
- **THEN** a dialog displays the printer's certificate SHA-256 fingerprint and
  asks the user to accept or reject it before the connection completes

#### Scenario: Subsequent connections with a matching pinned fingerprint proceed silently
- **WHEN** the system reconnects to a printer whose certificate fingerprint
  was previously accepted and still matches
- **THEN** the connection proceeds without prompting the user again

#### Scenario: A changed fingerprint blocks the connection and warns the user
- **WHEN** the system connects to a printer whose certificate fingerprint no
  longer matches the previously pinned one
- **THEN** the connection is refused and the user is shown a security warning
  requiring explicit re-confirmation before connecting

### Requirement: The LAN access code is stored securely, not in plain configuration
The system SHALL store each printer's LAN access code via KWallet, and SHALL
NOT write it to plain configuration files.

#### Scenario: Access code is absent from the plain-text config file
- **WHEN** a printer profile with an access code has been added
- **THEN** the access code does not appear anywhere in the applet's
  plain-text configuration file on disk

### Requirement: The configured printer list persists across Plasma restarts
The system SHALL persist each printer profile's non-secret fields (name,
host, serial number, port) so that configured printers are still present
after Plasma Shell restarts, without requiring the user to re-add them.

#### Scenario: Printer profile survives a Plasma Shell restart
- **WHEN** a printer has been added and Plasma Shell is subsequently
  restarted
- **THEN** the printer still appears in the popup's printer list and the
  system attempts to reconnect to it (re-prompting for certificate trust only
  if the pinned fingerprint no longer matches)
