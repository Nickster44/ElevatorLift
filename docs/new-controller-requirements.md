# New Controller Requirements Draft

## Safety And Motion

- Hardware safety chain must remove motion authority without depending on application firmware.
- Final upper/lower limits should be hardwired and also monitored by firmware.
- The controller should latch faults until an explicit reset action.
- A watchdog timeout should put the VFD command path and motion outputs into a safe state.
- Motion should require a precheck: safety loop closed, no active limit conflict, valid position, target floor valid, VFD healthy, and no active fault.
- The state machine should have explicit states: boot, unknown-position, idle, homing, moving, stopping, stopped, service, and fault.
- Any unexpected movement while idle should fault.
- Lack of expected encoder movement while commanding motion should fault.
- Stop timeout should fault if the VFD does not acknowledge or monitor as stopped.
- Primary floor stopping should use a measured deceleration-distance offset, not a PID position loop.
- Calibration mode should preserve the old behavior: after floor positions are set and program mode exits, run toward the farther top/bottom end, reach normal speed, command stop, measure actual stop distance, and save that calibration value.
- If VFD deceleration, max frequency, normal run speed, or encoder scaling changes, the stop-distance calibration should be marked stale or require recalibration.

## Position Sensing

- Use a quadrature counter IC rather than application-level edge interrupts.
- Read position from a single monotonic count source.
- Define count polarity so upward motion and downward motion are unambiguous.
- Store current position in MRAM frequently enough to recover from reset.
- Store homing/calibration records with CRC and sequence numbers.
- Use home and final limit switches to validate counter position.
- The likely encoder is an SKF Motor Encoder Unit. Its datasheet describes two Hall-effect square-wave outputs with 90-degree phase shift, open-collector outputs, and 32-80 pulses per revolution depending on unit size.
- The encoder interface should include pullups or configurable biasing, current limiting, input protection, and Schmitt-trigger or differential/noise-tolerant conditioning before the counter IC.

## Persistence And Logging

- MRAM should store:
  - Current position snapshot.
  - Last known motion state.
  - Floor target table.
  - Stop offset/calibration table.
  - Local lift settings, including normal run speed and service jog speed.
  - Cached VFD parameter values and last write status.
  - Saved station network settings.
  - Configuration version and CRC.
  - Fault/event ring buffer.
  - Boot count and reset reason.
- Use redundant records or sequence numbers so interrupted writes do not corrupt the active configuration.
- Keep logs binary internally, with web/API export as JSON or CSV.
- Use MRAM as the authoritative store for critical configuration and recent logs.
- Consider optional SD card or external QSPI flash for rich WebUI assets, OTA bundles, and long-term downloadable logs.

## Connectivity

- The primary field-service path is a self-hosted Wi-Fi access point. Ethernet is not a near-term requirement because outdoor wiring may be impractical.
- The controller should allow a user to connect directly to the AP, configure station-mode Wi-Fi credentials, and then reboot into station mode.
- If station connection fails at boot or is lost during operation, the controller should enable the fallback AP again.
- The controller should support local AP access for service and initial setup.
- Station mode is optional and only for convenience on a local network.
- Motion and setup must not require cloud access.
- Authenticated web access should be added before any production write operation.
- RF remote pairing and program/service settings should be available from the web interface so the enclosure does not need extra physical buttons or switches for normal configuration.
- Provide an automation path for local smart-home systems. The baseline should be a documented local REST API with token-protected write endpoints and mDNS hostname support. MQTT/Home Assistant discovery can be added later for status entities and command topics. Cloud voice assistants such as Google Home should go through Home Assistant or another local automation hub rather than bypassing the controller's authentication, logging, and motion prechecks.

## WebUI And Configuration

- The WebUI should expose local lift settings such as run speed, jog speed, floor positions, stop offsets, RF remote assignments, network settings, and log export.
- The WebUI should also expose VFD parameter read/write access using a metadata table with names, units, min/max/default values, and access levels.
- Normal users should only see validated settings. Installer/advanced mode can expose the raw VFD parameter list.
- All VFD writes should require the lift to be stopped, be range checked, be read back after write, and be logged.
- Lift run speed should be stored as a local controller setting and translated into VFD run commands during motion.
- Current/overload VFD settings may be used for a configurable load-limit feature after calibration, but should not be presented as a certified weight measurement.
- The WebUI should support configuration backup/restore and a factory-default reset path.
- The WebUI should support entering/exiting calibration mode, setting floor positions, starting the measured stop-distance calibration, viewing the saved calibration value, and warning when calibration is stale.

## Power And Mechanical Placement

- The preferred design target is a single control board that fits cleanly inside the VFD housing.
- If the board is mounted inside the VFD housing, it may need to tap one 120 VAC leg for control power.
- The power input should include appropriate fusing, surge protection, creepage/clearance, and an isolated AC/DC supply or approved enclosed module.
- The previous design used an accessory board in the disconnect box with a 120 VAC to DC supply; this remains a fallback architecture if VFD-box space is too limited.
- The previous accessory board used a RECOM `RAC10-12SK/277` 12 V, 10 W supply. This is a useful baseline because the lift light is believed to be 12 V at about 750 mA, but it leaves limited current margin if a solenoid output returns.
- The first PCB should target roughly 3.5 in x 3.5 in with mounting holes near the corners. Exact mounting hole coordinates and keepouts can be revised after measuring the VFD housing.
- Mains/control power should enter through a serviceable connector. Start with a screw terminal footprint; evaluate blade terminals if they improve cabinet wiring and safety clearances.
- Mechanical design should account for service access, antenna placement, separation from VFD power wiring, connector strain relief, and safe separation between mains and SELV circuitry.

## RF Remote Input

- The legacy RF receiver is believed to be a Linx RXM-418-LR module with existing remotes already available.
- The new board must keep a compatible 418 MHz receiver input path because the existing remotes remain part of the required user workflow.
- Pairing, remote assignment, and enable/disable behavior should be managed through the local web interface rather than a dedicated program-mode switch.
- RF commands must be treated like user requests, not safety signals; motion prechecks and interlocks still apply.

## Outputs And Interlocks

- The old disconnect-box accessory board used 120 VAC, 10 A mechanical relays for solenoids, gate interlocks, controls, and lights.
- If gate interlocks are not used, the new board should avoid carrying bulky relay channels just for legacy compatibility.
- Retain at least one appropriately rated 12 V light-control output. A protected MOSFET output is the likely default if the light load is DC and shares the controller 12 V supply.
- Any future gate/interlock control should be reviewed as part of the safety architecture and may require safety-rated hardware rather than general-purpose MCU-controlled relays.

## VFD Interface

- The VFD UART should include protection and a defined ground/reference strategy.
- The VFD serial input is opto-isolated but still uses standard UART framing. The new board must include a driver stage that can provide the required opto input current; direct MCU GPIO drive is not sufficient based on the old design experience.
- Firmware should verify checksums for all VFD responses.
- VFD monitor status should be polled during motion.
- Stop should be layered: serial stop command plus a fail-safe hardware path where possible.
- Firmware should support reading and writing all documented VFD parameters through generic get/set protocol functions.
- Firmware should maintain a VFD parameter cache so the WebUI can show recent values even if the drive is temporarily unavailable.

## Environmental And EMI

- The board will likely live in a metal VFD enclosure. Moisture exposure should be reduced by the enclosure, but VFD EMI/noise should be treated as the main electrical stressor.
- Include filtering, TVS/ESD protection, good grounding strategy, input hysteresis, careful connector placement, and layout separation between noisy VFD/mains areas and low-voltage logic.
- Cable shields, grounding, and connector pinout should be reviewed after the existing encoder, button, RF, and light wiring are identified.

## KiCad Design Inputs To Confirm

- Supply voltage available in the lift cabinet.
- Available internal VFD-box volume and mounting points.
- VFD control terminal voltage/reference requirements.
- Exact encoder model, voltage, cable length, pullup voltage, and maximum pulse rate.
- Number and voltage of button, limit, home, safety, and service inputs.
- Whether the Linx RXM-418-LR receiver and existing remotes should remain supported.
- Actual light load voltage/current and whether any solenoid/interlock outputs remain required.
- Environmental requirements for outdoor use: surge, ESD, VFD noise, moisture, temperature, and connector sealing.
