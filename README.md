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
  hardware-requirements-matrix.md   Schematic-facing hardware requirements
  hardware-selection.md             First-pass hardware IC/module candidates
  home-automation-integration.md    Local REST/Home Assistant/Google Home bridge plan
  jlcpcb-lcsc-sourcing.md           JLCPCB/LCSC-oriented major part matrix
  motion-control-and-calibration.md Motion strategy and measured stop calibration
  old-system-review.md              Existing firmware/hardware behavior and risks
  vfd-serial-protocol.md            Extracted EM01 serial protocol notes
  new-controller-requirements.md    Hardware and firmware requirements draft
  webui-vfd-and-storage-plan.md     WebUI, VFD parameter, and log storage plan
  webapp-embedded-delivery.md       ESP32 static bundle budget and serving rules
  field-verification-log.md         Observed legacy hardware and remaining site checks
  schematic-capture-readiness.md    Rev-A capture inputs, BOM register, and release gates
datasheets/
  README.md                         Local datasheet manifest for major ICs/modules
firmware/
  platformio.ini                    Starter PlatformIO target for ESP32-S3
  include/
  src/
webapp/
  app/                              Full development and service-console UI
  embedded/                         Static entry point for the ESP32 build
  docs/                             WebUI API, safety, and delivery contracts
hardware/
  ElevatorLift.kicad_pro            KiCad 10 project; open this file
  ElevatorLift.kicad_sch            Root hierarchical schematic
  Power_RevA.kicad_sch              AC input and 12 V/5 V/3.3 V power
  MCU_Storage_RevA.kicad_sch        ESP32-S3, USB, MRAM and RTC
  VFD_Interface_RevA.kicad_sch      VFD UART and enable interface
  Encoder_Counter_RevA.kicad_sch    Encoder isolation and counter
  IO.kicad_sch                      RF, field inputs and outputs
  ElevatorLift.kicad_pcb            PCB WIP; full footprint/net transfer on a 90 mm x 90 mm outline with 86 mm hole centers
  ElevatorLift_Custom.kicad_sym     Project-owned symbols
  ElevatorLift_Custom.pretty/       Project-owned footprints
  SnapEDA-Library/                  Vendored imported symbols, footprints and STEP files
  sym-lib-table / fp-lib-table      Portable project library registrations
  architecture-blocks.md            Block-level hardware architecture notes
```

## Current Design Knowledge

The current hardware direction is an `ESP32-S3-WROOM-1U-N16R8` controller board in the VFD enclosure, using a self-hosted Wi-Fi AP as the primary service path and optional station Wi-Fi for local-network access. Ethernet is not a near-term requirement.

Known design constraints captured so far:

- KiCad 10.0.4 is the active hardware design version.
- Measured legacy mechanical target is approximately 90 mm x 90 mm with corner mounting-hole centers 86 mm apart; 2.56 mm hole diameter remains to be confirmed.
- The Rev-A VFD serial circuit has been cross-checked against the field design and reproduces its TI `TXS0104E` 3.3 V-to-5 V topology. The assembled board, connector pinout, reference, and idle levels still require bench verification before lift connection.
- The observed encoder path appears to be 12 V open-collector A/B with 270 ohm pullups and 270 ohm series resistors feeding a `TLP291-4` optocoupler. Trace details, current, and maximum pulse rate remain open.
- Legacy Linx/TE RXM-418-LR 418 MHz RF remote support is mandatory.
- Baseline control supply is RECOM `RAC10-12SK/277`, 12 V, 10 W, because the previous system successfully powered its lighting from this supply and JLCPCB lists it as assembly part `C5199922`.
- Rev A retains a jumper-selectable external 12 V lighting input. Use it if measured controller margin is insufficient or future lighting requires more power than the onboard supply can provide.
- Critical state and recent logs should use 4 Mbit SPI/QPI MRAM, with Siproin `PM004MNIATR` as the current JLC-friendly candidate.
- Web assets, OTA staging and noncritical logs should use the module's 16 MB flash and 8 MB PSRAM first. Rev A does not include a separate QSPI NOR device.
- Outside control should use local REST first, optional MQTT later, and Home Assistant as the recommended bridge to Google Home, watches, and broader automation.
- Motion control should stay with measured deceleration-distance stopping, not PID. Calibration should measure actual stop distance after leaving program mode and save that value for future prediction stops.

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

The old calibration workflow set floor positions in program mode, then on exit ran toward whichever end of travel was farther away, allowed the lift to reach speed, commanded a stop, measured the coast/deceleration distance, and saved that value as the stop calibration. The redesign should retain this measured stop-distance concept while storing it more robustly in MRAM.

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
- `GET /api/settings` - read local lift settings such as run speed and jog speed.
- `POST /api/settings` - update guarded local lift settings.
- `GET /api/logs/recent` - read the current in-memory recent event log.
- `GET /api/vfd/parameters` - list documented VFD parameter metadata.
- `GET /api/vfd/parameter?number=N` - request a VFD parameter read.
- `POST /api/vfd/parameter` - write a stopped-only, range-checked VFD parameter.
- `POST /api/move?floor=N` - request a move to a configured floor.
- `POST /api/stop` - request a controlled stop.

Fallback AP credentials are configured from `firmware/include/Secrets.h`, which is ignored by Git. Station network credentials are saved through the local web API.

Planned WebUI expansion includes local lift settings, RF pairing, VFD parameter read/write, current-limit/load-limit tuning, configuration backup/restore, and log export.

The preferred home-automation path is local REST API first, optional MQTT later, and Home Assistant as the bridge to Google Home. Google Home should not talk directly to the lift controller or bypass the controller's authentication, logging, and motion prechecks.

## Hardware Architecture Draft

The next board should be designed around these blocks:

- Exact `ESP32-S3-WROOM-1U-N16R8` external-antenna Wi-Fi module.
- Fused 120 VAC + neutral input, isolated 12 V supply, 5 V rail, and 3.3 V rail.
- VFD UART interface reproducing the field-used 3.3 V-to-5 V `TXS0104E` topology, with an assembled-board bench gate before lift connection.
- LS7366R SPI quadrature counter.
- Encoder input conditioning for the observed 12 V open-collector quadrature outputs, including isolation/level translation before the counter.
- SPI MRAM for live position snapshots, settings, recent event logs, and fault records.
- The selected N16R8 module's 16 MB flash and 8 MB PSRAM serve WebUI assets, OTA staging and noncritical data; no separate Rev-A QSPI device is planned.
- Hardware safety chain independent of application firmware.
- VFD enable/stop/brake control path that fails safe on MCU reset or watchdog timeout.
- Protected digital inputs for call buttons, RF receiver, limit switches, home switch, and safety loop.
- Mandatory RXM-418-LR RF receiver path.
- Protected 12 V MOSFET light output with jumper-selectable onboard or external 12 V supply, plus an optional low-voltage auxiliary output header.
- Watchdog and brownout detection.
- Surge/ESD/EMI protection suitable for outdoor wiring and a VFD enclosure.

The service/recovery interface uses dry-contact inputs: J43 provides SERVICE
KEY, HOLD-TO-RUN and GND; J44 provides SERVICE UP, SERVICE DOWN and GND. The
intended external hardware is a maintained keyed service switch, a momentary
hold-to-run control, and momentary UP/DOWN controls. These optically isolated
MCU inputs are authorization/command inputs only; they are not a safety-rated
enabling circuit and may not bypass the independent emergency-stop, final-limit,
watchdog or VFD motion-authority paths.

## Opening The Hardware Project On Another Computer

Clone or copy the complete repository and open `hardware/ElevatorLift.kicad_pro`
with KiCad 10. The custom and SnapEDA libraries are stored under `hardware/` and
registered through `${KIPRJMOD}` paths, so no `D:\Downloads` or user-profile
library path is required. Do not copy only the `.kicad_pro` file: the five child
schematics, board, project library tables, custom libraries and vendored
SnapEDA library directory must remain together.

## Physical Verification Checklist

The initial on-site observations are recorded in `docs/field-verification-log.md`.
Before final schematic release, complete the remaining checks below:

1. Confirm the estimated 40 mm enclosure height and 12 mm underside clearance, then measure connector/wire-bend keepouts and airflow.
2. Confirm the approximately 90 mm x 90 mm board, 86 mm mounting-hole centers, 2.56 mm hole diameter, screw size, and standoff material.
3. Old controller hardware: photograph both sides of the Particle/Xenon board and accessory Nano board, record IC markings, regulator parts, RF receiver wiring, level-shifting parts, optocouplers, drivers, relay part numbers, fuses, MOVs, terminal blocks, and any bodge wiring.
4. VFD serial interface: verify the field-matched `TXS0104E` circuit on the assembled board, including header pinout, idle voltage, common/reference, polarity, receive level, cable length, and bench behavior.
5. Power input: where 120 VAC can be tapped, whether it is upstream/downstream of the disconnect, available neutral, protective earth/chassis connection, fuse location, wire gauge, and connector style.
6. Existing 12 V loads: light voltage/current, inrush behavior, shared return path, connector type, and whether any solenoid/interlock output still needs power.
7. Encoder wiring: verify the observed 12 V supply, 270 ohm pullups, 270 ohm series resistors, `TLP291-4` path, exact encoder model, A/B idle voltage, cable/shielding, connector pinout, and maximum pulse rate.
8. Button, home, limit, and safety wiring: voltage levels, normally-open/normally-closed behavior, whether contacts are dry or powered, cable routing, and what is hardwired versus MCU-only.
9. RF system: confirm receiver module marking, antenna type/location, data output idle level, remote count, remote mapping, pairing behavior, and the separate wired `LEARN` node.
10. Grounding and EMI: cabinet earth strategy, shield terminations, VFD motor lead routing, separation between mains/motor wiring and control wiring, and any existing noise filters.
11. VFD parameters: dump or photograph current drive parameters, especially acceleration, deceleration, max frequency, current/overload settings, serial timeout, and any stop/enable terminal configuration.
12. Safety devices: identify final limits, emergency stop, gate/latch devices, brakes, and any non-MCU circuits that remove motion authority.

## Near-Term Project Plan

1. Review the completed Rev-A KiCad schematic, replace the remaining provisional connector/counter/MRAM/RTC library assets, and close actionable ERC findings.
2. Verify the first-pass connector map against the real wiring and physical verification checklist.
3. Bench-test the assembled field-matched `TXS0104E` VFD interface, header pinout, levels, and fault behavior before connecting to the real lift.
4. Define the MRAM data layout for position snapshots, settings, VFD parameter cache, remote registry, and event logs.
5. Implement the program-mode exit calibration workflow that measures and stores stop distance.
6. Implement LS7366R and MRAM firmware drivers behind the existing firmware interfaces.
7. Build a bench VFD serial simulator before testing motion logic on a real lift.
8. Add authentication/API tokens before any production WebUI or automation write operation.

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
