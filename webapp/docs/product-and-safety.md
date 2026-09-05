# Product and Safety Decisions

This document records durable WebUI decisions so future frontend work remains aligned with the controller hardware and firmware.

## Primary controls

The Overview control card mirrors the standard five-button RF remote:

1. Floor 1
2. Floor 2
3. Floor 3
4. Controlled Stop
5. Lift-light toggle

Stop belongs with the operational controls, not in global navigation. It requests a normal controller stop; emergency stopping remains a separate hardwired function.

## Floors and position

- Floors retain stable numeric identifiers even when renamed.
- Each floor stores an editable nickname and encoder position.
- The nickname is shown in live position, control buttons, logs, calibration context, automation, and backup files.
- The current installation's homing reed switch is near Floor 3. Homing copy and workflow assume upward travel to that reference unless the physical installation changes.

## Drive and load protection

- Common VFD parameters appear first, with all 17 documented EM01 parameters available in the expanded register.
- Every write is installer/advanced as appropriate, allowed only when stopped, range checked, read back, and logged.
- Calibration highlights the measurement date, run speed, and stopping distance. Changing run speed or VFD deceleration marks calibration stale.
- Nominal current, overload percentage, and allowed overload time are prominent protection settings. The UI must not describe them as a certified scale or exact weight limit.

## RF remotes

- A remote is not assigned to one floor; every compatible transmitter has the same five button functions.
- The LICAL-DEC-MS001 decoder can retain up to 40 learned addresses but cannot enumerate them for the WebUI or delete one address individually.
- The WebUI registry is separate MRAM data keyed by observed `TX_ID`. It can store a nickname, first/last seen time, last command, and audit history.
- Pairing requests the decoder's 17-second Learn Mode through `LEARN` and reports `MODE_IND` state.
- Erasing requires a deliberate 10-second hold and clears all decoder addresses. Nickname profiles become unlinked; they are not silently rebound.
- Logs include observed transmitter ID, resolved nickname when known, button, requested command, accepted/rejected result, and reason.

## Backup and restore

A versioned backup includes:

- Controller settings and cached/custom VFD parameters.
- Floor numbers, nicknames, and encoder positions.
- Calibration value and dependency metadata.
- Non-secret network preferences.
- Remote nickname/observation profiles.

It cannot include the decoder's internal learned-address memory. On restore, remote profiles remain pending and IDs may stay hidden until that transmitter is observed. Restore must never auto-pair a transmitter, bind a nickname by list position, silently overwrite an active profile, or fail because a saved transmitter is absent.

## Restricted service recovery

The product must not expose a casual "override safety" button. A diagnosed device failure may justify a separate installer-only recovery state, subject to all of these requirements:

- Installer re-authentication and a recorded reason/channel.
- Cabinet-local keyed or service authorization.
- Continuous physical hold-to-run input at the cabinet.
- Explicit direction, low service speed, short automatic timeout, and complete audit logging.
- RF, automation, and ordinary floor calls disabled throughout recovery.
- Emergency stop, hardwired final limits, VFD faults, watchdog/enable authority, and the independent hardwired safety chain remain effective and cannot be bypassed by the browser or MCU.

The WebUI may explain prerequisites and display recovery state, but web authorization alone can never initiate recovery motion.
