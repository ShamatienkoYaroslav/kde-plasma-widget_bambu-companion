## MODIFIED Requirements

### Requirement: Full representation renders live printer list content
The system SHALL render a full (popup) representation that displays list
content sourced from the printer list data model: each configured printer's
live status when at least one printer is configured, or a login-state-aware
empty state when none are configured yet (a prompt to log into Bambu Cloud
when logged out, or a message indicating no printers were found on the
account when logged in). The full representation SHALL NOT display
hardcoded placeholder/dummy printer data, and SHALL NOT display a generic
"add a printer" prompt.

#### Scenario: Opening the popup with configured printers
- **WHEN** the user clicks the tray icon to open the full representation and
  at least one printer has been added
- **THEN** the popup opens and displays each configured printer's current
  live status, with no errors or warnings in Plasma's log

#### Scenario: Opening the popup with no printers configured
- **WHEN** the user clicks the tray icon to open the full representation, no
  printers are configured, and no Bambu Cloud account is logged in
- **THEN** the popup opens and displays a prompt to log into Bambu Cloud,
  with no errors or warnings in Plasma's log

#### Scenario: Opening the popup with no printers configured and logged in
- **WHEN** the user clicks the tray icon to open the full representation, no
  printers are configured, and a Bambu Cloud account is logged in
- **THEN** the popup opens and displays a message indicating no printers
  were found on the account, with no errors or warnings in Plasma's log
