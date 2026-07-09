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
  jlcpcb-lcsc-sourcing.md           JLCPCB/LCSC-oriented major part matrix
  old-system-review.md              Existing firmware/hardware behavior and risks
  vfd-serial-protocol.md            Extracted EM01 serial protocol notes
  new-controller-requirements.md    Hardware and firmware requirements draft
  webui-vfd-and-storage-plan.md     WebUI, VFD parameter, and log storage plan
datasheets/
  README.md                         Local datasheet manifest for major ICs/modules
firmware/
  platformio.ini                    Starter PlatformIO target for ESP32-S3
  include/
  src/
hardware/
  ElevatorLift.kicad_pro            KiCad 10 project shell
  ElevatorLift.kicad_sch            Root schematic shell
  ElevatorLift.kicad_pcb            PCB shell with provisional 3.5 in x 3.5 in outline and corner mounting holes
  architecture-blocks.md            Block-level hardware architecture notes
```

## Current Design Knowledge

The current hardware direction is an ESP32-S3-WROOM-1U controller board in the VFD enclosure, using a self-hosted Wi-Fi AP as the primary service path and optional station Wi-Fi for local-network access. Ethernet is not a near-term requirement.

Known design constraints captured so far:

- KiCad 10.0.4 is the active hardware design version.
- Target PCB envelope is provisionally 3.5 in x 3.5 in with corner mounting holes.
- VFD serial is standard 9600 baud UART framing, but the VFD input is opto-isolated and needs a driver stage with enough current for the opto input.
- Encoder is believed to be the SKF Hall/open-collector quadrature unit. The datasheet recommends 270 ohm pullups at 5 V.
- Legacy Linx/TE RXM-418-LR 418 MHz RF remote support is mandatory.
- Baseline control supply is RECOM `RAC10-12SK/277`, 12 V, 10 W, because the previous accessory board used it and JLCPCB lists it as assembly part `C5199922`.
- The lift light is believed to be 12 V at about 750 mA, so the 10 W supply is tight if future solenoids are added.
- Critical state and recent logs should use 4 Mbit SPI/QPI MRAM, with Siproin `PM004MNIATR` as the current JLC-friendly candidate.
- Web assets and noncritical long logs should use MCU flash/LittleFS first, with optional QSPI NOR storage if the WebUI grows.

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

The preferred home-automation path is local REST API first, with optional MQTT/Home Assistant support later. Google Home should integrate through a local bridge or explicit integration layer rather than bypassing the controller's authentication, logging, and motion prechecks.

## Hardware Architecture Draft

The next board should be designed around these blocks:

- ESP32-S3-WROOM-1U or equivalent external-antenna Wi-Fi module.
- Fused 120 VAC input and isolated 12 V supply, currently centered on RECOM `RAC10-12SK/277`.
- 3.3 V buck regulation from the 12 V rail.
- UART opto-input driver and protected VFD receive path.
- LS7366R SPI quadrature counter.
- Encoder input conditioning for 5 V open-collector quadrature outputs.
- SPI MRAM for live position snapshots, settings, recent event logs, and fault records.
- Optional QSPI NOR flash for WebUI assets, OTA staging, noncritical logs, and exported data.
- Hardware safety chain independent of application firmware.
- VFD enable/stop/brake control path that fails safe on MCU reset or watchdog timeout.
- Protected digital inputs for call buttons, RF receiver, limit switches, home switch, and safety loop.
- Mandatory RXM-418-LR RF receiver path.
- Protected 12 V MOSFET light output and optional low-voltage auxiliary output header.
- Watchdog and brownout detection.
- Surge/ESD/EMI protection suitable for outdoor wiring and a VFD enclosure.

## Physical Verification Checklist

When the old lift system can be inspected in person, verify these items before final schematic release:

1. VFD enclosure space: usable width, height, depth, door clearance, standoff height, wire-bend clearance, airflow, and any metal keepouts.
2. Mounting pattern: old controller board size, exact mounting hole coordinates, screw size, chassis/standoff material, and whether the provisional 3.5 in x 3.5 in PCB fits.
3. Old controller hardware: photograph both sides of the Particle/Xenon board and accessory Nano board, record IC markings, regulator parts, RF receiver wiring, level-shifting parts, optocouplers, drivers, relay part numbers, fuses, MOVs, terminal blocks, and any bodge wiring.
4. VFD serial interface: terminal labels, idle voltage, common/reference pin, opto input current requirement, polarity, receive output level, cable length, and whether the old board used any transistor/resistor stage between MCU UART and the VFD.
5. Power input: where 120 VAC can be tapped, whether it is upstream/downstream of the disconnect, available neutral, protective earth/chassis connection, fuse location, wire gauge, and connector style.
6. Existing 12 V loads: light voltage/current, inrush behavior, shared return path, connector type, and whether any solenoid/interlock output still needs power.
7. Encoder wiring: exact encoder model label, supply voltage at the encoder, A/B idle voltage, pullup location/value, cable length/shielding, connector pinout, and maximum observed pulse rate.
8. Button, home, limit, and safety wiring: voltage levels, normally-open/normally-closed behavior, whether contacts are dry or powered, cable routing, and what is hardwired versus MCU-only.
9. RF system: confirm receiver module marking, antenna type/location, data output idle level, remote count, remote button mapping, and pairing/encoding behavior.
10. Grounding and EMI: cabinet earth strategy, shield terminations, VFD motor lead routing, separation between mains/motor wiring and control wiring, and any existing noise filters.
11. VFD parameters: dump or photograph current drive parameters, especially acceleration, deceleration, max frequency, current/overload settings, serial timeout, and any stop/enable terminal configuration.
12. Safety devices: identify final limits, emergency stop, gate/latch devices, brakes, and any non-MCU circuits that remove motion authority.

## Near-Term Project Plan

1. Create the KiCad schematic pages for power, MCU, VFD interface, encoder/counter, storage, RF/inputs, and light output.
2. Define the first-pass connector map from the old wiring assumptions and the physical verification checklist.
3. Bench-test the VFD opto UART driver current and polarity before connecting to the real lift.
4. Define the MRAM data layout for position snapshots, settings, VFD parameter cache, remote registry, and event logs.
5. Implement LS7366R and MRAM firmware drivers behind the existing firmware interfaces.
6. Build a bench VFD serial simulator before testing motion logic on a real lift.
7. Add authentication/API tokens before any production WebUI or automation write operation.

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
