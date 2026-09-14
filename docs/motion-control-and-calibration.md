# Motion Control And Calibration

## Recommended Motion Strategy

Retain measured deceleration-distance stopping: command a selected VFD speed and
issue STOP early by a measured offset. The owner reported good legacy accuracy;
that observation does not qualify this controller or establish the old fault's cause.

Do not use a classic PID position loop as the primary lift control strategy. The VFD already controls motor speed and ramp behavior, and the controller talks to it through a relatively slow serial command path. A PID loop would add tuning complexity without solving the main historic risk, which was likely state/position/serial robustness rather than poor steady-state positioning.

## Current Firmware Behavior

The portable supervisor uses a directional prediction-stop threshold:

```text
remaining_counts = (target_counts - position_counts) * direction
remaining_counts <= measured_stop_distance -> request STOP
```

This is implemented in `firmware/src/core/Supervisor.cpp`, with separate overshoot,
progress, wrong-direction and final-tolerance checks. `core/Configuration` serializes
the measured calibration metadata into versioned MRAM journal records. Target
commissioning and program-exit APIs are still unsupported while hardware is
inhibited; see the software integration checklist. No floor positions or valid
calibration are invented at startup.

## Calibration Mode Requirement

The requested operational profile preserves measured coast distance, but starts
calibration explicitly rather than automatically on programming-mode exit:

1. User enters programming/calibration mode from the WebUI or service workflow.
2. User sets or confirms floor positions.
3. Start calibration explicitly from the WebUI after validating Floor 1 < Floor 2 < Floor 3. At or below the Floor 1/Floor 3 midpoint choose upward travel; above it choose downward travel. Reject insufficient travel before RUN.
4. The controller commands motion long enough to reach the configured normal run speed.
5. The controller records the position at the instant it sends the stop command.
6. The controller waits for fresh VFD stopped-status/zero-frequency telemetry and stable encoder position. STOP acknowledgement alone is never sufficient.
7. The controller records the final stopped position.
8. The measured difference becomes the deceleration travel distance.
9. The controller stores the calibration value in MRAM with sequence number and CRC.

This calibration value is mostly determined by the VFD deceleration-rate parameter. If the WebUI changes VFD deceleration or normal run speed, the controller should mark the stop calibration stale and require recalibration before normal unattended motion.

## Stored Calibration Data

The current isolated model stores one calibration. The requested operational
profile requires separate upward/downward offsets; the single-record model is
not complete. For each direction retain:

- `configuration_revision`
- `calibrated_run_speed_tenths_hz`
- `calibrated_vfd_decel_parameter`
- `calibration_direction`
- `start_position_counts`
- `stop_command_position_counts`
- `final_position_counts`
- `measured_stop_distance_counts`
- `timestamp_or_sequence`
- `crc`

Normal floor travel must reject absent, stale or direction-mismatched calibration.
Changes to floors, encoder scaling, speed, deceleration or relevant drive parameters
invalidate it. Explicit-start and directional persistence remain integration work.

## Runtime Checks

Keep the runtime logic conservative:

- Stop command is sent when remaining distance is less than or equal to the calibrated offset.
- Repeat STOP independently of bounded reply retries; see [EM01 timing](vfd-serial-protocol.md). Neither acknowledgement nor retry exhaustion proves physical stopping.
- If final stopped position is outside an allowed tolerance, latch a fault or require service recalibration.
- Relevant settings changes invalidate calibration; a warning alone must not permit normal motion.
- Calibration mode must not override hardwired safety devices, final limits, or fault conditions.

## Top Reference And Service Boundary

There are exactly three floors, increasing in the upward direction. HOME is a
separate configured reference near floor 3, not floor-one zero. A rising HOME
event during upward homing records the reference crossing; coast after that edge
is retained. The counter-origin adapter must complete the reference handshake
before position becomes valid. An already-active HOME cannot establish a new
reference without a deliberate service approach. Ordinary idle HOME never
changes coordinates. Homing/calibration require local key plus continuous hold;
service release requests STOP and cancels completion. All constants require field
commissioning and physical verification before an operational profile exists.
