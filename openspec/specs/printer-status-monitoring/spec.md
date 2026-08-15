# printer-status-monitoring Specification

## Purpose

Turns a connected printer's raw MQTT status reports into structured,
continuously up-to-date printer state that the UI can display, regardless of
which transport (LAN today, Cloud in a later phase) delivered the report.

## Requirements

### Requirement: A full status report is parsed into complete printer state
The system SHALL parse a printer's full status report into a structured
representation covering at minimum: print state, print progress percentage,
current/total layer, nozzle and bed temperatures (current and target),
chamber temperature, fan speeds, speed profile, WiFi signal strength, and
estimated remaining time.

#### Scenario: Full report yields complete status
- **WHEN** a printer's connection receives a full status report containing
  all known fields
- **THEN** the resulting printer state reflects every one of those fields
  accurately

### Requirement: Partial status reports update state without discarding unrelated fields
The system SHALL merge a partial (delta) status report into the printer's
existing known state, updating only the fields present in the report and
leaving previously known fields for anything not mentioned unchanged.

#### Scenario: A delta report updates only the fields it contains
- **WHEN** a printer's connection receives a report containing only an
  updated nozzle temperature
- **THEN** the printer's nozzle temperature updates accordingly and all other
  previously known fields (e.g. bed temperature, progress) remain unchanged

### Requirement: Printer status reflects new reports as they arrive
The system SHALL update the displayed printer status promptly whenever a new
status report (full or partial) is received on an active connection, without
requiring the user to manually refresh.

#### Scenario: A new report updates the popup without user action
- **WHEN** the popup is open showing a printer's status and a new report
  arrives on that printer's connection
- **THEN** the displayed status updates to reflect the new report without the
  user taking any action

### Requirement: Monitoring resumes automatically after a connection drop and recovery
The system SHALL detect when a printer's connection is lost and SHALL
automatically attempt to reconnect and resume status monitoring once the
printer becomes reachable again, without requiring the user to manually
re-add the printer.

#### Scenario: Status monitoring resumes after the printer comes back online
- **WHEN** a connected printer becomes unreachable and later becomes
  reachable again
- **THEN** the system reconnects and status updates resume, and the printer's
  displayed state reflects "offline"/"disconnected" while unreachable
