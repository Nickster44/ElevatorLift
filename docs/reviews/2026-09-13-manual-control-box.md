# External manual-control box: hardware evidence review

Reviewed 2026-09-13. **Operational cargo profile not qualified.** This is an
evidence review, not permission to bypass a protection or operate the lift.
No KiCad design, firmware, interface contract, wiring, or safety protection was
changed. No equipment was energized, firmware uploaded, or physical motion requested.

## Finding and evidence boundary

September 14 documentation note: this is a dated evidence snapshot. Subsequent
software work adopted the owner-specified D0-D4 actions (Floor 1, Light, Floor 3,
Floor 2, STOP), a configurable diagnostic default of 9600, and target-bound
manual-release STOP. Its earlier instructions to keep baud=null describe the
reviewed revision, not current software. See the
[current handoff](../hardware-dependency-handoff.md). Physical RF mapping,
jumper compatibility and independent stopping/brake authority remain unverified.

The board and contract agree on the monitor and service inputs. However, the
repository evidence inspected does not include an as-wired external-box diagram,
contact schedule, independent stopping-chain trace, installed VFD configuration,
or brake-control circuit. Which contacts the override bypasses, which emergency
and final-limit protections remain effective, and what stops/holds the load are
**unknown**. The proposed override supplying a healthy monitor state is an owner
requirement, not a verified external circuit.

The board no longer contains the MCU RUN collector. That proves separation of
UART OE from the removed collector, **not** that serial RUN cannot bypass an
external stopping circuit. Neither cargo-only terminology nor a healthy GPIO
changes this evidence gap.

Evidence used:

- Fresh Konnect export: [current.net](2026-09-13-manual-control-box/current.net).
  Exact component/pin references below refer to this export, not an assumed cable.
- [Hardware change report](2026-09-13-pin-comms-fourlayer.md), its captured PCB
  pad evidence, and [firmware adoption report](2026-09-13-firmware-handoff.md).
- `firmware/interface-contract.json` v2, `firmware/include/PinMap.h`,
  `firmware/src/main.cpp:610`, `core/FieldInputs.h`, and `core/Supervisor.cpp:239`.
- `docs/new-controller-requirements.md`, Restricted Service Recovery;
  `docs/field-verification-log.md`, Safety/reference wiring;
  `README.md:230`, unanswered independent-safety-device inventory.
- `docs/vfd-serial-protocol.md` describes protocol capabilities, not actual
  drive parameter readback. `_old_resources/old_code.ino.txt:204` supports the
  historical LOW-healthy convention, but is not a wiring diagram for this box.
- Searches of repository documentation and resource filenames found no external
  box wiring evidence. Unprovided owner photos, cables, and installed equipment
  cannot be inspected through the project files.

## Verified controller-side connections

Every listed field input is a contact-to-board-GND input, not a 3.3 V external
logic input. The source path is +12 V through R41-R48 (2.2 kΩ), an optocoupler
LED, and the field contact to GND. R49-R56 (10 kΩ) pull the collectors to +3V3.
Thus sufficient LED current makes the MCU input LOW. Open field wiring makes it
HIGH when the 3.3 V supply and pull-up are intact. Nominal LED current is about
5 mA, calculated rather than measured; thresholds/current margin remain bench
items. Field and MCU returns share GND: these optocouplers do not establish an
independent galvanic safety barrier. Do not inject external 12/24 V into these
terminals based merely on the name of the safety signal.

| Input | Field terminal | LED cathode / collector | Pull-up | U10 module pad / GPIO |
| --- | --- | --- | --- | --- |
| SAFETY_MON | J41.1 | U42.2 / .16 | R49 | 39 / 1 |
| HOME | J41.2 | U42.4 / .14 | R50 | 38 / 2 |
| LIMIT_UP | J42.1 | U42.6 / .12 | R51 | 37 / 43, through R62 1 kΩ |
| LIMIT_DOWN | J42.2 | U42.8 / .10 | R52 | 36 / 44 |
| SERVICE_KEY | J43.1 | U43.2 / .16 | R53 | 15 / 3 |
| HOLD_TO_RUN | J43.2 | U43.4 / .14 | R54 | 31 / 38 |
| SERVICE_UP | J44.1 | U43.6 / .12 | R55 | 32 / 39 |
| SERVICE_DOWN | J44.2 | U43.8 / .10 | R56 | 33 / 40 |

J41/J42/J43/J44 pin 3 is GND. GPIO35/36/37 remain unavailable on the
ESP32-S3-WROOM-1U-N16R8. GPIO3 reset/eFuse qualification and GPIO43 ROM UART
contention remain the bench gates described in the hardware change report.

## Mode and input truth tables

L/H are measured-at-MCU logic levels, not field-terminal voltage. X means
unknown/unqualified. Rows assume a healthy 3.3 V rail/pull-up and enough field
current where stated. None is a physical motion-permission certification.

| Electrical condition | MCU input | Meaning / ambiguity |
| --- | --- | --- |
| Powered field contact closed to GND | L | Asserted; SAFETY_MON is interpreted healthy |
| Contact open | H | Deasserted; SAFETY_MON interpreted non-healthy |
| Broken signal or return conductor | H, absent another current path | Indistinguishable from an open contact |
| +12 V absent, +3V3 intact | H | Indistinguishable from open inputs; no dedicated field-power proof |
| Signal shorted to GND | L with field power | Can imitate healthy safety, active key, or pressed button |
| MCU supply missing/reset/boot or input circuitry faulty | X | Do not infer a valid software state or a physical stop |

| Proposed physical mode/event | SAFETY_MON | KEY / HOLD / UP / DOWN | What can be concluded |
| --- | --- | --- | --- |
| Ordinary healthy loop | L by proposed interface | Actual wiring unknown; expected inactive contacts would read H | Healthy monitor alone does not prove ordinary mode |
| Keyed override intentionally enabled | L by proposed interface | KEY=L only if a separate key contact is actually wired/closed at J43.1 | Same safety bit as ordinary healthy operation |
| Healthy monitor plus service up request | L | L / L / L / H, conditional on contact wiring | Core service-request pattern, not proof of external override or permission |
| Healthy monitor plus service down request | L | L / L / H / L, conditional | Same limitation |
| Key released | May remain L for an ordinary healthy loop | KEY=H if its contact opens | Core service stop request; hardwired effect unknown |
| Hold released | Not established by board | HOLD=H if its contact opens | Core service stop request; hardwired effect unknown |
| Both directions pressed | Not established by board | UP=L and DOWN=L | Core rejects service start / requests service stop; external arbitration unknown |
| Safety loop opens without an asserted override | H if monitor contact opens | Other inputs depend on wiring | Software non-healthy, but independent stop mechanism unverified |
| Override remains enabled while a bypassed contact opens | L is possible under proposal | No bypass identity or feedback input exists | Cannot discover which protection was bypassed from these bits |

Existing independent SERVICE_KEY input could identify a key position **only after**
an auxiliary contact, its mechanical relationship to the override, and its truth
table are documented. It does not prove that bypass contacts actually transferred
or reveal which devices remain effective. A stuck/shorted key input also appears
active. Do not derive override mode from SAFETY_MON, hold, or direction alone.

### NO/NC consequences that must not be conflated

- An NC healthy safety-monitor contact opening on fault is consistent with
  LOW=healthy and open-wire=non-healthy, provided there is no parallel bypass.
- A NO limit contact closing at travel end matches current LOW=limit-active
  software, but a broken wire resembles a clear limit.
- An NC limit contact opening at travel end gives the reverse mechanical meaning
  from current LOW=limit-active software. It cannot be silently adopted as compatible.
- Key/hold/direction NO closures would match current active-low assertions, but
  the installed contact arrangements are not evidenced. Home is likewise unresolved.

Retain the current polarity and qualification inhibit. Resolve the field truth
table before proposing a software inversion or hardware change.

## Independent stopping authority: established versus missing

```text
ESTABLISHED controller paths
J41-J44 contacts -> U42/U43 conditioning -> MCU GPIO -> software decisions
MCU UART17/18 <-> U24 translator <-> J24 <-> external VFD serial port
GPIO42 -> U24.8 OE + R24 pulldown ONLY (communications, not RUN permission)

UNVERIFIED installation path -- not an as-built circuit
E-stop / final limits / other protections / key and hold contacts
    -> [unknown bypass topology and relay/contact arrangement]
    -> [unknown independent VFD inhibit or energy-interruption mechanism]
    -> motor torque removal / stopping / brake application / load holding
```

R25/U25/J25/TP26 are absent. J24 is 01=5V, 02=GND, 03=VFD RX,
04=VFD TX; no hardwired RUN signal is present there. No trace in this board's
export puts J41-J44 contacts into an independent drive/brake interruption chain.
The required separate installation circuit could exist; it is not proven here.
Do not assume an EM01 terminal is STO, that loss of torque applies a brake, or
that a brake is spring-applied merely because a brake is mentioned in requirements.

| Failure/event | Software evidence | Independent physical outcome still needed |
| --- | --- | --- |
| Key/hold release; direction release/conflict | `Supervisor.cpp:286` and :296 request stop in service; release-to-arm at :225; service timeout 5 s | Contact interruption path and stopping/holding action without MCU |
| Broken safety wire | Normally HIGH/non-healthy; :243 faults when hardware is qualified | Does the break also interrupt independent authority? Parallel paths? |
| Broken limit/home wire | HIGH can resemble inactive/clear under current convention | NO/NC schedule, separate final limit, continuity diagnosis |
| Loss of field power | Collectors tend HIGH if 3V3 survives | Relay dropout, brake supply/default, restart behavior; no inference from GPIO |
| MCU crash/reset/unpowered | Code cannot guarantee STOP during a crash; OE pulldown disables UART on reset when effective | Hardware response with MCU absent; retained VFD command and brake behavior |
| UART loss | Core faults on communication loss; diagnostics cannot establish physical stop | Installed timeout/action, independent stop path, brake timing, restart latch |
| Serial RUN while external stop asserted | Present diagnostics profile does not send RUN | Drive configuration/topology must make RUN ineffective independently of software |
| Welded/bypassed contact or short-to-GND monitor | May still read healthy | Independent detection/protection and reset conditions, not debounce |

`main.cpp:72` leaves FieldContinuityQualified=false; contract deploymentReady is
false. Therefore service motion is presently inhibited even with the hypothetical
valid patterns above. Core stop behavior is host-tested logic, not operational
drive control or a safety-rated stopping function. Safety/home/hold/direction are
sampled directly; the 100 ms stability/25 ms freshness filter in FieldInputs.h
applies to upper/lower/key and must not be described as filtering all eight inputs.

## RF evidence and remaining assembly decisions

Fresh export: JP41.2 -> U41.3 SEL_BAUD0; JP42.2 -> U41.4 SEL_BAUD1.
Both jumpers have pin 1=GND and pin 3=3V3. Each is a three-pin both-open symbol;
no center-net pull resistor or recorded assembly shunt selection establishes a default.

| JP42 BAUD1 shunt | JP41 BAUD0 shunt | Baud |
| --- | --- | --- |
| 1-2 (LOW) | 1-2 (LOW) | 2400 |
| 1-2 (LOW) | 2-3 (HIGH) | 9600 |
| 2-3 (HIGH) | 1-2 (LOW) | 19200 |
| 2-3 (HIGH) | 2-3 (HIGH) | 28800 |

This is the component's selection table, **not a selected build configuration**.
Set before power-up and match the existing transmitter. TX_ID uses the selected
baud; U41.7 LATCH is GND, specifying momentary active-HIGH data outputs, unlike
the optocoupled active-LOW field inputs. Source: Linx-authored
[MS decoder data guide, Figure 9 and Latch/TX ID sections](https://www.mouser.com/datasheet/2/238/lical-dec-ms001-1109236.pdf).
Its safety warning also means this RF link must not be credited as an independent
emergency stopping system. Receiver/transmitter rate compatibility still requires
qualification; listing all decoder rates does not endorse all rates for this RF path.

| Decoder output/pin | MCU GPIO / module pad | Physical button assignment |
| --- | --- | --- |
| D0 / U41.13 | 4 / 4 | Unknown |
| D1 / U41.14 | 5 / 5 | Unknown |
| D2 / U41.17 | 6 / 6 | Unknown |
| D3 / U41.18 | 7 / 7 | Unknown |
| D4 / U41.19 | 8 / 12 | Unknown |

Floor 1/2/3, Stop and Light are intended functions, not an evidenced D0-D4 order.
Legacy Serial1/Serial2 at 9600 (`old_code.ino.txt:71`) does not prove the decoder
straps or remote-button mapping. Keep contract baud=null and
buttonMappingVerified=false. Do not learn/erase remotes to answer this review.

## Owner evidence needed and non-energized verification plan

Obtain the following without operating the lift. Any enclosure access or electrical
measurement requires qualified personnel and the site's energy-control procedure,
including stored electrical/mechanical energy and a secured gravity load. A key,
STOP button, software inhibit, or disconnected MCU is not energy isolation.
See [OSHA energy-control guidance](https://www.osha.gov/etools/lockout-tagout/hot-topics/energy-control-program/energy-control-circuitry-prohibition).
This review does not authorize energization to collect missing data.

| ID | Prerequisite and procedure | Required record / acceptance evidence |
| --- | --- | --- |
| MB-01 topology | Existing drawings first; qualified de-energized trace if absent | Terminal-to-terminal drawing covering every key pole, hold/direction contact, E-stop, gate/latch/safety device, ordinary and final limits, relay coils/contacts, VFD inputs and brake supply. Mark every bypass branch explicitly. Label normal and override current paths. |
| MB-02 identification | Safe access under site isolation procedure | Overall and close-up photos of box front/back, every switch contact block/terminal number and wire marker, both cable ends, relay/safety-controller part numbers, VFD exact model/revision/terminal labels, motor/brake nameplates and any brake rectifier/controller. Photos must show hidden key poles, not just the front legend. |
| MB-03 contact schedule | Isolated and verified de-energized contacts; disconnect from electronics only under a documented qualified procedure, label/restoration controlled | Record continuity/resistance and actual terminal pairs for key normal/override, hold released/held, up/down released/pressed/both, each E-stop and each limit independently actuated without carriage travel. Identify NO/NC/COM and mechanical linkage. Parallel connected circuits can invalidate a simple continuity reading. |
| MB-04 monitor harness | De-energized, identified harness and return | Trace J41-J44 destinations and common. Establish whether outputs are dry contacts or driven signals, rated source voltage/current, and whether key has an independent auxiliary contact. Document simulated open conductor effects by circuit analysis/isolated continuity, not a live wire-break experiment. |
| MB-05 drive and brake authority | Existing parameter backup/manuals and complete MB-01 | Exact terminal function and command-source configuration, serial timeout/action, reset/auto-restart behavior, brake voltage/default state and controlling contacts. For every E-stop/final limit, trace how it stops/holds with override engaged and MCU absent. Manufacturer evidence must resolve serial RUN precedence; no RUN trial in this review. |
| RF-01 straps | Batteries removed; board/remote isolated from all supplies | Photos of decoder and every existing remote board; continuity of SEL_BAUD0/1 to supply/ground with pin-1 orientation recorded; identify transmitter/encoder variants. Record new-board shunt assembly instruction only after confirming compatibility. |
| RF-02 button mapping | Remotes unpowered/batteries removed | Label each physical button and trace its encoder data input using photos/isolated continuity. Relate that data line to decoder D0-D4, including any unconnected D5-D7. If traces cannot be established, leave unknown pending a separately authorized isolated RF bench test with no lift connected. Do not press a powered remote near the installation. |

Non-energized inspection can establish topology and contact states, not dropout
time, braking distance/holding capacity, relay fault response under power, or
serial-command precedence in the actual drive. Those remain separate qualified,
supervised bench/system verification gates after an approved safety architecture.
No powered procedure was run or authorized here.

## Checks performed and handoff

- Konnect fresh root netlist export succeeded; IO short-net scan: 0.
- Konnect root ERC at **error severity**: 0 errors. This does not mean zero warnings;
  the earlier warning-inclusive report recorded 31 warnings, not requalified here.
- Konnect PCB DRC: 325 unconnected items, 181 warnings, zero schematic parity
  findings; total 325 errors. Full [DRC JSON](2026-09-13-manual-control-box/drc.json)
  retained. No new routing or physical integration claimed.
- Existing read-only netlist checker: 227 assertions PASS against fresh export and
  previously captured IPC pads. This is not a fresh live-PCB IPC capture.
- `node firmware/scripts/generate-contract.mjs --check`: PASS.
- `node firmware/tests/contract.mjs`: PASS v2 inhibited contract.
- `firmware/tests/run.ps1`: PASS 2866 assertions, **host simulation only**.
- Three saved schematic file hashes (MCU, VFD, IO) differ from the older hardware
  report's hash manifest. The fresh exported connectivity passes; do not describe
  that old manifest as the current source snapshot. PCB/project hashes match it.

No hardware readiness claim is closed by these software/connectivity checks.
The prior hardware report's statement that firmware updates remain mandatory is
historical: v2 software adoption is now evidenced. Conversely, the contract field
`motionAuthority=external-hardwired-safety-loop-only-no-mcu-run-permission-output`
is an intended architecture plus a verified absent board output, not proof of an
installed independent chain. README's fail-safe stop/brake requirement remains
unfulfilled evidence, not an implemented controller brake output.

Firmware handoff: preserve diagnostic-only v2, deployment/upload/motion inhibits,
active-low field convention, unavailable GPIO35-37, and GPIO42 translator-only
behavior. Do not substitute a healthy monitor bit, cargo-only label, or host-test
result for MB-01 through MB-05. Keep HW-03/HW-05 and new HW-11 open. Any future
operational profile needs an explicitly reviewed mode/authority contract based on
the owner evidence above; do not silently reinterpret SERVICE_KEY as verified
override feedback or invert NC limits. No software changes are requested by this
evidence review alone.
