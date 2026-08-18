## REMOVED Requirements

### Requirement: User can add a LAN printer
**Reason**: Manual printer entry is replaced by automatic sync from a
logged-in Bambu Cloud account — the "Add Printer" UI (LAN form and account
picker) is removed entirely.
**Migration**: Already-configured LAN printers are left alone while the
account stays logged in, but are removed the next time the user logs out of
Bambu Cloud (see the new "Logging out of Bambu Cloud clears the configured
printer list" requirement below) — there is no UI path to re-add them
manually afterward. A user who wants LAN-specific connectivity for a printer
already synced via Cloud has no UI path to do so as of this change; this is
a known, accepted limitation (see design.md).

### Requirement: User can add a printer from their Bambu Cloud account
**Reason**: Superseded by automatic sync — logged-in accounts' devices are
now added automatically rather than requiring the user to pick them from a
list.
**Migration**: No action needed; the automatic sync (see the new
requirement below) covers the same devices a manual pick would have.

## ADDED Requirements

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
