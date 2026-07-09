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
| PWR-001 | Mains input | Tap available 120 VAC leg inside VFD housing | Fused AC input section with screw terminal or blade terminal option | Creepage, clearance, surge, service safety | Confirm tap point, disconnect behavior, and enclosure grounding |
| PWR-002 | Isolated DC | Provide 12 V low-voltage control power | RECOM RAC10-12SK/277 baseline | 12 V, 10 W, 840 mA; light load is about 750 mA | Confirm final 12 V load budget and whether any solenoid output remains |
| PWR-003 | Logic rail | Regulate 3.3 V for MCU and logic from 12 V | AP63203/AP63200 class buck | Wi-Fi current peaks, VFD noise, thermal margin | Finalize current budget and layout filtering |
| PWR-004 | Backup time | Preserve logs/state across power loss | MRAM handles write endurance; optional RTC backup | No motion on backup power | Decide RTC battery/supercap |

## Position And Motion Feedback

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| ENC-001 | Encoder input | Read quadrature position reliably | SKF encoder A/B into LS7366R | Open-collector Hall outputs pull low; cable noise | Confirm exact encoder model and maximum pulse rate |
| ENC-002 | Input conditioning | Protect and clean A/B before counter | 5 V pullups, series resistance, TVS/ESD, RC option, Schmitt cleanup | SKF datasheet recommends 270 ohm pullups at 5 V; design must handle about 18.5 mA sink current per low channel | Decide whether counter/input conditioning runs at 5 V with SPI level shifting or uses a translated 3.3 V logic path |
| ENC-003 | Counter IC | Offload quadrature counting | LS7366R SPI counter | SPI speed, count width, reset/index behavior | Confirm package/source availability |
| ENC-004 | Home/limits | Validate absolute position | Home switch plus upper/lower final limits | Safety chain independent of MCU | Confirm switch voltage and wiring |

## VFD Interface

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| VFD-001 | Serial control | Send EM01 run/stop/monitor/get/set commands into VFD opto-isolated serial input | UART transistor/opto-drive interface with current set for the VFD input LED; protected receive path | 9600 baud standard UART framing; direct MCU drive previously did not source/sink enough opto input current | Determine required VFD input current from manual/bench test and choose driver resistor values |
| VFD-002 | Hardware stop/enable | Remove motion authority independent of serial if available | VFD enable/stop terminal driver | Must fail safe on reset/watchdog | Confirm VFD terminal functions |
| VFD-003 | Parameter access | WebUI read/write of all documented parameters | Firmware metadata table and range checks | Stopped-only writes, read-back verify | Validate parameter 13 ambiguity on real drive |

## Nonvolatile Storage

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MEM-001 | Critical state | Store position, state, settings, logs with frequent writes | Siproin PM004MNIATR SPI/QPI MRAM | 4 Mbit baseline, SPI sharing | Confirm final package/stock at JLC order time |
| MEM-002 | Web assets | Store richer WebUI files if needed | MCU flash/LittleFS or W25Q128JV QSPI NOR | Avoid using MRAM for large static assets | Decide optional QSPI footprint |
| MEM-003 | Long logs | Store extended downloadable history | Optional QSPI flash or microSD | Removable media reliability if SD | Decide if SD footprint is worth board space |
| MEM-004 | Timekeeping | Timestamp logs without cloud dependency | RV-3028-C7 RTC | Battery/supercap, I2C bus | Decide RTC backup source |

## Connectivity And RF

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| NET-001 | Local service AP | Always allow local access when station unavailable | ESP32-S3 softAP | Metal enclosure attenuates antenna | External antenna/mechanical path |
| NET-002 | Station Wi-Fi | Optional local network connection | ESP32-S3 station mode | Must fall back to AP on failure | WebUI credential UX |
| RF-001 | Legacy remotes | Support existing remotes | Linx RXM-418-LR or compatible receiver | Mandatory rev-A feature; decode/validate remote data | Confirm exact receiver wiring/protocol |
| RF-002 | Pairing | Pair/remap remotes from WebUI | Firmware remote registry in MRAM | RF commands are not safety signals | Define remote action model |
| NET-003 | Home automation | Expose local automation path without cloud dependency | Local REST API plus optional MQTT/Home Assistant integration | Motion commands must remain authenticated, logged, and subject to prechecks | Decide whether to add MQTT discovery or keep REST-only in rev A |

## Inputs And Outputs

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| IN-001 | User buttons | Read call/stop/service buttons | Protected GPIO or isolated input receiver | Voltage level TBD | Confirm field wiring voltage |
| IN-002 | Safety loop monitor | Monitor safety status in firmware | Protected input only; hardwired chain handles authority | Must not be sole safety path | Confirm safety circuit voltage |
| OUT-001 | Lift light | Control current lift lighting | 12 V MOSFET low-side or high-side switch with fuse/current protection | Estimated 12 V, about 750 mA | Confirm LED/incandescent load type, inrush, and wiring return |
| OUT-002 | Interlocks/solenoids | Omit unless requirement returns | External relay/SSR header if needed | Safety review required | Confirm no interlock outputs required |
| OUT-003 | Status indicator | Local board status indication | LED or service header | Useful during bring-up | Decide visible indicator location |

## Mechanical And Environmental

| ID | Function | Required behavior | Candidate implementation | Key constraints | Open decisions |
| --- | --- | --- | --- | --- | --- |
| MECH-001 | Enclosure fit | Fit inside VFD housing if practical | Target about 3.5 in x 3.5 in PCB with mounting holes near corners | Mains separation, antenna path, connector access | Confirm exact mounting hole spacing and keepout after enclosure measurement |
| EMI-001 | VFD noise immunity | Avoid false inputs/resets near VFD | Filtering, TVS, layout separation, grounding plan | Motor leads and mains are noisy | Define cable shield/earth strategy |
| CONN-001 | Serviceability | Connect existing field wiring cleanly | Pluggable terminal blocks and keyed headers | Mains/SELV separation | Define connector map |
