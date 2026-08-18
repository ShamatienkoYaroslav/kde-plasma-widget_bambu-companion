# printer-management Specification

## Purpose

Lets a user add, trust, and persist a Bambu Lab printer reachable on their
local network, so the applet knows which printers to connect to and how.

## Requirements

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

### Requirement: A printer's connection mode determines which transport is used
The system SHALL associate each printer with a connection mode of LAN-only,
Cloud-only, or LAN-preferred-with-Cloud-fallback, and SHALL use only the
transport(s) implied by that mode when connecting.

#### Scenario: LAN-only printer never uses Cloud
- **WHEN** a printer configured as LAN-only cannot be reached over LAN
- **THEN** the system does not attempt to connect to it via Bambu Cloud

#### Scenario: Cloud-only printer never uses LAN
- **WHEN** a printer configured as Cloud-only is being connected
- **THEN** the system connects via Bambu Cloud and does not attempt a LAN
  connection

#### Scenario: LAN-preferred printer falls back to Cloud
- **WHEN** a printer configured for LAN-preferred-with-Cloud-fallback fails
  to connect over LAN
- **THEN** the system attempts to connect to the same printer via Bambu
  Cloud instead

### Requirement: The configured printer list automatically syncs with a logged-in Bambu Cloud account's devices
The system SHALL, whenever a Bambu Cloud account is logged in (checked at
startup and immediately after a successful login), fetch that account's
bound device list and add a printer for each device not already configured
(matched by serial/device id). The system SHALL NOT remove a previously
added printer just because it no longer appears in the account's device
list — removal remains a manual action.

#### Scenario: Already-logged-in account syncs its devices on startup
- **WHEN** the applet starts and a Bambu Cloud account is already logged in
- **THEN** the system fetches that account's device list and adds a printer
  for each device not already configured

#### Scenario: Logging in triggers an immediate sync
- **WHEN** the user completes a Bambu Cloud login
- **THEN** the system fetches the account's device list and adds a printer
  for each device not already configured, without further user action

#### Scenario: Already-configured devices are not duplicated
- **WHEN** a sync runs and a fetched device's serial matches an
  already-configured printer
- **THEN** no duplicate printer profile is created for that device

#### Scenario: A device dropping off the account does not remove its printer
- **WHEN** a sync runs and a previously-synced device is no longer present
  in the account's device list
- **THEN** the printer previously added for that device remains configured

### Requirement: Logging out of Bambu Cloud clears the configured printer list
The system SHALL, when the user logs out of their Bambu Cloud account,
remove every configured printer — both Cloud-sourced and LAN-sourced —
stopping each one's connection, deleting its persisted profile, and
removing any stored LAN access code from KWallet. The printer list is
account-scoped: without a logged-in account there is no source of truth for
which printers are configured. The same clearing SHALL also happen at
startup if no Bambu Cloud account is logged in and one or more printers are
already configured (e.g. left over from before this behavior existed).

#### Scenario: Logging out removes all printers
- **WHEN** the user logs out of their Bambu Cloud account while one or more
  printers (LAN and/or Cloud) are configured
- **THEN** every configured printer is removed, its connection stopped, its
  persisted profile deleted, and any stored LAN access code removed from
  KWallet

#### Scenario: Starting up already logged out clears any leftover printer list
- **WHEN** the applet starts, no Bambu Cloud account is logged in, and one
  or more printers are already configured (from a prior session)
- **THEN** every configured printer is removed the same way as an explicit
  logout, leaving the printer list empty

#### Scenario: Logging back in does not restore previously-cleared printers on its own
- **WHEN** the user logs back into a Bambu Cloud account after a logout that
  cleared the printer list
- **THEN** the account's current device list is synced as usual (per the
  automatic-sync requirement above), adding a printer for each bound device
  — no attempt is made to restore printers that existed before the logout
