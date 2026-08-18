## ADDED Requirements

### Requirement: User can add a printer from their Bambu Cloud account
The system SHALL, once the user is logged into a Bambu Cloud account, let
them add a printer by selecting it from that account's list of bound
devices, without needing to manually enter its host, serial number, or
access code.

#### Scenario: Adding a printer from the account's device list
- **WHEN** the user, logged into a Bambu Cloud account, selects one of the
  account's printers to add
- **THEN** a new printer profile is created for that printer and the system
  attempts to connect to it via Bambu Cloud

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
