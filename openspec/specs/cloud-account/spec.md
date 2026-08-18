# cloud-account Specification

## Purpose

Lets a user log into their Bambu Lab cloud account so the applet can list
and connect to their printers through Bambu's cloud relay, while keeping the
account's credentials/tokens secure and the unofficial nature of this
connectivity visible to the user.

## Requirements

### Requirement: User can log into a Bambu Cloud account
The system SHALL let the user log into a Bambu Cloud account by providing
their account email and password.

#### Scenario: Successful login
- **WHEN** the user submits valid account email and password
- **THEN** the system authenticates against Bambu Cloud and the account
  becomes available for listing/adding cloud printers

#### Scenario: Invalid credentials are reported
- **WHEN** the user submits an email/password that Bambu Cloud rejects
- **THEN** the system reports the login failure to the user rather than
  silently failing

### Requirement: Login supports a two-factor email verification step
The system SHALL, when Bambu Cloud requires a second factor for a login
attempt, prompt the user for the emailed verification code and complete
login only after it is submitted and accepted.

#### Scenario: Account requires verification code
- **WHEN** a login attempt indicates a verification code is required
- **THEN** the system prompts the user to enter the code emailed to them,
  and completes login only once a valid code is submitted

### Requirement: Cloud credentials and tokens are stored securely
The system SHALL store the Bambu Cloud account's authentication token via
KWallet, and SHALL NOT write it to plain configuration files. The user's
password SHALL NOT be persisted at all — only used for the login request.

#### Scenario: Token is absent from the plain-text config file
- **WHEN** a login has completed successfully
- **THEN** the resulting authentication token does not appear anywhere in
  the applet's plain-text configuration file on disk

### Requirement: User can log out of the Bambu Cloud account
The system SHALL let the user log out, after which the stored cloud token is
discarded and cloud-connected printers stop being reachable via that
account.

#### Scenario: Logging out discards the stored token
- **WHEN** the user logs out of the Bambu Cloud account
- **THEN** the stored authentication token is removed from KWallet

### Requirement: The unofficial nature of Bambu Cloud connectivity is disclosed to the user
The system SHALL make clear, in the cloud login UI, that this connects
through Bambu Lab's unofficial/undocumented API rather than an
Bambu-Lab-supported integration.

#### Scenario: Disclosure is visible before logging in
- **WHEN** the user opens the cloud login UI
- **THEN** a visible notice states that this uses an unofficial,
  reverse-engineered API that Bambu Lab does not document or support
