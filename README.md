# Elevator Lift Controller Redesign

This repository is the starting point for a redesigned outdoor elevator lift controller. The original controller was a Particle Xenon sketch that drove an EM01 VFD over serial, watched a safety loop, counted position pulses in firmware, stored floor positions in EEPROM, and exchanged simple serial messages with a secondary relay/input board.

The new design goals are:

- Make motion control deterministic and auditable with an explicit state machine.
- Move encoder counting out of interrupt-heavy application code and into a quadrature counter IC.
- Store current position, floor targets, calibration values, faults, and event logs in nonvolatile MRAM.
- Host a local access-point web interface for setup, diagnostics, data logs, and maintenance actions, with optional station-mode connection to a local network.
- Keep safety functions electrically independent from the MCU wherever possible.
- Design a custom KiCad control board after the firmware and hardware requirements are clear.

Important: this repository is not a certified elevator controller. The software here is a development baseline. Any real lift must use appropriately rated safety devices, hardwired interlocks, braking/enable circuits, limit switches, emergency stop hardware, enclosure design, and professional review before operation.

## Current Repository Layout

```text
_old_resources/
  old_code.ino.txt                 Original Particle Xenon firmware
  EM01 Manual User_EN-V1.02.pdf     VFD manual; serial protocol starts on page 27
  SKF-Motor-Encoder-Unit---15276_1-EN.pdf
docs/
  hardware-selection.md             First-pass hardware IC/module candidates
  old-system-review.md              Existing firmware/hardware behavior and risks
  vfd-serial-protocol.md            Extracted EM01 serial protocol notes
  new-controller-requirements.md    Hardware and firmware requirements draft
  webui-vfd-and-storage-plan.md     WebUI, VFD parameter, and log storage plan
firmware/
  platformio.ini                    Starter PlatformIO target for ESP32-S3
  include/
  src/
```

## Old System Summary

The old sketch used these major I/O groups:

| Function | Old Particle pin | Notes |
| --- | --- | --- |
| Top button | `A3` | Floor command or jog up in program mode |
| Right button | `A4` | Floor command or relay toggle in program mode |
| Bottom button | `A5` | Jog down in program mode or relay toggle |
| Left button | `A2` | Floor command |
| Center button | `D2` | Stop, set position, or hold for homing behavior |
| RF line | `D3` | Output controlled by incoming serial data |
| Safety loop | `A1` | Input pullup; low meant safety loop closed |
| Up position pulses | `D6` | Rising-edge interrupt increments position |
| Down position pulses | `D8` | Rising-edge interrupt decrements position |
| VFD serial | `Serial1`, 9600 baud | EM01 ASCII command protocol |
| Relay/input serial | `Serial2`, 9600 baud | Reads switch/remote data and writes relay bits |
| USB debug | `Serial`, 115200 baud | Diagnostics |

The original firmware stored floor positions and stopping thresholds in EEPROM. The live position was only held in RAM except when calibration values were saved. Motion was controlled by sending repeated VFD run commands until the calculated stopping point was reached, then sending repeated stop commands until a stop acknowledgment was seen or a retry counter expired.

## Observed Risk Areas In The Old Firmware

- Position is RAM-only during operation. A reset, brownout, or firmware fault can lose the true carriage position.
- Encoder pulse counting is done directly in MCU interrupts without an external counter or clearly defined quadrature validation.
- Several state flags are shared across timers, serial callbacks, interrupts, and the main loop.
- Stop timing depends on a configured threshold and repeated serial stop messages to the VFD.
- VFD response parsing is weak; partial or stale serial data can affect stop-state decisions.
- The homing path relies on a value in the serial input array and can be affected by stale/default input data.
- One relay bit check uses C++ operator precedence incorrectly: `currRelayValue & 0x02 == 0x02` checks bit 0, not bit 1.
- Blocking delays in button handlers can hide fast-changing input or safety conditions.
- EEPROM writes are used for calibration but there is no structured event/fault log.

These are not proof of the historic first-floor overshoot, but they are credible contributors to intermittent behavior.

## Starter Firmware Direction

The starter firmware under `firmware/` currently targets an ESP32-S3 with Arduino/PlatformIO because it provides integrated Wi-Fi, multiple UARTs, SPI, adequate RAM/flash, and a straightforward local web server. This is only the first reference target. The code is split around interfaces so the board can later move to another MCU or module if the hardware goals require it.

Key starter modules:

- `VfdProtocol`: builds EM01-compatible ASCII commands and checks common acknowledgments.
- `PositionStore`: ESP32 NVS placeholder for persistent state; intended to be replaced by an SPI MRAM driver.
- `main.cpp`: nonblocking motion service loop, safety input checks, button commands, VFD command refresh, and a local HTTP diagnostic API.
- `PinMap.h` and `LiftConfig.h`: all provisional hardware choices and tuning constants.

## Local Web Interface/API

The sample firmware starts a web server on port 80. It enables a fallback access point for direct service access. A user can save station network credentials from the local page/API, reboot the controller, and then access it from the local network if the connection succeeds. If station connection fails or is lost, the controller returns to fallback AP access.

Endpoints:

- `GET /` - small browser dashboard with live status.
- `GET /api/status` - JSON status including state, position, target, safety, and last VFD command.
- `GET /api/network` - current AP/station network status.
- `POST /api/network` - save station SSID/password and require a reboot.
- `POST /api/reboot` - reboot after configuration changes.
- `POST /api/move?floor=N` - request a move to a configured floor.
- `POST /api/stop` - request a controlled stop.

Fallback AP credentials are configured from `firmware/include/Secrets.h`, which is ignored by Git. Station network credentials are saved through the local web API.

Planned WebUI expansion includes local lift settings, RF pairing, VFD parameter read/write, current-limit/load-limit tuning, configuration backup/restore, and log export.

## Hardware Architecture Draft

The next board should be designed around these blocks:

- MCU/module with Wi-Fi AP and station support.
- Compact isolated 120 VAC to DC power supply or equivalent VFD-box power strategy.
- Isolated or protected UART interface to the VFD.
- Quadrature counter IC on SPI or parallel bus.
- Encoder input conditioning for open-collector quadrature outputs.
- SPI MRAM for live position snapshots, settings, event logs, and fault records.
- Optional external flash or SD storage for long-term logs, rich WebUI assets, and exported data.
- Hardware safety chain independent of application firmware.
- VFD enable/stop/brake control path that fails safe on MCU reset or watchdog timeout.
- Protected digital inputs for call buttons, RF receiver, limit switches, home switch, and safety loop.
- Compact protected outputs for lights and any retained interlock/control loads.
- Watchdog and brownout detection.
- Surge/ESD/EMI protection suitable for outdoor wiring and a VFD enclosure.

## Near-Term Project Plan

1. Confirm the mechanical/electrical safety chain and what must remain hardwired.
2. Identify the existing encoder or sensor output type and choose the quadrature counter IC.
3. Choose the MRAM part and define the data layout for current position, floor targets, network settings, and logs.
4. Decide whether the final board should use an ESP32-S3 module, another Wi-Fi MCU/module, or a two-MCU split.
5. Build a bench VFD serial simulator before testing on a real lift.
6. Port the starter state machine to the chosen hardware pinout and counter/MRAM drivers.
7. Begin the KiCad schematic with connector definitions, power tree, isolation/protection, and safety wiring.

## Build The Starter Firmware

Install PlatformIO, then run:

```powershell
cd firmware
pio run
```

For upload and serial monitor:

```powershell
pio run -t upload
pio device monitor
```

No production hardware should be connected until the pinout, VFD interface, safety chain, and stop behavior have been bench tested.
