# ElevatorLift end-to-end integration review

Review date: 2026-09-10. Baseline: `1c37a65` on `main`; see [source manifest](source-manifest.json) for the full commit and source hashes. This review concerns the saved repository design, not unrecorded changes in an open editor or an installed legacy controller.

## Assessment

**The pieces do not yet implement one operational controller.** They share a recognizable intended architecture, but the schematic describes a substantially more advanced controller than the firmware implements. The web application is a mixed live client and demonstration interface. The PCB is not fully routed.

- **Works together in offline tests:** firmware status JSON can be consumed by the actual web page handler; basic move/stop route names and token-header convention agree; EM01 transmit frames match manual examples; some provisional safety/limit checks, stop timeout, authentication, and NVS CRC checks execute successfully in host simulation.
- **Ready for limited development/bench work:** reproducible firmware builds, web builds, deterministic software simulations, and individual low-voltage peripheral characterization with motion outputs physically disconnected. A suitable development board and simulated UART/inputs can support firmware development. This does **not** mean the current custom PCB or complete application is ready to connect to a drive.
- **Prevents complete operation:** N16R8-reserved GPIOs used for limits/service; extensive firmware GPIO mismatch including reversed VFD UART; unimplemented enable control; coupled serial-enable/run-permit circuit; no LS7366R/MRAM/RTC drivers; missing valid-position/homing/calibration/service states; unsafe STOP/fault/freshness behavior; unsupported web operations and misleading live/preview transitions; missing embedded web delivery; unfinished PCB and independent safety/installation verification.

No new-controller function was demonstrated on powered hardware or verified end to end during this review. Historical field observations establish requirements and legacy behavior, not Rev-A validation. No mains was energized, no firmware uploaded, no drive connected, and no physical command sent. Production sources, library tables, schematics, PCB, firmware and UI were preserved. Only this review directory was added; nothing was committed or pushed.

## Evidence and methods

| Evidence | Result and limit |
| --- | --- |
| `pio run` in `firmware` | PASS. Espressif32 7.0.1, Arduino-ESP32 2.0.17, GCC 8.4.0. RAM 48,744/327,680 bytes; application 790,257/3,342,336 bytes. Built the configured **N8/no-PSRAM development target**, not a qualified N16R8 board target. |
| `npm run check` | PASS, TypeScript. Type correctness is not runtime response validation. |
| `npm run build:embedded` | PASS. 246,798 raw bytes / 73,849 gzip bytes across HTML/CSS/JS. Assets are generated, not integrated into firmware. |
| `npm test` | PASS: one server-rendered HTML smoke test. It does not call firmware or exercise controls. |
| `npm run lint` | PASS. No functional/safety assurance implied. |
| [Host C++ checks](host-results.txt), [runner](run-host.ps1), [source](host/review.cpp) | **37 observations confirmed**, including intentionally reproduced defects. Compiles the actual unmodified `main.cpp` and production support `.cpp` files against review-only Arduino/GPIO/Serial/HTTP/Wi-Fi/Preferences substitutes. No physical transports. Passing reproduction checks means the reported defect exists, not that safety requirements pass. |
| [Web handler checks](web-results.json), [runner](check-web.mjs) | **8 observations confirmed** using the actual transpiled `page.tsx`, fake hooks/JSX/fetch/timers, and JSON emitted by the actual firmware serializer. This is handler simulation, not browser DOM, real React scheduling, HTTP transport, Wi-Fi or electrical validation. |
| [Konnect results](konnect-results.json), [collector](collect-kicad.ps1) | All six saved sheets inspected; full review `status=complete`, no coverage diagnostics, 229 resolved symbols, zero unresolved. Board coverage 135 footprints, 496 pads, 144 named nets. Complete tool coverage is **not** complete electrical qualification. |
| [Exported netlist](current.net), [readable export](netlist-summary.json) | Fresh KiCad 10.0.4 export through Konnect. Hardware paths below derive from this export, not prior readiness claims. [Parser](inspect-netlist.mjs) reads exported data only. |
| [ERC](erc.json) | 0 errors, 31 warnings: 14 footprint-link diagnostics, 9 off-grid endpoints, 6 same local/global labels, 2 U11 pin-type notices. No new duplicate-power-reference or U30 multi-unit-value error reproduced. |
| [DRC](drc.json) | **343 unconnected items**, 191 other warnings; schematic parity list empty. Of the 191: 113 footprint-library differences, 14 missing-library diagnostics, 26 silk-over-copper, 26 silk-overlap, 12 silk-edge-clearance. NOT READY for fabrication. |
| Structural checks | Zero label-short/orphan findings on each sheet. Sheet-local single-pin counts 0/2/22/2/4/17 are mostly cross-sheet labels; full-hierarchy export has no unexpected single-node net (remaining single-node entries explicitly marked no-connect). Do not mistake sheet-local warnings for disconnected hierarchy nets. |
| Additional audits | Six sheet-local bulk-capacitance warnings and U40/U24 proximity warning. Shared-rail bulk exists (e.g. C20, C22, C24/C25, C15); audit warnings are not proof those rails lack bulk. Physical decoupling quality still depends on layout. |
| [Library resolution](library-resolution.json) | Konnect resolves project-local decoder symbol, corrected ESP32 footprint and USB footprint. Current ESP footprint has 49 pads (including nine ground segments); queried ESP/USB footprints have no linked 3D model. This does not validate every pad/mechanical dimension or prove another computer opens the project. |

The initial PDF extraction failed on console encoding and succeeded using UTF-8. The original EM01 manual pages 27–30 were read; parameter/example tables on pages 29–30 were also rendered and visually checked. Tool-discovery output parsing needed one correction; final Konnect checks completed. No build prerequisite remained missing. Physical, real-browser/live-server, brownout, RTOS timing and manufacturing qualification checks remain unperformed, as detailed below.

### Reproduce the offline checks

From repository root:

```powershell
# Requires existing local PlatformIO/toolchains and webapp npm dependencies.
Push-Location firmware
pio run
Pop-Location
Push-Location webapp
npm run check
npm run build:embedded
npm test
npm run lint
Pop-Location
& docs/reviews/2026-09-10-integration/run-host.ps1
node docs/reviews/2026-09-10-integration/check-web.mjs
```

`run-host.ps1` currently names the installed MSYS2 compiler path and writes its executable to the Windows temporary directory. Test secrets are fabricated in `host/Secrets.h`; no real credentials are needed. `collect-kicad.ps1` requires PowerShell 7 and the installed Konnect plugin and refreshes generated review evidence. [Build logs](build-results.json) were captured by [check-builds.ps1](check-builds.ps1). Host Preferences models success/failure/corruption, not ESP flash timing, wear, atomicity or Xtensa struct layout. Network substitutes do not simulate the production HTTP parser's blocking behavior.

## Intended system and functionality/continuity matrix

Requirements are synthesized from `README.md`, `docs/new-controller-requirements.md`, `docs/hardware-requirements-matrix.md`, `docs/motion-control-and-calibration.md`, `docs/field-verification-log.md`, hardware capture documents, and prior selections: **three landings**, existing five-button 418 MHz remotes (no new transmitter/encoder on this board), N16R8, top HOME near floor 3, LS7366R, PM004MNIATR, native USB, RAC10-12SK/277 and 12 V lamps, J43/J44 local service controls. Auxiliary output is provisioned; interlock/solenoid outputs and external asset flash/SD are not mandatory Rev-A functions. MQTT is optional, Ethernet deferred.

Completion vocabulary: **Planned** = requirement only; **Partial** = missing functional layers; **Implemented/untested** = code/circuit exists without applicable verification; **Automated subset** = stated behavior tested in simulation/build only; **Bench-tested** and **End-to-end verified** require physical evidence. No row qualifies for the latter two for this new design. “Captured” means schematic connectivity, not assembled hardware.

Source shorthand: **FW** `firmware/src/main.cpp`; **Pins** `firmware/include/PinMap.h`; **UI** `webapp/app/page.tsx`; **NET** fresh exported netlist above. Mismatch IDs link conceptually to the next section.

| Requirement | Physical/circuit-to-firmware path | API/UI/output or feedback | Current completion and evidence | Remaining gap |
| --- | --- | --- | --- | --- |
| PWR-001/002: 120 VAC to isolated 12 V | J20 L → F20/MOV → PS20 RAC10; PS20.5 → F21 → +12V; PS20.4 → R21 → GND | No rail-health telemetry | Captured/untested, NET Power | M05/M25: load/fuse/thermal and mains review; PCB routing |
| PWR-003/004: 5 V and 3.3 V | +12V → PS21/U21 → +5V; +12V → U20/L20 → +3V3 | MCU/peripheral supply | Captured/untested, NET | No full load/startup/source-change measurement |
| MCU-002: USB programming and recovery | J10 D−/D+ → U10 GPIO19/20, U12 protection; J11 UART0 and BOOT/EN | Build serial behavior/ROM recovery | Partial; data nets present | M04/M05: correct target, USB-only power missing, runtime CDC/upload verification |
| MCU-001: N16R8 controller | U10 exact module/CorrectedV2 footprint | All controller services | Partial; build passes a different memory configuration | M01–M04: reconcile actual GPIO availability and build profile |
| Three landing calls, signed upward-positive position | RF or HTTP → `requestMoveToFloor()` → target/count comparison → UART | `/api/move`, `/api/status`; three UI buttons | Partial; host accepts calls with provisional inputs | M02/M06/M07: hardware counter absent in software, four hardcoded firmware floors, no valid-position gate |
| RF-001: existing five-button remotes | J40 → U40 RXM-418-LR → U41 decoder → GPIO4–8 | FW button handler; UI imitation of layout | Partial; data nets match five GPIO numbers | M08: floor mapping is 3/4/STOP/2/STOP, no light/floor1; no source-aware arbitration |
| ENC-001/002: encoder conditioning | J30/J31 open-collector A/B → R30/31 2.2k → U30 → R32/33 10k → U31 Schmitt → U32 A/B | Intended counter-derived status | Captured/untested | M06/M24: no SPI driver; confirm current, direction, rate, missing-pulse/noise behavior |
| ENC-003: hardware quadrature counting | U32 4 MHz clock, 3.3 V, SPI shared with U11 | Intended 32-bit counter extended to signed system coordinate | Planned software; FW currently two increment/decrement ISRs | M06: configure registers/count width/filter, rollover and reset synchronization |
| ENC-004: top HOME/reference | J41.2 → U42B → HOME → U10 GPIO2 | FW reads GPIO10 and zeroes position while idle | Partial/incompatible | M02/M07: HOME origin conflicts with floor table; no seek/backoff/reference validation |
| Program floor positions | Counter/local controls → intended configuration records | UI Setup edits local component state only | Planned controller support | M07/M16: no `/api/floors`, persistence, range/order/tolerance or explicit position-valid state |
| Measured stopping-distance calibration | Counter + drive ramp → measured coast distance | Drive panel reports sample “current” calibration | Planned, not measured | M09/M16: no state machine/measurement/storage/invalidation; fake UI qualification |
| Predictive normal stopping | `remaining <= activeCommand.stopOffsetCounts` → STOP | stopping/idle status | Partial; threshold and timeout simulated | M09/M10: constant threshold ignores saved setting, overshoot, physical stopped verification |
| Safety loop monitor | J41.1 → U42A → GPIO1 active LOW healthy | FW GPIO9 → `safetyOk`; UI banner | Partial; provisional-input safety rejection tested | M02/M11/M15: actual safety wire not read; banner not full motion permission |
| Final upper/lower limits | J42 → U42C/D → GPIO35/36 | FW GPIO11/12; UI limit pills | Partial/incompatible | M01/M02: reserved GPIOs and wrong software map; external hardwired final-limit chain unverified |
| Independent motion authority/E-stop | Intended external chain; U25/J25 open collector under GPIO42 | No enable/watchdog driver | Planned safety integration; output captured | M03/M11: external circuit/terminal semantics missing; serial STOP alone insufficient |
| MCU reset/watchdog/power loss safe state | R24 pull-down, output gate pull-downs, reset network | Startup writes VFD settings; no safe shutdown sequence | Partial; problematic boot/reboot simulated | M10/M11/M20: no qualified watchdog path or restart permission policy |
| Sensor/VFD fault detection and latching | Inputs + counter + UART replies | FW Fault/lastFault | Partial; some faults and timeout tested | M10/M11: absent progress/idle-motion/VFD health checks; STOP clears latch |
| IN-003: keyed service recovery | J43 key/hold, J44 up/down → U43 → GPIO37–40 | UI service toggle local only; recovery panel locked | Planned software | M01/M12: no local-authority state/jog/deadman/timeout/channel selection or logs |
| VFD-001: EM019600 8N1 | U10 → U24 TXS0104E → J24 5V/GND/VFD RX/VFD TX | `VfdProtocol` TX + weak STOP ACK handler | Partial; transmit vectors automated | M02/M03/M10: GPIO swap, OE low, no robust parser/transaction freshness |
| VFD telemetry/alarm reporting | MONITOR frame → intended cached telemetry | UI fixed 0Hz/0A/326V/34°C and “Communicating” | Planned receive implementation | M10/M15: monitor not polled/parsed, faults cannot remove permission |
| VFD-003: parameter access | HTTP handlers send get/set commands | Metadata routes exist; Drive panel not connected | Partial; ranges/idle-write rejection subset tested | M13/M16: wraparound, no readback/completion, access labels unenforced, parameter13 ambiguity |
| RF-002: learn/cancel/erase | GPIO14 → D40 → U41.11 plus SW40; MODE_IND GPIO16 | Browser timers only | Captured hardware/planned firmware | M17: no GPIO driver/API, deliberate learn pulse vs erase hold, reset cancellation |
| RF-003: transmitter identity/names | U41.9 TX_ID → GPIO15 | Browser sample remote IDs | Planned | M17/M18: actual ID is reusable learned slot, capture/association/registry and reset reconciliation missing |
| OUT-001: light toggle | GPIO41 → R57/Q40 → J46; JP40 onboard/external supply | UI `/api/light/toggle`; FW no route | Partial circuit only | M16/M25: implement state, repeated-command semantics, load characterization |
| Optional auxiliary output | GPIO3 → R59/Q41 → J47 | No firmware/API | Captured, optional planned feature | Define intended use or declare unavailable; evaluate boot strap/output defaults |
| MEM-001/005: critical position/calibration/settings | U11 PM004MNIATR on SPI | `PositionStore`/`LiftSettingsStore` use ESP NVS | Partial substitutes; CRC round-trip/corruption simulated | M06/M19: no MRAM driver/schema/sequence/redundancy/validity or calibration record |
| MEM-004: offline time | U13 RV3028, J12 backup, GPIO47/48 I²C | Events use `millis()` | Captured/planned software | M02/M19: GPIO48 misused as LED, no RTC driver/backup qualification/time validity |
| Logs and retention/export | Intended MRAM+RTC+source-aware events | `/api/logs/recent` RAM32; UI hardcoded events/CSV button | Partial | M19/M16: persistent log/2048 setting/accepted-rejected reason/source/time and UI consumption absent |
| NET-001/002: AP/STA recovery | ESP Wi-Fi → setup/reconnect/AP logic | Network GET/POST; UI editor stub | Implemented/untested on hardware; compiles | M20: fallback behavior, credential validity, JSON escaping, slow client responsiveness |
| Local REST/auth/automation | `ApiAuth` header token, optional arg token | UI localStorage token, status/move/stop | Partial; configured-auth rejection and status data compatibility automated | M13/M14/M20: auth optional by default, no role/origin policy, no MQTT, weak validation/error detail |
| MEM-002: self-hosted WebUI/updates | Module flash and intended LittleFS/OTA | Embedded bundle builds separately | Partial build tooling only | M04/M21: root serves diagnostic HTML, no static file serving/filesystem image or OTA workflow |
| Backup/restore/versioning | Intended versioned settings/calibration/remote registry | UI success-like toast but no file or endpoint | Planned | M18/M19/M16: transaction/migration/reconciliation not implemented |
| UI freshness/permission/error recovery | HTTP status → page state → operator decisions | Three-second polling, catch switches demo on | Partial, defects reproduced | M14/M15: no schema/capabilities/timeouts, false online/idle/telemetry, discarded error reasons |
| MECH/EMI/portability | 90mm class outline/86mm hole centers, separate RF/Wi-Fi antenna paths, local libraries | Same saved project on another machine | Partial; hierarchy/libs resolve; DRC incomplete | M22/M23/M25: routing, qualified footprints, clearance/earth/enclosure and clean-clone validation |

## Interface mismatches and practical impact

### M01 — N16R8-reserved pins assigned to operational inputs — blocker

`hardware/MCU_Storage_RevA.kicad_sch`, U10 physical pin **28/IO35 = LIMIT_UP**, **29/IO36 = LIMIT_DOWN**, **30/IO37 = SERVICE_KEY**. Espressif identifies IO35–37 as unavailable for other uses on the octal-PSRAM variant. These cannot be treated as ordinary GPIO on the selected N16R8. Firmware also configures IO35/36 as interrupt inputs. A symbol with these pins drawn as ordinary bidirectional pins and an ERC pass do not make that mapping valid. Reallocate these functions, then establish one versioned hardware/software pin contract; do not “solve” it merely by enabling PSRAM in the build. [Espressif module datasheet, section 3 notes](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).

### M02 — GPIO and peripheral roles disagree — blocker

All U10 pin numbers below are module/symbol pins, not GPIO numbers. Firmware evidence: `firmware/include/PinMap.h:7` and FW `setupPins():709`, UART initialization `:734`.

| Intended net/function | U10 pin / GPIO from NET | Firmware actually assumes | Impact |
| --- | --- | --- | --- |
| Safety LOW=healthy | 39 / GPIO1 `SAFETY_MON` | GPIO9 | Reads MRAM chip-select net as safety instead of U42 safety output. |
| HOME LOW=active | 38 / GPIO2 `HOME` | GPIO10 | Reads LS7366R chip select as HOME. |
| Upper final monitor | 28 / GPIO35 | GPIO11 | Reads SPI MOSI instead of limit; desired pin reserved. |
| Lower final monitor | 29 / GPIO36 | GPIO12 | Reads SPI clock instead of limit; desired pin reserved. |
| VFD UART TX | 10 / GPIO17 `VFD_TX_3V3` → U24.A1 → J24.03/VFD RX | GPIO18 TX | Transmits on the line leading to VFD TX; possible contention when translator enabled. |
| VFD UART RX | 11 / GPIO18 `VFD_RX_3V3` ← U24.A2 ← J24.04/VFD TX | GPIO17 RX | Listens on the line leading to VFD RX. Swapping external wires alone does not fix OE or other mapping. |
| SPI CS MRAM/counter | 17 / GPIO9; 18 / GPIO10 | Safety/HOME pull-up inputs | No SPI chip selection or storage/count data path. |
| SPI MOSI/SCK/MISO | 19/GPIO11,20/GPIO12,21/GPIO13 | Two limit inputs; MISO unused | No SPI initialization/transactions. |
| Status LED | 23 / GPIO21 | GPIO48 | Fault indication drives RTC SCL instead; physical LED not controlled. |
| RTC SDA/SCL | 24/GPIO47,25/GPIO48 | Unused / LED output | RTC unavailable; SCL driven as non-I²C output. |
| VFD_ENABLE (OE and run) | 35 / GPIO42 | Not defined | Defaults low; UART blocked and run collector off. |
| Light / auxiliary | 34/GPIO41;15/GPIO3 | Not defined | Outputs cannot respond to commands. |
| Service key/hold/up/down | 30/37,31/38,32/39,33/40 | Not defined | Local service controls have no application path; key pin also reserved. |
| RF D0–D4 | 4/4,5/5,6/6,7/7,12/8 | Top/right/bottom/left/center | Electrical numbers agree; semantic button mapping does not (M08). |
| RF LEARN/TX_ID/MODE_IND | 22/14,8/15,9/16 | Not defined | No pairing control/identity capture/decoder-state feedback. |

### M03 — UART OE and run-permit coupled; neither implemented — blocker

NET `VFD_ENABLE` ties U10.35/GPIO42, U24.8/OE, R24 pull-down, and R25 feeding U25's LED. U25 collector/emitter go to J25.1/.2; LED return is logic GND. It is not an independent watchdog or safety relay. Firmware never sets GPIO42, leaving OE low and therefore translator outputs high-impedance. [TI TXS0104E](https://www.ti.com/product/TXS0104E).

Furthermore, enabling serial communication also asserts the run collector. Disabling run removes serial communication, including STOP/monitor/readback. A simple GPIO fix is insufficient: define separate communication and run-authority behavior (prefer independently controlled hardware paths), drive-terminal semantics, reset levels and sequencing. J24 order **01=5V,02=GND,03=VFD RX,04=VFD TX** matches the user's selection. Its common-ground translator is not galvanically isolated.

### M04 — Module/build/debug/storage target not aligned — blocker for target qualification

`firmware/platformio.ini:1` selects `esp32-s3-devkitc-1` without a pinned platform, flash/PSRAM override, or custom partition selection. Installed board metadata names N8/no PSRAM, uses `default_8MB.csv`, QIO flash and USB hardware mode 1. This default includes OTA-sized app regions and a `spiffs` data partition, but the application implements neither OTA nor filesystem serving. No `ARDUINO_USB_CDC_ON_BOOT` selection was found. ROM USB recovery and runtime serial-console behavior must be tested separately; missing CDC configuration is not proof ROM USB cannot upload. Pin resources, 16MB flash, octal PSRAM initialization, partitions and debug interface need a single explicit target profile. No additional asset-flash IC is required merely because software has not used the module's capacity.

### M05 — USB-only service power claim is false in current schematic

J10 VBUS → U14.1/VIN → U14.6/VOUT → +5V. U20.3/IN and .2/EN are **+12V**, not +5V. Consequently USB alone cannot power U10's +3V3 supply through the designed regulator path. `docs/schematic-capture-readiness.md`, “5 V source sharing,” says either source runs logic; only the 5V rail is shared. Choose and document a USB-powered logic path or explicitly require isolated external low-voltage power for service. Do not rely on parasitic backfeeding through USB data/peripheral pins.

### M06 — Encoder/counter/MRAM paths stop at unimplemented firmware

`Encoder_Counter_RevA` U32 SPI pinout matches the LS7366R data sheet: 4 CS,5 SCK,6 MISO,7 MOSI; 12 A,11 B; 14 +3V3,3 GND. Counter enable and inactive index are high; flag outputs unused. Firmware `onUpPulse()/onDownPulse():73` expects separate up/down pulses at GPIO35/36, not a SPI quadrature count. U31's outputs terminate at U32, not those GPIOs. Merely swapping GPIO definitions will not obtain position. [LSI/CSI LS7366R datasheet](https://lsicsi.com/wp-content/uploads/2021/06/LS7366R.pdf).

U11 PM004MNIATR uses shared SPI with its own CS. No driver exists for either device. Future drivers need per-device transactions, deasserted CS at startup, 32-bit counter configuration and rollover handling, count validity after reset, MRAM startup/mode commands and integrity checks. There are no explicit CS pull-up resistors on U11.1/U32.4 in the export; firmware safe boot configuration and hardware bias need review.

U11.3/.7 are named `~WP/SIO2` and `~HOLD/SIO3`, tied to +3V3. The local Siproin v1.34 pin-description table calls these SIO2/SIO3, not conventional flash WP/HOLD. This is **single-bit SPI only** as wired; do not issue quad/QPI transfers with these pads strapped. The data sheet's unused-pin wording and power-cycle requirements need confirmation before claiming a generic FRAM/flash driver is compatible. It specifies finite endurance (10^8 cycles), so “MRAM” alone does not establish unlimited application write life. See `datasheets/Siproin_PM004MNIATR_4Mbit_SPI_MRAM_datasheet.pdf`, sections 1–3 and 8/11.

### M07 — Floor model, HOME origin and position validity conflict — motion blocker

`LiftConfig.h:22` has **four** floors at 0/20,000/40,000/60,000. UI `:35` has three at 0/18,420/39,250 and `:148` uses fixed 8,000/27,000 thresholds to label the current landing. Editable floor values do not drive that calculation. FW `findFloor():126` always reads constants. Top HOME near floor 3 is documented in `new-controller-requirements.md:26`; FW `:632` sets position zero whenever HOME is active while idle, colliding with floor1=0 and positive higher-floor coordinates.

FW `setup():736` treats missing/corrupt NVS as position zero and can enter idle; there is no position-valid, unknown-position, homing or floor-programming state. A CRC-valid old count is also not proof the car did not move during a power outage. Host tests reproduce invented-zero startup and top-HOME coordinate reset. A controller-owned coordinate model, validity/recovery procedure, configured landings/tolerance and nullable current-floor status are required before normal moves.

### M08 — Five RF buttons have incompatible semantics and arbitration

FW `serviceButtons():681` maps GPIO4/D0 → floor3, GPIO5/D1 → floor4, GPIO6/D2 → STOP, GPIO7/D3 → floor2, GPIO8/D4 → STOP. There is no remote floor1 or light toggle. Exact legacy physical-button-to-D-bit association remains a measurement, but this cannot implement **any** permutation of the required three floors/STOP/light because light is absent and floor4 is extra.

`risingEdge():674` initializes previous states false, so a held HIGH at startup is treated as a new command. Simultaneous floor requests choose code-order winner; later STOP takes precedence in that iteration but can also change Fault to Stopping. RF and HTTP call the same basic request function, but no central source/priority/debounce/release-to-rearm policy exists. LATCH at U41.7 is correctly grounded for momentary outputs; RX_CNTL at .8 grounded is a supported “unused receiver-control mode,” **not an output short**. Validate actual remote baud and full packet/button behavior.

### M09 — Stopping setting is accepted but unused; calibration absent

POST `/api/settings` saves `stopOffsetCounts` (`FW:397`); `requestMoveToFloor():184` copies the fixed floor table's 3,500 instead. Host test saves 17, then verifies active threshold remains 3,500. Run-speed updates are accepted during movement and affect the next run frame. Calibration validity is not invalidated by speed or VFD changes. `serviceMotion():640` takes absolute remaining distance: passing beyond the stopping window makes remaining distance grow, so motion may continue in the original direction. No maximum-travel/progress/time envelope contains this failure.

### M10 — STOP/fault/drive-feedback semantics are unsafe — motion blocker

`VfdProtocol.cpp:isStopAck` searches a substring. FW `serviceVfdRx():610` retains replies received outside Stopping, then consumes any old `(3)84` when a later stop starts. No framing, checksum validation, transaction freshness or RX-echo discrimination. Receiving a command acknowledgement immediately sets Idle; it is not proof physical deceleration/braking finished. Monitor status is never queried/parsed.

`beginStopping():151` accepts a Fault state; a subsequent ACK sets Idle without explicit fault reset. `enterFault():142` sends one STOP then ceases retries and does not remove hardware enable. `serviceMotion()` detects neither prolonged no-count/no-VFD-reply motion nor unexpected idle movement. Missing STOP ACK does cause a two-second timeout fault, but the fault path is not qualified. These are reproduced against real application logic. Missing VFD/counter drivers also mean physical fault detection cannot currently work even if individual state-machine branches pass.

### M11 — Independent safety authority is a requirement, not a verified circuit

J41/J42 monitor circuits and J43/J44 service contacts feed the MCU. No exported path puts their contacts in series with U25/J25 permission, nor is an independent external watchdog/safety controller/brake loop captured. The installation may contain an external chain; this review cannot verify it. It must remain a release prerequisite, not an assumed protection.

FW `initializeVfd():599`, called from `setup()`, writes acceleration, deceleration, timeout and maximum frequency before deciding safety status, with no ACK and no initial STOP. With safety open at boot it enters Fault without calling `enterFault()`; the host observes four parameter writes and no STOP. `/api/reboot` schedules reset even while moving (`:519`, `loop():772`). Pull-downs can establish off states after reset, but do not detect a processor stalled with its output HIGH. No application-owned watchdog/authority-removal logic or proof of independent firmware-lockup shutdown exists.

### M12 — Physical service station has no runtime owner

J43 pins1/2/3 are key/hold/GND; J44 pins1/2/3 up/down/GND. R45–48 feed opto LEDs from +12V; closing a dry contact sinks the LED current and pulls the corresponding 3.3V collector LOW via U43, with R53–56 pull-ups. No FW pins/state/jog/timeout/conflicting-direction handling exists. UI `serviceMode` at `:106/:294` is local display state only. Key + hold + direction must be sampled continuously by firmware, combined with independent safety permission; releasing any required input must stop. Web service configuration cannot grant local physical authority.

### M13 — VFD routes compile but lack completed transactions and strict validation

FW routes `:425/:450` return 202 with literal `readbackParsing:"pending"` or `readbackVerification:"pending"`, but there is no asynchronous completion store/poll/readback implementation. GET can send UART during movement without authentication or a transaction arbiter. POST has idle/range/read-only checks but no distinct installer/advanced authorization despite metadata labels.

Parameter number is narrowed to uint8 before validation: 266 becomes FMAX10. `toInt()` accepts numeric prefixes. Parameter13 read behavior in original manual p30 is input1/input2/input3/relay digit status, whereas the single metadata definition calls it boolean RELE. Startup overwrites ACC/DEC/TIME/FMAX every reset (1s/1s/0.1s/120Hz), undermining persistence of drive tuning. The 100ms command refresh equals the requested 100ms drive timeout, leaving no demonstrated scheduling/wire-time margin. No receive health, bus voltage/current/temperature/overload alarm or calibrated load-limit behavior is implemented.

### M14 — API validation, versions and errors are not a complete contract

FW `/api/move` at `:540` narrows `toInt()` to uint8: floor258 becomes floor2; `2junk` also succeeds. Motion rejection is generic `{error:"move_rejected"}`, not the documented reason/message. Settings silently ignore advertised `homingTimeoutMs`/`logRetentionRecords` updates and have no per-field unknown-key rejection. Inputs are query/form arguments, not a general JSON body contract.

UI `apiFetch():112` verifies only HTTP success and JSON content type; no runtime schema/API revision/capability validation, timeout/AbortController, or out-of-order response guard. An empty JSON object reaches rendering and crashes in simulation. UI collapses controller error details into generic messages and ignores move/stop response state before polling again. Firmware emits signed 64-bit counts as JSON numbers, but JS Number is exact only through 2^53−1: choose an explicit bounded count domain or string encoding for the long-term contract.

### M15 — UI can present false operational feedback — blocker for operator use

UI `:101/:103` initializes healthy demo data with `demoMode=false`, displaying Controller online before contact. A failed poll at `:121` sets demoMode but preserves prior real status; `lastUpdated` changes even on failure. At `:176`, STOP in demo mode changes state locally without attempting `/api/stop`. The handler test establishes live moving status from real firmware JSON, disconnects, presses STOP and observes local Idle with no outgoing request. A demo move timer at `:164` can later overwrite reconnected live data.

`UI:248` derives “All interlocks healthy” only from safetyOk. `:298` always says drive Communicating with fixed values, even on failed API. `:276` equates Idle to stationary/ready despite firmware's ACK weakness. These must become explicit connecting/stale/offline/preview states with authoritative permission and freshness. Preview must be opt-in and isolated from a previously connected machine. Web STOP is still not an emergency-stop circuit.

### M16 — Most visible controls are not connected

Only four UI `apiFetch` paths exist: status, move, stop, light/toggle. Firmware has no light route or GPIO driver, so that one returns 404. `SetupPanel:337`, `DrivePanel:369`, `RemotesPanel:391`, `LogsPanel:436` and `SystemPanel:440` largely use local state/toasts/sample data. No live floor save, speed save, calibration, VFD page read/write, RF API, persistent log rendering/CSV, network editing, backup/restore or reboot call is wired from those panels. Unsupported controls remain usable when basic status connects. “Calibration is current,” decoder-erase-complete and configuration-backup-prepared messages are not hardware results.

### M17 — RF configuration/timing contract incomplete

JP41/42 strap decoder baud; there are no pull resistors on their center nets, so unpopulated shunts leave inputs floating. Define assembly defaults and verify legacy rate. RXM-LR is rated to 10kbps; not every selectable decoder rate is usable through this receiver. FW does not configure a receiver for TX_ID or GPIO drivers for LEARN/MODE_IND. The browser's 17-second timer is not a 17-second HIGH command: learning uses a momentary LEARN assertion, whereas holding it HIGH for ten seconds clears memory. Separate bounded controller transactions and physical feedback are required. Linx also specifies 2.0–5.5V decoder supply and permits grounded RX_CNTL when unused, so U41 at 3.3V and .8 at GND are not defects. [Linx-authored decoder manual, mirrored PDF](https://datasheet.octopart.com/LICAL-DEC-MS001-Linx-datasheet-132900.pdf).

### M18 — RF identity is a reusable slot, not a permanent transmitter ID

The same Linx manual's TX ID section specifies an eight-bit learned-order number transmitted at the selected baud. Slots can be reused/overwritten. UI sample 32-bit-looking IDs and documentation about restoring names when an “ID” reappears must not be taken as stable physical identity. Include a decoder learning/erase generation and explicit re-association policy; restoring a profile and merely seeing the same slot can associate the wrong remote. No registry firmware exists yet. This is a contract-design finding, not a claim that a currently implemented registry corrupts data.

### M19 — Persistence/logging/defaults/versioning stop at substitutes

`PositionStore.cpp` stores one CRC-protected raw struct in NVS `lift/state` (magic LFT1, version1). `LiftSettings.cpp` similarly uses `settings/lift` (LSE1, version1). No sequence/redundant record, last-motion marker, floor configuration, calibration provenance or recovery validity. Raw C++ layout/padding is not a portable versioned wire/storage format; host CRC tests do not certify target ABI or power-loss atomicity. `servicePersistence():699` writes every250ms and ignores failure. No retention/wear budget is calculated for actual writes.

`NetworkConfig` CRC record lacks explicit schema version; credentials are stored, not returned, but `jsonNetworkStatus():309` fails to JSON-escape SSIDs. `EventLog.h` capacity is32 RAM records despite advertised2048; reset erases events; milliseconds are not RTC timestamps. Fault events omit reason/source; “MotionStopped” is logged when STOP is requested. UI event schema `{title,time,detail,level}` does not consume firmware `{seq,ms,code,a,b}`. Firmware speeds default45/25/20Hz, UI displays45/10/8; saved stopping offset is not used (M09). No config backup/restore implementation or migration exists.

### M20 — Network servicing and command authority lack timing/security qualification

AP fallback and optional STA/mDNS code exists, but real connection/recovery was not tested. AP is disabled when STA connects, so “AP primary service path” should not imply always-on access. Built-in diagnostic HTML at FW `:338` sends writes without the token header, making its buttons fail when auth is configured. `Secrets.example.h` permits empty API token; the host suite explicitly uses a nonempty test token. Deployment credential policy is not established by this review. No separate roles, request-origin/CSRF policy or audited client identity exists.

FW `loop():764` calls synchronous `server.handleClient()` before motion servicing. In the installed Arduino2.0.17 library, `WebServer.h:49` has 5s data/POST/send waits; `Parsing.cpp:45` waits for body data with `delay(1)` before returning. Thus “nonblocking motion loop” is not a proven end-to-end responsiveness guarantee. Slow-client/body tests and a bounded scheduling architecture are required, especially with a100ms VFD timeout. This is source evidence of a blocking path, not a measured MCU latency result.

### M21 — Web bundle/update mechanism not installed in firmware

Embedded Vite build succeeds; FW GET `/` returns a small inline diagnostic page, not the React console. No filesystem mount, asset handler, gzip/cache/MIME handling, SPA fallback, firmware update route or filesystem build/upload step exists. The cloud/Vinext build is not a substitute for same-origin MCU API hosting; no proxy/remote-device bridge was verified. A controller-hosted deployment and version-compatible rollout/rollback contract must be implemented and tested.

### M22 — Schematics and PCB net identity agree, fabrication readiness does not

Fresh parity list is empty, but343 connections are unrouted. Matching net assignments do not physically connect pads. DRC library differences and silkscreen findings need reconciliation, not a blanket waiver. Claimed 90mm envelope/86mm mounting centers do not establish screw diameter, actual clearances, wire-bend access, U.FL/SMA cable paths, thermal/EMI suitability or protective-earth integrity. A numerical netclass clearance is not a safety-standard qualification.

### M23 — Component/package and library claims need correction before assembly

NET D41/D42 values are **SMBJ15A** but footprints are `Diode_SMD:D_SMA`; D20 with the same value uses `D_SMB`. SMBJ15A is an SMB/DO-214AA package, so D41/D42 part-to-land-pattern assignments disagree. Select/verify the actual TVS package and polarity and then update through Konnect, not by blindly refreshing all footprints. [Manufacturer SMBJ15A package](https://www.aosmd.com/products/tvs/high-power-tvs/smbj15a).

D40 is generically “BAT54” with `D_SOD-323`: obtain the full manufacturer/package suffix rather than assuming all BAT54 variants match. `ED300/3` remains an Altech substitute land pattern; Phoenix1717732 is explicitly provisional. U11/U32/RTC and U10 exposed-pad segmentation need drawing-based qualification. `component-library-request.md` still calls RTC footprint LGA-8_2x3mm, while NET is `MicroCrystal_C7_SON-8_1.5x3.2mm_P0.9mm`; it lists TPD2EUSB30DRTR while NET U12 is **TPD2EUSB30ADRTR**. Downloaded-library presence is not independent pinout verification.

### M24 — Field “isolation,” polarity and timing assumptions need qualification

Encoder/safety/home/service optos share **GND** between field connectors and transistor returns. They offer optical signal transfer/level conditioning but do not form a galvanic isolation boundary for those ports. Independent mains isolation is a different function. Firmware active-low assumptions match opto collector behavior in principle, but are attached to wrong GPIOs; exact NC/NO contact semantics, shorts-to-GND and cable-open diagnostics remain unresolved.

Encoder R30/31=2.2k implies roughly5mA when sinking at12V (estimate using~1.2V LED drop), not the legacy270-ohm loading. U30 timing/CTR at that current and R32/33=10k collector pull-ups may limit edge fidelity; the4MHz counter clock does not guarantee the opto front end passes the required rate. No actual encoder max speed/PPR/cable trace was captured here. Both quadrature channels are optically inverted then Schmitt-inverted, but direction/sign still needs physical verification. No firmware filter/error budget currently exists.

### M25 — Supply and load protection are not yet coordinated

RAC10-12SK/277 is12V/840mA nominal; a750mA lamp leaves90mA at12V before controller/converter/encoder/input loads. Prior successful lamp operation is useful evidence but not a Rev-A combined-load test. F21 is0.75A hold and carries all onboard12V current, including the onboard-selected lamp; its hold rating equals the lamp estimate before other loads. Thermal derating and inrush can matter. JP40 allows an external lamp supply; test/select it based on measured margin rather than silently replacing the user's chosen AC/DC module.

F20/MOV/R21 supply-return link, fuse interrupt rating/inrush coordination, regulator thermals, output TVS/clamp/load behavior and external-supply grounding need qualified values. J20.3/CHASSIS connects USB shields; it is not a documented protective-earth bonding scheme for the enclosure. The safety review must cover the complete installation, not just PS20's module certifications. See local RECOM data sheet selection/load tables and NET Power/IO.

## API contract inventory

| Interface | Actual firmware result/body | UI today | Compatibility/completion |
| --- | --- | --- | --- |
| GET `/api/status` | 200, state/counts/targetFloor/speed/safety/home/limits/last-command/fault/network | Poll3s; same field names | Data shape tested; semantics/freshness/permission incomplete. No version, capabilities, validity, VFD health, light state. |
| POST `/api/move?floor=N` | 202 status,401 unauthorized,400 missing,409 generic rejected | Uses route/header | Partial; narrowing/validation/prechecks and reason handling defective. |
| POST `/api/stop` | 202 status or401 | Uses route unless preview mode | Partial; fault latch and offline handling unsafe. |
| POST `/api/light/toggle` | Absent | Expects JSON `{on}` | Incompatible/404. |
| GET/POST `/api/settings` | GET six fields; POST accepts four numeric form/query fields, saves NVS | Local-only setup | Partial; fields/defaults/actual motion settings disagree. |
| GET `/api/vfd/parameters` | Definitions with min/max/default/scaling/access, no live values | Static table | Planned connection; metadata is not readback. |
| GET/POST `/api/vfd/parameter` | 202 sent/pending; POST has idle/range/read-only/auth checks | No calls | No completion/readback protocol or displayed result. |
| GET `/api/logs/recent` | Array of at most32 numeric-code/millisecond RAM events | Static event models | No adapter/fetch/persistence/pagination/CSV. |
| GET/POST `/api/network` | Status/write-auth flag; stores form/query SSID/password,202 restartRequired | Editor toast only | API partly implemented; unsafe JSON string construction and real recovery untested. |
| POST `/api/reboot` |202 schedules restart after1s | Toast only | No live UI path; FW accepts moving state. |
| Floors/remotes/learn/erase/calibration/config backup/restore/recovery | Absent | Local preview or disabled explanation | Planned; must not be advertised as operational. |
| GET `/` / static assets / OTA | Inline diagnostic HTML / no React asset handler / no OTA handler | Separate web build | No integrated shipping/deployment mechanism. |

## Control authority and failure behavior

| Decision or failure | Intended owner | Present behavior | Required boundary |
| --- | --- | --- | --- |
| E-stop, final limits, brake/drive inhibit | Independent hardwired equipment | External topology unverified; board only monitors contacts plus one MCU-driven collector | Establish/test independent removal with MCU unpowered/stalled. Never grant this authority to UI/RF alone. |
| Normal motion acceptance | Firmware supervisor under hardwired permission | Idle+safety+known constant floor only | Add valid position/calibration/drive health/directional limits/conflicts, re-evaluate continuously. |
| Web/RF/local priority | One firmware arbiter | HTTP then RF code-order handling; service absent | Deterministic STOP/fault priority, source tagging, release-to-rearm and simultaneous-input policy. |
| Keyed service | Physical key+hold+direction, firmware limits, external safety | No driver/state; UI toggle changes only local state | Local continuous consent, bounded speed/time, no remote normal moves or safety bypass. |
| Communications loss | Firmware drive-health supervisor and independent inhibit | Continues run without replies; only stopping timeout exists | Freshness deadlines, deterministic fault/stop/inhibit; verify real drive timeout independently. |
| STOP complete | Firmware based on fresh response plus stopped evidence | Substring ACK immediately becomes Idle | Distinguish command accepted, decelerating, stopped, fault; validate physical position stability. |
| Browser disconnected | UI must withdraw claims, firmware remains authoritative | Automatically enters demo; STOP may be local-only | Explicit offline state, no invented success, bounded best-effort operational STOP feedback. |
| Reset/brownout/reboot | Hardware defaults plus controlled firmware startup | GPIO pull-downs present; app overwrites drive parameters, assumes restored/default position; moving reboot allowed | Boot inhibit, source reconciliation, no unintended startup command, power-loss validity and controlled restart. |
| Storage/sensor failure | Firmware validity/fault supervisor | NVS failure ignored; encoder/sensor validity absent | Fail closed for normal motion, explicit logged recovery, no fabricated zero/calibration. |

RF authentication by the decoder is not equivalent to safety permission; the Linx receiver guide itself warns against treating radio data as an independent safety mechanism. UI disabled buttons are not protection against direct API/RF/local requests. Current electrical interlocks do not demonstrate a safety-rated enabling device or certified lift controller.

## Prioritized integration backlog

All entries are proposals, not applied fixes. P0 blocks operator/motion integration; P1 blocks a functional Rev-A release; P2 addresses verification/documentation after foundational decisions. “Blocker” describes dependency/unsafe integration, not an instruction to energize hardware after one fix.

| Priority/type | Work item and references | Acceptance condition / dependency |
| --- | --- | --- |
| P0 blocker B1 | Freeze N16R8-compatible pin/peripheral contract; M01–M04 | No reserved pins; firmware/generated pin map agrees with NET; UART roles correct; explicit board target. Konnect `edit_schematic_component`/wiring tools for approved changes, then export/compare/ERC/DRC. |
| P0 blocker B2 | Separate serial availability from motion permission and define independent hardwired chain; M03/M11 | Drive can remain inhibited while STOP/monitor/config communicates; stalled/unpowered MCU cannot sustain permission; externally reviewed wiring/terminal semantics. |
| P0 blocker B3 | Implement valid-position + counter + guarded motion/fault state machine; M06–M10 | Unknown-position startup; top-HOME model; no movement without progress/drive health/calibration; no overshoot continuation; no STOP fault bypass. B1/B2 first. |
| P0 functional defect F1 | Fresh framed EM01 parser/transaction manager, stop completion, watchdog timing; M10/M13/M20 | Reject stale/invalid/echo replies, serialize reads/writes, bound retries; actual stopped evidence and loop timing under client load. |
| P0 functional defect F2 | Remove automatic live-to-demo transition; M14/M15/M16 | Offline never reports physical success/idle; cancel isolated demo timers; schema/capability/freshness validation; truthful telemetry and error display. Required before operator-facing use. |
| P0 blocker B4 | Resolve PCB/package/mains readiness; M22–M25 | D41/D42 correct package, qualified connectors/critical footprints, completed routing, reconciled DRC/library differences, independent mains/earth/mechanical review. No manufacturing release before this. |
| P1 functional defect F3 | Strict API validation/reboot/settings permissions; M09/M13/M14/M20 | Reject wraparound/junk/unknown fields; idle/authority guards; no moving restart; meaningful reasons; explicit auth/role/origin policy. |
| P1 implementation I1 | PM004MNIATR/RTC drivers and versioned persistence; M06/M19 | Shared-SPI tests, power-cycle recovery, sequence/CRC records, validity semantics, finite-write budget, real timestamped durable logs. |
| P1 implementation I2 | Floor programming, homing and measured calibration; M07/M09 | Three authoritative landings, top reference, bounded calibration sequence, direction/scale and tuning provenance, stale-calibration inhibition. Depends on B3/F1/I1. |
| P1 implementation I3 | RF mapping/identity/learn/erase and arbitration; M08/M17/M18 | Five required actions, verified baud/slot capture, bounded learn vs erase, generation-aware registry, logs, no unintended boot/repeat/conflict commands. |
| P1 implementation I4 | Local service controls; M12 | Key/hold/direction continuously enforced; simultaneous up/down rejected; release/timeout/fault removes command; normal RF/web moves unavailable throughout service. Depends on B2/B3. |
| P1 implementation I5 | Light/optional auxiliary drivers + API; M16/M25 | Safe reset polarity, authoritative state, predictable retries/multi-client behavior, measured supply/load. State-setting preferred over untracked toggle. |
| P1 implementation I6 | Self-hosted UI and remaining endpoints; M16/M19/M21 | Versioned same-origin assets, bounded transfer handling, actual panel reads/writes, no mock success; explicit update/rollback policy. |
| P1 verification V1 | Encoder/VFD/RF electrical/baud/latency tests; M24/M17/M13 | Recorded scope/logic-analyzer traces and configured constants; test plan below. |
| P1 verification V2 | Combined power, USB service, source loss and brownout; M05/M25 | Documented power budget and supply selection; no backfeed, reliable boot, no unsafe outputs. |
| P1 verification V3 | Independent safety and integrated acceptance | Qualified external review plus witnessed results under controlled, separately authorized procedures. Software tests do not replace it. |
| P2 documentation D1 | Update claims listed below after decisions, not before | Requirements distinguish current implementation from target, exact parts/pins/API schemas, verified test records and revision IDs agree. |
| P2 verification V4 | Clean-clone portability and repeatable dependencies; M04/M23 | Destination KiCad standard/project libs and LFS assets resolve; lock/pin toolchain; CI runs contract/behavior tests, not just builds. |

### Documentation conflicts and stale claims

| Document/section | Claim versus verified state | Proposed correction |
| --- | --- | --- |
| `README.md` Hardware architecture/service station; `hardware/README.md` assumptions | “Optically isolated” field inputs share GND. | Describe optocoupled level conditioning/common reference; separately name true isolation boundaries. |
| `docs/schematic-capture-readiness.md`,5V sharing | Either USB/onboard source runs logic. | USB powers5V only in current NET; 3V3 requires12V. |
| `docs/component-library-request.md` | RTC LGA footprint; TPD2EUSB30DRTR; blanket verified/imported wording | Actual RTC C7 SON footprint, U12 ADRTR, explicit verification status per critical part; add D41/D42 and D40 package qualification. |
| `hardware/reports/schematic-review-summary.md`, light gate | Requires external lamp feed unless larger AC/DC used | Newer field/selection docs allow RAC10 with measured margin; retain measurement gate without silently mandating a larger supply. |
| `hardware/README.md:50` | “0.8mm SELV power routing” can be read as clearance | Readiness report says0.8mm track/0.2mm clearance; label widths versus clearances explicitly. |
| `docs/hardware-requirements-matrix.md`, MEM-003/PWR-003/ENC-002 | Optional QSPI/SD decision,5V RF load, legacy270Ω front end | RevA asset flash is module-internal; RF actually3V3; current2.2k frontend is deliberate but unverified, not a literal copy. |
| `hardware/architecture-blocks.md`, encoder/reference/flash chip selects | Reference architecture still mentions legacy passives/flash selection | Keep historical observation distinct from current captured net/pin contract. |
| `docs/snapeda-library-import.md` | Historical refdes/package statements | Reconcile against current J10 USB, U41 decoder footprint and corrected assets; do not treat import history as live BOM. |
| `README.md` starter/API descriptions | “Nonblocking” and “stopped-only” may overstate guarantees | Some handler/body paths block; Idle does not establish physical stopped; readback is pending forever. |
| `webapp/docs/api-contract.md` | Preview-on-failure rule permits risky connected-to-preview transition | Separate opt-in demonstration from offline live controller; define validity, capabilities, errors and completion. |
| `webapp/docs/firmware-integration-gaps.md` | Correctly requires authoritative state and unsupported controls disabled | Requirements are still **not implemented**; link concrete tests and tracked fixes instead of implying the handoff resolved them. |
| `docs/new-controller-requirements.md`/UI remote restore text | Observed ID reappearance treated as remote identity | Learned-slot reuse needs generation and re-association; not a permanent transmitter identifier. |

Older readiness reports' numerical ERC/DRC results largely reproduce. Their limited “bench planning, not manufacturing” conclusion remains appropriate; they do not establish software compatibility. No historical documentation was rewritten during this review, preserving evidence and the user's review-only boundary.

## Remaining integration test plan

Environment labels: **S** simulation/host; **L** powered isolated low-voltage logic board/development fixture; **P** peripheral fixture with drive motion physically inhibited/disconnected; **A** assembled system under a separately authorized, qualified procedure. No L/P/A tests were executed here. Never run current `setup()` against a real VFD merely to read parameters: it writes settings on startup. Build a read-only/simulation fixture first. Power-board tests should use an isolated current-limited low-voltage source with the mains section disconnected until a qualified mains procedure exists.

| Test / gaps | Prerequisites | Procedure | Expected acceptance result | Environment |
| --- | --- | --- | --- | --- |
| T01 Pin/variant contract (M01–04) | Approved N16R8 mapping and Konnect updates | Export NET; compare every GPIO/peripheral/polarity to firmware config; compile target; assert no reserved pins | Exact agreement; no accidental UART reversal/CS/LED conflict; expected16MB/8MB configuration | S, then L for identification |
| T02 Power/USB-only (M05/25) | Qualified passive BOM, low-voltage fixture, current limiter, no drive | Apply12V alone, USB alone, both, remove each; record3V3/5V/12V, reverse currents, EN and resets | Behavior matches declared service-power design; no parasitic boot/backfeed/unsafe output; stable rails at load | L |
| T03 Boot/reset/watchdog (M03/11) | Separate inhibit/serial design; dummy load; scope | Reset, BOOT mode, software stall, watchdog, brownout, failed NVS, safety-open boot | Inhibit remains safe; no startup RUN/unapproved settings; unknown position blocks normal motion; recovery explicit | S + L/P, drive disconnected |
| T04 SPI coexistence (M06/19) | Counter/MRAM drivers, CS bias/config | Alternate transactions, reset either peripheral, inject timeout/all0/all1 MISO, vary SPI rates/modes | No bus contention; device-specific modes/readback; failures invalidate motion state, not fabricate position | S + L |
| T05 Encoder bandwidth/polarity (M24) | Exact encoder/PPR/maxRPM/cable data, pulse generator, counter driver | Sweep A/B phase/rate/duty/noise; test open/short/stuck channels; compare generated count with raw/clean/counter count | Upward sign correct; no lost/reversed counts at specified margin; faults for invalid/progress-loss cases | S + P |
| T06 Counter wrap/reset/time continuity (M06/07) | Defined count domain and valid-position model | Cross32-bit limits both directions; MCU-only/counter-only reset; reboot during movement; corrupt restored snapshot | Coherent signed count/rollover; uncertain motion produces unknown-position, never invented zero | S + L |
| T07 Safety/limit truth table (M02/11/24) | Approved NO/NC and wiring diagnosis, external chain drawing | Inject each contact/open cable/short-to-GND; both limits; each travel direction; test with MCU removed/stalled | Correct polarity; explicit blocked reasons/direction permissions; independent circuit removes authority without app | S + L/P; external chain final A |
| T08 Service station (M12) | Service state machine and physical controls | Exercise key/hold/up/down combinations, release, both directions, timeout, reset and web/RF calls during service | No motion without all required local conditions; release/fault/timeout stops; remote normal commands rejected/logged | S + L/P; final A |
| T09 EM01 frames/transactions (M10/13) | Parser/arbiter, UART emulator, verified manual | Fragment/concatenate/corrupt frames; inject echo/staleACK/lateACK, dropped replies, alarms, wrong response types | Only complete verified fresh matching responses accepted; bounded failure; ACK not equated to stationary | S + L/P |
| T10 UART electrical/enable timing (M02/03) | Correct GPIO mapping and independent permit, scope | Verify J24 pinout/idle voltage, TX/RX with emulator; OE/run sequencing with drive inhibited |9600 8N1 correct direction/levels; no output contention; monitor/config while inhibit stays safe | L/P |
| T11 Control responsiveness (M13/20) | Scheduling budget, non-motion UART sink, network test host | Slow POST/header clients, stalled downloads, many polls, Wi-Fi loss, flash writes under synthetic motion | Measured worst-case loop/stop latency below approved deadline with margin; independent inhibit works during stalls | S for bounds; L for real RTOS/network |
| T12 Motion edge cases (M07/09/10) | Validity/supervisor fixes | Simulate zero progress, wrong direction, overshoot beyond window, unexpected idle drift, target-current equality, conflicting calls, lost VFD | Deterministic rejection/fault/stop; no runaway continuation or fault reset by STOP; all sources share rules | S first |
| T13 Physical deceleration and calibration (M07/09) | All P0 fixes; proven safety/limits; qualified procedure and travel clearance | Program three floors/top HOME; measure stopping in both directions/load cases; alter speed/decel/scale; interrupt calibration | Safe bounded motion, recorded actual stop distance/provenance, invalidation/recalibration, landing tolerance satisfied | S sequence first; final A (new authorization) |
| T14 MRAM/persistence (M06/19) | Driver/schema/write budget, controllable supply | Save/reload versions; random power cuts during records; corrupt/partial writes; full log; migration/restore failures | Latest complete record or explicit invalidity; sequence/CRC; no flash-style unsafe quad use; endurance/retention budget | S + L |
| T15 RTC (M02/19) | Driver and qualified J12 backup | Remove main power, set invalid time, battery absent/depleted, I²C stuck, MCU reset | Correct timestamp/time-valid marker; bounded bus recovery; no false date or motion on backup | L |
| T16 RF commands/baud (M08/17) | Known remotes, shunt settings, logic analyzer, no drive | Map every button to D0–D4; held-at-boot, repeated/overlapping presses, range/noise/interference, baud mismatch | Exactly floor1/2/3/STOP/light; no phantom startup/release command; RF loss cannot be safety sole path | S + P |
| T17 RF learn/erase/identity (M17/18) | Spare/test decoder or approved test memory; controller transactions | Momentary learn/cancel,17s timeout,10s erase/cancel, reset midoperation,40-slot reuse and backup replay | Hardware MODE_IND/result confirms action; no accidental erase; slot-generation/name association correct | S + P; destructive decoder erase only separately approved |
| T18 Light/aux loading (M23/25) | Correct TVS package/polarity, actual lamp, fuses and source choice | Dummy load then actual lamp inrush/dimming if applicable; both JP40 positions; rapid toggles, two clients, reset | Safe-off boot, no rail collapse/thermal trip, authoritative consistent output state and predictable retries | L/P |
| T19 UI/API negative integration (M14–16) | Mock HTTP server with actual contract; real browser tests | Delay/reorder/drop replies,401/409/500/404, wrong-version JSON,{}, invalid fields; disconnect moving then STOP; reconnect during demo timer | No false online/telemetry/idle/success; controls capability-gated; stale clearly marked; reasons preserved | S/browser, then L |
| T20 Web deployment/update (M04/21) |16MB partitions, asset serving/package process | Offline AP load, gzip MIME/cache/SPA/API404 separation, concurrent clients, firmware/UI versions, interrupted update/rollback | Console truly served by board; API never replaced with HTML; bounded control latency; recoverable compatible update | S + L |
| T21 API/config/arbitration (M13/14/19/20) | Strict parser/auth/schema and supervisor | Invalid numbers/overflow/negative/junk, unknown keys, huge body, JSON vs form, moving settings/reboot, unauthorized origins, restore conflicts | Correct status/reason, no partial commit or unsafe action; credentials excluded from exports; audit source/result | S + L |
| T22 Clean-clone/PCB/library (M22/23) | Destination KiCad10 standard libraries + Git LFS, qualified assets | Clone/copy root; open `hardware/ElevatorLift.kicad_pro`; resolve all footprints/models; rerun ERC/DRC/parity after routing | No machine-specific dependencies; no unresolved required parts or unreviewed DRC; correct component geometry | S/desktop, unpowered board/mechanical check |
| T23 Installation qualification (M11/22/25) | Qualified reviewer; approved mains/earth/EMI/enclosure/braking procedures; all earlier gates | Independently inspect/test AC tap/disconnect, fuses, separation, PE, cable routing, independent stops/fault recovery and loaded travel | Signed installation acceptance and recorded measured behavior; no reliance on UI/RF as safety chain | A, outside this review's authorization |

## Recommended next integration milestone

First produce a **non-motion controller integration baseline**: approved N16R8 pin map, isolated logic power, serial communication available while drive permit remains off, real counter/MRAM/RTC access, unknown-position boot, truthful versioned status API, and UI with no automatic demo fallback. Prove it with synthetic inputs/UART before connecting a real drive. Then implement normal motion, top-HOME/calibration, RF and service arbitration behind the same supervisor, and only proceed to assembled-system testing after independent safety and PCB release gates are closed.
