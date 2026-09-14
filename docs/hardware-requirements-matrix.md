# Hardware Requirements Matrix

This matrix converts the project goals into schematic-facing requirements. `TBD` items must be answered before the first PCB revision is finalized.

## Core Control

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MCU-001 | Main controller | Run motion state machine, WebUI, VFD protocol, storage, RF input, diagnostics | `ESP32-S3-WROOM-1U-N16R8` | Wi-Fi AP/station, external antenna, 16 MB flash, 8 MB PSRAM, enough SPI/UART/I2C | Confirm external antenna cable and enclosure feedthrough |
| MCU-002 | Debug/programming | Firmware upload and serial debug | Vertical `USB4145-03-0230-C_REVA2` USB-C to ESP32-S3 native USB, plus J11 boot/reset header (pins 3/4 NC) | Accessible in enclosure, USB ESD protected | Confirm physical service access |
| MCU-003 | Watchdog/reset | Put outputs into safe state on firmware lockup/reset | ESP32 watchdog plus external supervisor if needed | Hardware outputs must default safe | Decide if external supervisor is required |

## Power

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| PWR-001 | Mains input | Tap available 120 VAC leg inside VFD housing | Fused AC input section with screw terminal or blade terminal option | Creepage, clearance, surge, service safety | Confirm tap point, disconnect behavior, and enclosure grounding |
| PWR-002 | Isolated DC | Provide 12 V low-voltage control power | RECOM RAC10-12SK/277 baseline plus jumper-selectable external 12 V lighting input | 12 V, 10 W, 840 mA; previous lighting operated from this supply | Confirm Rev-A load margin and use the external lighting feed if required |
| PWR-003 | 5 V interface rail | Provide a regulated 5 V rail for VFD translation; RF/decoder use 3.3 V | Selected `R-78E5.0-0.5` from the isolated 12 V rail, isolated from USB VBUS with `LM66100DCKR` ideal-diode paths | VFD load budget, noise, startup order | Confirm 5 V current budget and source-transition behavior |
| PWR-004 | 3.3 V logic rail | Regulate 3.3 V for MCU and logic from 12 V | AP63203WU-7 buck | Wi-Fi current peaks, VFD noise, thermal margin | Finalize current budget and layout filtering |
| PWR-005 | Backup time | Preserve logs/state across power loss | MRAM handles write endurance; optional RTC backup | No motion on backup power | Decide RTC battery/supercap |

## Position And Motion Feedback

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| ENC-001 | Encoder input | Read quadrature position reliably | SKF encoder A/B into LS7366R | Open-collector Hall outputs pull low; cable noise | Confirm exact encoder model and maximum pulse rate |
| ENC-002 | Input conditioning | Protect and isolate/clean A/B before counter | Captured: 12 V, 2.2 kΩ LED resistors, TLP291-4, 74HC14D into LS7366R-S | Legacy 270 Ω values are not fitted; verify current and switching margin | Verify actual encoder phase/rate and captured conditioning |
| ENC-003 | Counter IC | Offload quadrature counting | LS7366R SPI counter | SPI speed, count width, reset/index behavior | Confirm package/source availability |
| ENC-004 | Home/limits | Validate absolute position | Home switch plus upper/lower final limits | Safety chain independent of MCU | Confirm switch voltage and wiring |

## VFD Interface

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| VFD-001 | Serial control | Send EM01 run/stop/monitor/get/set commands through the established four-pin interface | Field-matched `TXS0104E` 3.3 V-to-5 V translation stage with OE default-low, test pads, and optional protection | 9600 baud standard UART framing; assembled-board connector and fault behavior still require verification | Bench-test the header, levels, and fault behavior before lift connection |
| VFD-002 | Hardware stop/enable | Remove motion authority independently of serial | External hardwired chain; no MCU RUN driver; GPIO42 controls UART OE only | Installed chain and brake behavior unverified | Close HW-11 external-box evidence gates |
| VFD-003 | Parameter access | WebUI read/write of all documented parameters | Firmware metadata table and range checks | Stopped-only writes, read-back verify | Validate parameter 13 ambiguity on real drive |
| VFD-004 | Stop calibration dependency | Track VFD deceleration parameter used during measured stop calibration | Calibration metadata in MRAM | Deceleration changes invalidate stop offset | Confirm exact VFD deceleration parameter behavior on real drive |

## Nonvolatile Storage

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MEM-001 | Critical state | Store position, state, settings, logs with frequent writes | Siproin PM004MNIATR SPI/QPI MRAM | 4 Mbit baseline, SPI sharing | Confirm final package/stock at JLC order time |
| MEM-002 | Web assets | Store WebUI files and OTA images | N16R8 module flash/LittleFS | Avoid using MRAM for large static assets | No separate Rev-A QSPI device |
| MEM-003 | Long logs | Store extended downloadable history | Module flash / bounded MRAM logs; no external QSPI or SD fitted | Retention and capacity must be measured | Revisit additional storage only in a later revision |
| MEM-004 | Timekeeping | Timestamp logs without cloud dependency | RV-3028-C7 RTC | Battery/supercap, I2C bus | Decide RTC backup source |
| MEM-005 | Stop calibration | Store measured deceleration travel distance and settings used for calibration | MRAM record with sequence and CRC | Must survive power loss and detect stale calibration | Direction-specific offsets are required; target implementation and physical calibration remain pending |

## Connectivity And RF

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| NET-001 | Local service AP | Always allow local access when station unavailable | ESP32-S3 softAP | Metal enclosure attenuates antenna | External antenna/mechanical path |
| NET-002 | Station Wi-Fi | Optional local network connection | ESP32-S3 station mode | Must fall back to AP on failure | WebUI credential UX |
| RF-001 | Legacy remotes | Support five-button remotes for Floor 1, Floor 2, Floor 3, Stop, and Light toggle | Linx RXM-418-LR receiver plus LICAL-DEC-MS001 decoder | Mandatory rev-A feature; all five decoded outputs must reach MCU | Confirm exact legacy connector and voltage levels |
| RF-002 | Pairing | Start/cancel Learn Mode and erase all learned addresses from WebUI | MCU output to a dedicated wired `LEARN` pad/node, MCU input from `MODE_IND`, and local button/service pads | `LEARN` is not exposed by the legacy header; safe default is required | Confirm decoder timing, polarity, and physical-button wiring on bench |
| RF-003 | Remote identity | Associate observed transmitter with nickname and log command source | Decoder `TX_ID` routed to MCU; MRAM WebUI registry keyed by learned slot and association epoch | Decoder memory cannot be enumerated or selectively deleted; ID/profile reconciliation must tolerate unseen transmitters | Confirm `TX_ID` electrical format and capture timing from decoder datasheet/bench test |
| NET-003 | Home automation | Expose local automation path without cloud dependency | Local REST API plus optional MQTT/Home Assistant integration | Motion commands must remain authenticated, logged, and subject to prechecks | Decide whether to add MQTT discovery or keep REST-only in rev A |

## Inputs And Outputs

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| IN-001 | User buttons | Read call/stop/service buttons | J43/J44 optocoupler-conditioned contact-to-GND inputs | Shared ground, active LOW; not galvanic isolation | Confirm external contact topology and field compatibility |
| IN-002 | Safety loop monitor | Monitor safety status in firmware | Protected input only; hardwired chain handles authority | Must not be sole safety path | Confirm safety circuit voltage |
| IN-003 | Restricted service recovery | Permit diagnosed monitored-channel recovery only with cabinet-local authorization and continuous hold-to-run | Keyed/service input plus hold-to-run input, guarded firmware state | Must not bypass E-stop, hardwired final limits, VFD/watchdog authority removal, or permit normal-speed/remote motion; automatic timeout and logging required | Safety review and exact field procedure required before implementation |
| OUT-001 | Lift light | Control current lift lighting | Protected 12 V MOSFET switch with jumper-selectable onboard or external 12 V source | Estimated 12 V, about 750 mA; external feed supports higher load or reduced onboard margin | Confirm load type, inrush, wiring return, and selected supply source |
| OUT-002 | Interlocks/solenoids | Omit unless requirement returns | Not fitted; optional AUX removed | Safety review required | Confirm no interlock outputs required |
| OUT-003 | Status indicator | Local board status indication | LED or service header | Useful during bring-up | Decide visible indicator location |

## Mechanical And Environmental

| ID | Function | Required behavior | Captured implementation / requirement | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MECH-001 | Enclosure fit | Fit inside VFD housing | 90 mm x 90 mm PCB, 86 mm corner-hole centers, estimated 40 mm height and 12 mm underside standoff clearance | Mains separation, antenna path, connector access | Confirm hole diameter, screw size, connector/wire-bend keepouts, and height with calipers |
| EMI-001 | VFD noise immunity | Avoid false inputs/resets near VFD | Filtering, TVS, layout separation, grounding plan | Motor leads and mains are noisy | Define cable shield/earth strategy |
| CONN-001 | Serviceability | Connect existing field wiring cleanly | ED300/3 field terminals, Phoenix 1717732 mains terminal, selected headers | Mains/SELV separation; provisional mechanical assets | Verify footprint, mating/access and wiring |
