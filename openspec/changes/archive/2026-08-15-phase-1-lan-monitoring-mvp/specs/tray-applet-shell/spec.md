## REMOVED Requirements

### Requirement: Full representation renders placeholder printer list content
**Reason**: Superseded by real printer status data now that
`printer-management` and `printer-status-monitoring` exist — the popup no
longer shows hardcoded placeholder rows.
**Migration**: No user-facing migration; Phase 0 never persisted any state
that depended on the placeholder rows existing.

## ADDED Requirements

### Requirement: Full representation renders live printer list content
The system SHALL render a full (popup) representation that displays list
content sourced from the printer list data model: each configured printer's
live status when at least one printer is configured, or a prompt to add a
printer when none are configured yet. The full representation SHALL NOT
display hardcoded placeholder/dummy printer data.

#### Scenario: Opening the popup with configured printers
- **WHEN** the user clicks the tray icon to open the full representation and
  at least one printer has been added
- **THEN** the popup opens and displays each configured printer's current
  live status, with no errors or warnings in Plasma's log

#### Scenario: Opening the popup with no printers configured
- **WHEN** the user clicks the tray icon to open the full representation and
  no printers have been added
- **THEN** the popup opens and displays a prompt inviting the user to add a
  printer, with no errors or warnings in Plasma's log
