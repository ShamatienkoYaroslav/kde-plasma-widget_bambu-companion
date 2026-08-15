## Purpose

Lets a user act on a connected printer — pause, resume, stop, or skip
objects on a running print — from the applet, with feedback on whether each
command succeeded and a confirmation guardrail before the destructive stop
action.

## ADDED Requirements

### Requirement: User can pause a printing job
The system SHALL let the user pause a printer that is currently printing, and
SHALL send the pause command immediately without requiring confirmation.

#### Scenario: Pausing an active print
- **WHEN** the user activates "Pause" for a printer that is currently
  printing
- **THEN** the system sends a pause command to that printer immediately

### Requirement: User can resume a paused job
The system SHALL let the user resume a printer that is currently paused, and
SHALL send the resume command immediately without requiring confirmation.

#### Scenario: Resuming a paused print
- **WHEN** the user activates "Resume" for a printer that is currently paused
- **THEN** the system sends a resume command to that printer immediately

### Requirement: Stopping a print requires explicit confirmation
The system SHALL require the user to explicitly confirm before sending a stop
command, since stopping a print is destructive and cannot be undone.

#### Scenario: Stop prompts for confirmation
- **WHEN** the user activates "Stop" for a printer
- **THEN** the system shows a confirmation prompt before sending anything to
  the printer

#### Scenario: Confirming stop sends the command
- **WHEN** the user confirms the stop prompt
- **THEN** the system sends a stop command to that printer

#### Scenario: Cancelling the prompt sends nothing
- **WHEN** the user dismisses or cancels the stop confirmation prompt
- **THEN** no command is sent to the printer

### Requirement: User can skip specific objects on a multi-plate print
The system SHALL let the user request that specific objects be skipped on the
current print by identifying them (object IDs), and SHALL send a
skip-objects command containing that list.

#### Scenario: Skipping objects by ID
- **WHEN** the user submits a non-empty list of object IDs to skip for a
  printer that is currently printing
- **THEN** the system sends a skip-objects command containing that list of
  object IDs to that printer

### Requirement: Command outcome is reported back to the user
The system SHALL report whether each sent command (pause, resume, stop,
skip-objects) succeeded or failed, based on the printer's acknowledgement.

#### Scenario: A successful command is reflected as such
- **WHEN** a printer acknowledges a sent command as successful
- **THEN** the system reflects that success to the user (e.g. no error shown,
  updated printer state)

#### Scenario: A failed command is reported as an error
- **WHEN** a printer acknowledges a sent command as failed, or does not
  acknowledge it within a reasonable time
- **THEN** the system reports the failure to the user rather than silently
  ignoring it
