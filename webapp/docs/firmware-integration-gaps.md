# Firmware and WebUI Integration Gaps

This document is the shared handoff between firmware and WebUI development.
The browser is a display and command client; firmware remains authoritative for
position, motion permission, safety state, VFD communication, outputs, RF
decoder state, authentication, and audit logging.

Until a capability below has an implemented controller endpoint, the WebUI must
label it as preview-only or disable it. It must not report a hardware action as
accepted, completed, or communicating based only on local browser state.

## Contract Version And Capabilities

Firmware should expose an API version and explicit capability list through
`GET /api/status` or a dedicated `GET /api/capabilities` endpoint. The WebUI
should enable controls only when the connected controller advertises the
corresponding capability.

Initial capability names should cover motion, light state, floor configuration,
VFD telemetry and parameters, RF learn, RF erase-all, logs, network settings,
backup/restore, calibration, and restricted service recovery.

The WebUI also needs a distinct initial `connecting` state. It must not display
`Controller online` until a valid, version-compatible status document has been
received.

## Safety And Direction Permission

The current WebUI derives its top-level safety banner primarily from
`safetyOk`. Firmware should instead return an authoritative motion-permission
summary that includes at least:

- `motionAllowed`
- `blockedReasons[]`
- `canMoveUp`
- `canMoveDown`
- safety-loop state
- upper- and lower-limit state
- VFD fault/availability state
- position-valid state

The WebUI must use that complete result for its banner and command availability.
An active limit cannot coexist with an unconditional `All interlocks healthy`
message. Directional limits should disable commands that require travel farther
in the blocked direction. Firmware must still repeat every precheck when a
request arrives; disabled browser controls are not a safety mechanism.

## Position And Landing Identity

The WebUI currently estimates the displayed landing from fixed encoder-count
thresholds. This must be replaced by controller-owned data because floor
positions are configurable and the car may be stopped between landings.

Firmware should return:

- `position`
- `positionValid`
- `currentFloor`, nullable when not at a configured landing
- `atLanding`
- configured floor identifiers, nicknames, and positions
- the landing tolerance or the already-resolved landing result

When `currentFloor` is null, the WebUI should show an explicit between-landings
or unknown-position state and must not display `Lift is here` for any floor.

## VFD Communication And Telemetry

The current visual design contains representative frequency, current, DC-bus,
and temperature values. They must never be presented as live values merely
because `/api/status` succeeds.

Firmware should provide a cached VFD telemetry object containing communication
state, sample timestamp/age, output frequency, motor current, DC-bus voltage,
temperature when supported, active fault, and last valid command/response. The
WebUI should render `Unavailable`, `Stale`, or `Preview` until authoritative
values are present and fresh. `Communicating` must be driven by the controller's
protocol health, not browser connectivity.

## RF Learn And Erase-All

The current RF workflow is locally simulated. Before these controls become
operational, firmware must implement the documented learn and erase-all
endpoints and report authoritative decoder state.

Learn responses should report whether the request was accepted, `MODE_IND`
state, remaining learn-window time, completion/cancellation, and any observed
transmitter identity. Erase-all should use an explicit staged confirmation or
hold-session token, report progress, and confirm the final decoder result.

The WebUI must not say the LEARN line was asserted or decoder memory was erased
until firmware confirms it. Browser timers may display controller-provided
remaining time, but they cannot determine success. All requests require normal
authentication and audit events.

## Outputs And Other Planned Controls

Light state must be returned by firmware and treated as authoritative. A
set-state endpoint such as `POST /api/light` with `{ "on": true }` is preferable
to toggle-only behavior because retries and multiple clients can otherwise
invert the output unexpectedly.

Settings, network changes, calibration, VFD writes, backup/restore, remote
registry changes, and restricted recovery controls must follow the same rule:
no success message until the controller accepts and confirms the operation.
Unsupported controls remain visibly unavailable even when basic status polling
works.

## Errors, Freshness, And Auditing

Every state-changing response should include a stable result code, human-readable
message, and current authoritative state. Rejections should preserve the
specific firmware reason instead of becoming a generic unavailable message.
Timeouts and malformed responses must leave the displayed state unconfirmed.

Motion, stop, lighting, RF, settings, VFD, network, calibration, restore, and
recovery requests must be logged by firmware with source, authentication
identity when available, accepted/rejected result, reason, and resulting state.

## Integration Acceptance Tests

Before the WebUI is packaged into controller firmware, automated or bench tests
must cover:

1. Safety loop open, upper limit active, lower limit active, and VFD fault states.
2. Direction-specific command availability and server-side rejection.
3. Valid landing, between-landings, and invalid/restored-position states.
4. Fresh, stale, unavailable, and faulted VFD telemetry.
5. RF learn accepted, timed out, cancelled, and completed states.
6. RF erase-all cancellation, authorization failure, timeout, and confirmed completion.
7. Light commands from two clients without toggle-state races.
8. Unsupported API capability, API-version mismatch, malformed JSON, timeout, and `401`/`409` responses.
9. Confirmation that no preview-only action produces a controller-success message.

These tests supplement the controller's motion and safety tests. Passing WebUI
integration tests does not qualify the independent hardwired safety system.
