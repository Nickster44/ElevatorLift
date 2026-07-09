# Motion Control And Calibration

## Recommended Motion Strategy

Use the same basic strategy as the old controller: command the VFD at a selected run speed, stop early by a measured deceleration distance, and let the VFD deceleration parameter define the stop ramp. This is simple, deterministic, and already proved accurate enough in the old installation.

Do not use a classic PID position loop as the primary lift control strategy. The VFD already controls motor speed and ramp behavior, and the controller talks to it through a relatively slow serial command path. A PID loop would add tuning complexity without solving the main historic risk, which was likely state/position/serial robustness rather than poor steady-state positioning.

## Current Firmware Behavior

The starter firmware currently uses a prediction-stop threshold:

```text
remaining_counts <= stop_offset_counts -> send VFD stop command
```

This is implemented in `firmware/src/main.cpp` using `activeCommand.stopOffsetCounts`. The current starter data only has a single placeholder stop offset per floor target. Final firmware should move this to an MRAM-backed calibration table.

## Calibration Mode Requirement

The new controller should preserve the old calibration concept:

1. User enters programming/calibration mode from the WebUI or service workflow.
2. User sets or confirms floor positions.
3. When leaving programming mode, the controller chooses the longer available travel direction toward the top or bottom floor.
4. The controller commands motion long enough to reach the configured normal run speed.
5. The controller records the position at the instant it sends the stop command.
6. The controller waits for the VFD stop acknowledgment or confirmed stopped condition.
7. The controller records the final stopped position.
8. The measured difference becomes the deceleration travel distance.
9. The controller stores the calibration value in MRAM with sequence number and CRC.

This calibration value is mostly determined by the VFD deceleration-rate parameter. If the WebUI changes VFD deceleration or normal run speed, the controller should mark the stop calibration stale and require recalibration before normal unattended motion.

## Stored Calibration Data

The first implementation can store a single global stop distance if testing confirms both directions behave similarly. The data model should still leave room for expansion:

- `normal_stop_offset_counts`
- `calibrated_run_speed_tenths_hz`
- `calibrated_vfd_decel_parameter`
- `calibration_direction`
- `start_position_counts`
- `stop_command_position_counts`
- `final_position_counts`
- `measured_stop_distance_counts`
- `timestamp_or_sequence`
- `crc`

If future testing shows meaningful differences, extend this to separate values for upward travel, downward travel, or different run speeds.

## Runtime Checks

Keep the runtime logic conservative:

- Stop command is sent when remaining distance is less than or equal to the calibrated offset.
- VFD stop command should be repeated until acknowledgment or timeout.
- If final stopped position is outside an allowed tolerance, latch a fault or require service recalibration.
- If VFD deceleration, max frequency, run speed, or encoder scaling changes, invalidate or warn on the saved calibration.
- Calibration mode must not override hardwired safety devices, final limits, or fault conditions.
