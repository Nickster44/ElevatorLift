# WebUI, VFD Parameter, And Storage Plan

## WebUI Configuration Scope

The WebUI should become the primary setup and service tool. Physical buttons and switches should be kept for motion requests, emergency/safety functions, or service override only when they are clearly necessary.

Planned WebUI areas:

- Live status: position, target, state, VFD output frequency, current, bus voltage, temperature, safety inputs, limits, RF status, and last fault.
- Motion settings: normal run speed, service jog speed, stop offsets, floor positions, acceleration/deceleration policy, and homing behavior.
- VFD parameters: read all supported EM01 parameters, edit writeable parameters with range checks, and show raw protocol values.
- Load/weight limiting: expose VFD current limit/current-related settings as a controlled user-facing limit after bench validation.
- RF remotes: pair, name, enable/disable, assign behavior, and view last received remote command.
- Logs: recent events, faults, configuration changes, VFD alarms, resets, and exported CSV/JSON.
- Maintenance: backup/restore configuration, firmware version, reset fault latch, reboot, and factory defaults.

## VFD Parameter Handling

The firmware should maintain a metadata table for VFD parameters:

- Parameter number.
- Short name and display name.
- Units and scale.
- Minimum, maximum, and default value from the manual.
- Read/write permission.
- Safety class: user, installer, advanced, or locked.
- Whether a change takes effect immediately or requires a stop/reboot.
- Human-readable help text for the WebUI.

The WebUI should not simply expose raw writes to every user. It should provide a normal-user view for safe settings and an installer/advanced view for full VFD access. Every VFD write should be logged with old value, new value, timestamp/sequence, source, and result.

## Lift Settings Versus VFD Settings

Some settings should remain local controller variables rather than VFD parameters:

- Normal lift run speed.
- Service jog speed.
- Floor positions.
- Stop offset/calibration table.
- Homing speed and homing timeout.
- RF remote assignments.
- AP/station network settings.
- Log retention/export settings.

The VFD should provide motor drive limits and feedback. The controller should decide when motion is allowed and what target speed to command.

## Weight Limit / Current Limit

Using current limit as a user-facing weight limit is reasonable as a design goal, but it needs calibration and clear wording. Motor current is affected by load, temperature, mechanical friction, supply voltage, acceleration ramp, and direction. It should be treated as an adjustable overload threshold, not a certified scale.

Recommended implementation:

- Keep an installer-only raw VFD current parameter page.
- Add a user-facing "load limit" setting that maps to validated current-related VFD settings.
- Log all overcurrent/current-alarm events with direction, speed, position, and VFD monitor data.
- Consider separate thresholds for upward travel, downward travel, acceleration, and steady run if testing shows meaningful differences.

## Storage Recommendation

Use MRAM for data that must survive power loss and should tolerate frequent writes:

- Current position snapshots.
- Active/last motion state.
- Floor positions and calibration values.
- VFD parameter cache and local controller settings.
- Network settings.
- Recent event/fault ring buffer.
- Boot count and reset reason.

Use internal MCU flash or LittleFS for compact built-in web assets if the UI remains small. Add SD card or large external flash only if we want long history, downloadable logs, screenshots/assets, OTA bundles, or a rich WebUI with many static files.

## MRAM Capacity Estimate

"A few megabits" is useful, but the unit matters:

| MRAM size | Bytes | Practical use |
| --- | ---: | --- |
| 1 Mbit | 128 KB | Config, position snapshots, and a modest recent fault log |
| 4 Mbit | 512 KB | Good baseline for config plus thousands of compact log records |
| 16 Mbit | 2 MB | Comfortable MRAM-only recent history and parameter cache |

If one binary log record is 64 bytes, then:

| Storage reserved for logs | Approx. records |
| ---: | ---: |
| 128 KB | 2,048 |
| 512 KB | 8,192 |
| 2 MB | 32,768 |

For this controller, 4-16 Mbit MRAM is sufficient for critical settings and recent logs. It is not ideal for large web files or indefinite logging.

## SD Card Decision

Do not make SD card mandatory for the first board unless the WebUI or log-retention requirement grows. SD cards add sockets, board area, field reliability concerns, and filesystem corruption handling.

Recommended approach:

- Baseline board: SPI MRAM plus MCU internal flash/LittleFS for WebUI assets.
- Add an optional footprint for microSD or external QSPI flash if board space allows.
- Prefer external QSPI NOR flash over removable SD for built-in web assets and OTA bundles.
- Prefer SD only if the user needs removable long-term logs or easy offline export.

## Log Retention Model

Store two tiers:

- Critical recent log in MRAM as a fixed-size binary ring buffer.
- Optional extended log in SD/external flash when available.

Recommended log categories:

- Boot/reset.
- Motion command accepted/rejected.
- Motion start/stop.
- VFD command failures.
- VFD monitor snapshots at fault time.
- Safety loop open.
- Limit switch active.
- Position mismatch/no movement.
- RF command received.
- WebUI login/config change.
- VFD parameter read/write.
- Network connection state changes.

The WebUI should export logs as CSV/JSON, but the internal format should stay compact binary.

