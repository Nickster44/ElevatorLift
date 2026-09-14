# Elevator Lift Controller Redesign

This repository is the starting point for a redesigned outdoor elevator lift controller. The original controller was a Particle Xenon sketch that drove an EM01 VFD over serial, watched a safety loop, counted position pulses in firmware, stored floor positions in EEPROM, and exchanged simple serial messages with a secondary relay/input board.

The new design goals are:

- Make motion control deterministic and auditable with an explicit state machine.
- Move encoder counting out of interrupt-heavy application code and into a quadrature counter IC.
- Store current position, floor targets, calibration values, faults, and event logs in nonvolatile MRAM.
- Host a local access-point web interface for setup, diagnostics, data logs, and maintenance actions, with optional station-mode connection to a local network.
- Keep safety functions electrically independent from the MCU wherever possible.
- Develop the custom controller board alongside a versioned firmware interface contract.

Important: this repository is not a certified elevator controller. The software here is a development baseline. Any real lift must use appropriately rated safety devices, hardwired interlocks, braking/enable circuits, limit switches, emergency stop hardware, enclosure design, and professional review before operation.

## Software Integration Status (2026-09-13)

The active software is a **buildable but hardware-inhibited N16R8 profile**.
GPIO35-37 are unavailable on this module. The 2026-09-13 hardware update moves
LIMIT_UP/LIMIT_DOWN/SERVICE_KEY to GPIO43/44/3 and makes GPIO42 a communications-only
UART enable, removing the MCU RUN and optional AUX circuits. Software contract v2
now adopts this revision. GPIO3 is input-only, UART0 application logging is disabled,
and UART1 supports STOP/monitor/readback while safety is open. RUN, drive writes,
motion and uploads remain inhibited. Stable input levels do not prove wire continuity.
No physical motion or hardware verification has been performed.

VFD communication now includes periodic STOP refresh (300 ms scheduled interval),
monitor/read traffic, and a separate bounded reply-retry policy. The drive's TIME
watchdog is read and reported, never automatically changed; communication failure
keeps STOP refresh active without restarting motion or clearing faults.

The manual-release STOP path is now target-bound: key/hold/direction release or
invalid direction/safety input queues STOP before ordinary work. Input intent is
reported separately from actual motion permission. The requested operational v3
profile, RF programming and calibration expansion are not complete; v2 remains inhibited.

See the [hardware change and routing handoff](docs/reviews/2026-09-13-pin-comms-fourlayer.md).
The PCB is synchronized and has four copper layers, but remains unrouted and
not layout-release/fabrication ready; isolation keepouts and fabricator-specific
stackup/USB geometry still need completion.

Start with the [software integration checklist](docs/software-integration-checklist.md),
[hardware-dependency handoff](docs/hardware-dependency-handoff.md),
[firmware guide](firmware/README.md) and [current API contract](webapp/docs/api-contract.md).
Portable motion/protocol/device logic has regression tests; target commissioning,
RF integration and several guarded API workflows remain incomplete. The WebUI
shows unavailable capabilities instead of simulating successful hardware actions.

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
- Motion control should use measured deceleration-distance stopping, not PID. Planned calibration starts explicitly after valid floor programming and stores direction-specific offsets; target integration remains incomplete.

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

## Firmware Direction

The firmware targets the exact N16R8 module using Arduino/PlatformIO and a versioned hardware interface contract. Portable core logic is tested separately from target adapters. A successful build is not permission to deploy.

Key modules:

- `core/Supervisor`: fault-latched motion, service, homing and measured calibration logic.
- `core/Em01`, `DriveScheduler`: framed/checksummed transactions, telemetry and readback.
- `core/Devices`, `Configuration`: LS7366R, PM004 MRAM, RTC and integrity-checked records.
- `core/Rf`: isolated learned-slot/epoch and input arbitration logic.
- `main.cpp`: inhibited target diagnostics, light output, networking and static hosting.
- `interface-contract.json`: confirmed pins and explicit unresolved hardware decisions.

## Local Web Interface/API

The firmware hosts static WebUI assets on port 80 and a fallback AP. Station credentials can be saved, but remote reboot is deliberately unsupported in the inhibited profile. Failed/lost station connectivity restores AP access. Blank API tokens disable writes.

Endpoints:

- `GET /` - embedded WebUI; no automatic demo mode.
- `GET /api/status` - versioned status, capabilities, blocked reasons and freshness.
- `GET /api/capabilities` - explicit supported capabilities.
- `GET /api/network` - current AP/station network status.
- `POST /api/network` - save station SSID/password and require a reboot.
- `POST /api/light` - idempotent light output command.
- `GET /api/logs/recent` - bounded durable MRAM events, unavailable on storage failure.
- `GET /api/vfd/parameters` - list documented VFD parameter metadata.
- `POST /api/move?floor=N` - strict input validation, then hardware-inhibited rejection.
- `POST /api/stop` - queues diagnostic STOP (202), or reports unavailable UART (503); neither transmission nor acknowledgement proves physical stopping.

Other commissioning, parameter-write, RF, restore and reboot endpoints are explicitly unsupported. See the API contract for remaining work; no UI capability implies hardware verification.

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
- External hardwired motion authority; GPIO42 enables UART translation only, not RUN or a brake.
- Protected digital inputs for call buttons, RF receiver, limit switches, home switch, and safety loop.
- Mandatory RXM-418-LR RF receiver path.
- Protected 12 V MOSFET light output with jumper-selectable onboard or external 12 V supply. AUX has been removed.
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

## Remaining Software Work

Drivers, integrity-checked MRAM records, token authentication and host protocol
simulations exist. Track remaining operational motion, RF capture/programming,
explicit calibration, guarded settings and update workflows in the
[integration checklist](docs/software-integration-checklist.md).

## Build Firmware Without Deployment

Install PlatformIO, then run:

```powershell
cd firmware
pio run
```

Upload and uploadfs deliberately fail in this profile. See the
[firmware guide](firmware/README.md) for host tests and filesystem builds and the
[WebUI guide](webapp/README.md) for mocked browser tests. Do not remove the inhibit
to try hardware; physical qualification remains separate.
