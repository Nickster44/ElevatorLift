# Hardware Requirements Matrix

This matrix converts the project goals into schematic-facing requirements. `TBD` items must be answered before the first PCB revision is finalized.

## Core Control

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MCU-001 | Main controller | Run motion state machine, WebUI, VFD protocol, storage, RF input, diagnostics | ESP32-S3-WROOM-1U | Wi-Fi AP/station, external antenna, enough SPI/UART/I2C | Confirm module variant, flash size, antenna location |
| MCU-002 | Debug/programming | Firmware upload and serial debug | USB-C or USB header to ESP32-S3 native USB/UART | Accessible in enclosure, ESD protected | Decide service connector type |
| MCU-003 | Watchdog/reset | Put outputs into safe state on firmware lockup/reset | ESP32 watchdog plus external supervisor if needed | Hardware outputs must default safe | Decide if external supervisor is required |

## Power

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| PWR-001 | Mains input | Tap available 120 VAC leg inside VFD housing | Fused AC input section | Creepage, clearance, surge, service safety | Confirm tap point and enclosure grounding |
| PWR-002 | Isolated DC | Provide low-voltage control power | Mean Well IRM-05-5 or IRM-05-12 | Load budget, heat, safety certification | Choose 5 V versus 12 V module |
| PWR-003 | Logic rail | Regulate 3.3 V for MCU and logic | TPSM82822/TPSM82823 or AP63203 | Wi-Fi current peaks, noise | Finalize upstream DC rail |
| PWR-004 | Backup time | Preserve logs/state across power loss | MRAM handles write endurance; optional RTC backup | No motion on backup power | Decide RTC battery/supercap |

## Position And Motion Feedback

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| ENC-001 | Encoder input | Read quadrature position reliably | SKF encoder A/B into LS7366R | Open-collector Hall outputs, cable noise | Confirm exact encoder model and supply |
| ENC-002 | Input conditioning | Protect and clean A/B before counter | Pullups, series resistance, TVS/ESD, RC option, SN74LVC2G17 | Avoid false counts from VFD noise | Choose pullup voltage/resistance |
| ENC-003 | Counter IC | Offload quadrature counting | LS7366R SPI counter | SPI speed, count width, reset/index behavior | Confirm package/source availability |
| ENC-004 | Home/limits | Validate absolute position | Home switch plus upper/lower final limits | Safety chain independent of MCU | Confirm switch voltage and wiring |

## VFD Interface

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| VFD-001 | Serial control | Send EM01 run/stop/monitor/get/set commands | Protected UART, optional ISO6721 isolation | 9600 baud, unknown electrical layer | Confirm VFD serial voltage/reference |
| VFD-002 | Hardware stop/enable | Remove motion authority independent of serial if available | VFD enable/stop terminal driver | Must fail safe on reset/watchdog | Confirm VFD terminal functions |
| VFD-003 | Parameter access | WebUI read/write of all documented parameters | Firmware metadata table and range checks | Stopped-only writes, read-back verify | Validate parameter 13 ambiguity on real drive |

## Nonvolatile Storage

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MEM-001 | Critical state | Store position, state, settings, logs with frequent writes | Everspin MR25H40 SPI MRAM | 4 Mbit baseline, SPI sharing | Confirm MRAM size/cost |
| MEM-002 | Web assets | Store richer WebUI files if needed | MCU flash/LittleFS or W25Q128JV QSPI NOR | Avoid using MRAM for large static assets | Decide optional QSPI footprint |
| MEM-003 | Long logs | Store extended downloadable history | Optional QSPI flash or microSD | Removable media reliability if SD | Decide if SD footprint is worth board space |
| MEM-004 | Timekeeping | Timestamp logs without cloud dependency | RV-3028-C7 RTC | Battery/supercap, I2C bus | Decide RTC backup source |

## Connectivity And RF

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| NET-001 | Local service AP | Always allow local access when station unavailable | ESP32-S3 softAP | Metal enclosure attenuates antenna | External antenna/mechanical path |
| NET-002 | Station Wi-Fi | Optional local network connection | ESP32-S3 station mode | Must fall back to AP on failure | WebUI credential UX |
| RF-001 | Legacy remotes | Support existing remotes if retained | Linx RXM-418-LR or compatible receiver | Decode/validate remote data | Confirm exact receiver wiring/protocol |
| RF-002 | Pairing | Pair/remap remotes from WebUI | Firmware remote registry in MRAM | RF commands are not safety signals | Define remote action model |

## Inputs And Outputs

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| IN-001 | User buttons | Read call/stop/service buttons | Protected GPIO or isolated input receiver | Voltage level TBD | Confirm field wiring voltage |
| IN-002 | Safety loop monitor | Monitor safety status in firmware | Protected input only; hardwired chain handles authority | Must not be sole safety path | Confirm safety circuit voltage |
| OUT-001 | Lift light | Control current lift lighting | Compact relay, SSR, or optotriac/triac | Load voltage/current/type TBD | Measure light load |
| OUT-002 | Interlocks/solenoids | Omit unless requirement returns | External relay/SSR header if needed | Safety review required | Confirm no interlock outputs required |
| OUT-003 | Status indicator | Local board status indication | LED or service header | Useful during bring-up | Decide visible indicator location |

## Mechanical And Environmental

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MECH-001 | Enclosure fit | Fit inside VFD housing if practical | Compact two-layer or four-layer PCB | Mains separation and antenna path | Measure available space |
| EMI-001 | VFD noise immunity | Avoid false inputs/resets near VFD | Filtering, TVS, layout separation, grounding plan | Motor leads and mains are noisy | Define cable shield/earth strategy |
| CONN-001 | Serviceability | Connect existing field wiring cleanly | Pluggable terminal blocks and keyed headers | Mains/SELV separation | Define connector map |

