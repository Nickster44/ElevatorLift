# Pin reassignment, communications separation, and four-layer preparation

Work performed September 12–13, 2026, using Konnect MCP for every KiCad change.
Evidence filenames retain the September 12 session date. No mains, drive,
firmware upload, physical motion, tracks, vias, or copper zones were introduced.

## Outcome and boundary

The requested pin reassignment and VFD communications separation are implemented
and agree between the exported schematic netlist and live PCB pads. HW-01 and
HW-02 are **closed as schematic/connectivity defects**, not as firmware or bench
qualification gates. The PCB has four enabled copper layers and synchronization
ends in a no-op, without conflicts or pending changes.

**Preparation remains incomplete; this is not a layout-ready or fabrication-ready
release.** All-layer geometric isolation/antenna keepouts, detailed conditional
rules, fabricator dielectric stackup, some placement concerns, library warnings,
and prototype qualification remain open. The installed Konnect tools cannot
author keepout areas or arbitrary conditional rules: `add_zone` creates/refills
copper, which was explicitly prohibited, and `set_layer_constraints` only sets
layer-wide width/clearance minima. No substitute pour or misleading global
3.2 mm rule was created. `.konnect/project.json` contains routing guidance, not
machine-enforced KiCad custom rules.

## Implemented changes

- Confirmed U10 is ESP32-S3-WROOM-1U-N16R8. Module pads 28/29/30 (GPIO35/36/37)
  are marked NC, reserved by octal PSRAM.
- Moved LIMIT_UP to GPIO43/pad 37, LIMIT_DOWN to GPIO44/pad 36, and SERVICE_KEY
  to GPIO3/pad 15. The existing active-low optocoupler conditioning and voltage
  translation choices are retained.
- GPIO43/44 were **not unassigned**: J11 formerly exposed UART0. J11 pads 3/4
  are now NC; its power, ground, EN and BOOT connections and position remain.
  Native USB remains the programming interface, subject to the bench-power gate.
- Added R62, 1 kΩ, standard 0603, between U42C/R51 and LIMIT_UP. GPIO43 can drive
  ROM UART output at startup; R62 limits contention to approximately 3.3 mA when
  the optocoupler is ON. This does not make LIMIT_UP readable during ROM output.
  Firmware must disable UART0 console use, configure the input, and qualify all
  inputs before motion permission. No new downloaded library is required.
- Removed optional AUX components R59/R60/Q41/F41/D42/J47, their wires and labels.
  No AUX_CTRL or AUX power/switch net remains in the exported netlist.
- Renamed VFD_ENABLE to VFD_COMMS_ENABLE. Its exact endpoints are U10.35,
  U24.8 and R24.1; R24.2 remains GND. Removed R25/U25/J25/TP26 and RUN collector
  nets. U24 remains TXS0104E with 3.3 V/5 V supplies and its original UART mapping.
- Updated schematic annotations to remove the obsolete RUN-output description.
- Corrected the misleading USB-only power annotation and the IO heading that
  implied galvanic field isolation; separated the new MCU notes from its value.
- Matched TP21/TP22/TP24/TP25/TP40 schematic footprint IDs to the existing PCB
  1 mm test pads, preserving the user's board geometry.

Removing the MCU RUN circuit does not certify the external safety system. The
hardwired safety chain is outside this board and was not changed. VFD configuration
must independently demonstrate that serial RUN commands cannot bypass it.

## Exact connectivity

These are module pad numbers, not GPIO numbers. Full U10 mapping and 227 passing
assertions are in `2026-09-13-pin-comms-fourlayer/connectivity-checks.json`.

| Function/net | U10 GPIO | Module pad | Other endpoints / behavior |
| --- | --- | --- | --- |
| LIMIT_UP | 43 | 37 | R62.2; R62.1 joins U42.12 and R51.1; R51.2 = 3V3 |
| LIMIT_DOWN | 44 | 36 | U42.10, R52.1; R52.2 = 3V3 |
| SERVICE_KEY | 3 | 15 | U43.16, R53.1; R53.2 = 3V3 |
| VFD_COMMS_ENABLE | 42 | 35 | U24.8 OE and R24.1 only; 10k pulldown to GND |
| VFD_TX_3V3 | 17 | 10 | U24.2 A1 → U24.13 B1 → J24.03, VFD RX |
| VFD_RX_3V3 | 18 | 11 | U24.3 A2 ← U24.12 B2 ← J24.04, VFD TX |
| SAFETY_MON | 1 | 39 | U42.16 and R49 pull-up, unchanged |
| HOME | 2 | 38 | U42.14 and R50 pull-up, unchanged |
| HOLD_TO_RUN | 38 | 31 | U43.14, unchanged |
| SERVICE_UP | 39 | 32 | U43.12, unchanged |
| SERVICE_DOWN | 40 | 33 | U43.10, unchanged |
| LIGHT_CTRL | 41 | 34 | Existing light output, unchanged |
| RF_D0/D1/D2/D3/D4 | 4/5/6/7/8 | 4/5/6/7/12 | Existing RF decoder paths |
| RF_TX_ID / RF_MODE_IND / RF_LEARN | 15/16/14 | 8/9/22 | Existing RF paths |
| MRAM_CS_N / COUNTER_CS_N | 9/10 | 17/18 | Existing SPI chip selects |
| SPI_MOSI / SPI_SCK / SPI_MISO | 11/12/13 | 19/20/21 | Existing SPI bus |
| I2C_SDA / I2C_SCL | 47/48 | 24/25 | Existing RTC bus |
| USB_D− / USB_D+ | 19/20 | 13/14 | Existing native USB |
| STATUS_LED | 21 | 23 | Existing LED |
| ESP_BOOT / ESP_EN | 0/EN | 27/3 | Existing boot/reset circuitry |
| Reserved PSRAM | 35/36/37 | 28/29/30 | NC, no external nets |
| Unused GPIO | 45/46 | 26/16 | NC, unchanged |
| Supply / ground | — | 2 / 1,40,41_1…41_9 | 3V3 / GND |

J24 order is **01 +5V, 02 GND, 03 VFD RX, 04 VFD TX**. J11 is now
1 = 3V3, 2 = GND, 3/4 = NC, 5 = EN, 6 = BOOT. Do not attach a UART programmer
expecting the old J11 pinout. Connector numbering follows the actual exported
symbols, including J24's zero-padded pad names.

### GPIO3 reset and polarity assumptions

R53's existing 10 kΩ pull-up establishes HIGH with the service key open or with
the 12 V optocoupler supply absent. A closed, powered key pulls the collector
LOW. GPIO3 affects JTAG selection, not GPIO0/GPIO46 boot-mode selection. With
default JTAG eFuses, GPIO3's strap is ignored for JTAG selection; both key states
are valid. **Do not enable EFUSE_STRAP_JTAG_SEL or force pad JTAG**: GPIO39–42
also serve controller functions. Neither eFuse programming nor bench confirmation
was performed. A key changing during strap capture still requires reset testing.

Field input convention remains contact-to-GND closed = optocoupler ON = MCU LOW.
Contact open = HIGH. This is an electrical convention, not proof that a broken
wire is safe for every NO/NC switch arrangement. Optocoupler field and logic
circuits share board GND; they are not a separate galvanic safety barrier.

Reference: [Espressif module datasheet v1.8, sections 3 and 4](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).

## Synchronization and mechanical preservation

Initial dry run refused five test-point footprint-ID differences and stale
multi-unit identities for U31/U42. The test-point IDs were corrected in the
schematic. U31/U42 were recreated through the IPC update and restored to their
original position, orientation and side; all their pad coordinates were compared
against the baseline and agree within 0.001 mm. U31 remains B.Cu, U42 F.Cu.
This can normalize library artwork/fields; it does not constitute a blanket
footprint-library requalification.

The reviewed apply revision was
`c185d5d2c7eb2d21bc5a465b621276de5a76fc630dbb007315a484155a332c53`:
3 additions (U31/U42 relinks plus R62), 5 footprint updates, 12 existing pad-net
reassignments, zero conflicts. Ten obsolete AUX/RUN footprints were explicitly
removed through IPC because the synchronization tool preserves board-only parts.
They remain recoverable from Git's previous version and the before snapshots.

All surviving pre-existing footprint positions, rotations and sides match the
baseline except the deliberate local C23/C26 power-loop improvement below.
R62 is newly staged on B.Cu at (142.0,109.5) mm, rotation 0°; its trial
courtyard and silkscreen overlaps were corrected before final DRC.

- Outline unchanged: x 97.26–187.26, y 52.29325–142.29325 mm (90 × 90 mm).
- H1/H2: (99.26,54.29325)/(185.26,54.29325); H4/H3 at the same X values and
  y 140.29325. Hole spacing 86 mm, provisional NPTH diameter 2.6 mm.
- AC connector J20, F20, RV20 and RAC10 PS20 remain in their existing positions.
  All surviving edge connectors, USB J10, VFD J24, service J43/J44, SMA J40 and
  ESP32 U10 were preserved. Removal of J25/J47 is intentional.
- The reported 12 mm underside and roughly 40 mm upper clearance are unverified
  enclosure measurements, not a defined 12 mm-radius keepout. Screw envelopes,
  cable bend space and connector accessibility still need physical checking.
- USB J10 is inset from the board edge, and its cable path is not established by
  a footprint-only DRC. Several edge connector silkscreen warnings also remain.

## Layers, rules and remaining placement gates

| Layer | Enabled type | Intended use |
| --- | --- | --- |
| F.Cu | signal | Components, short critical signals and power loops |
| In1.Cu | power | Continuous SELV GND reference; exclude the mains island/corridor |
| In2.Cu | mixed | SELV power distribution and constrained signals with a suitable reference |
| B.Cu | signal | Existing underside components and remaining signals |

No plane has been poured. In1 continuity is a routing requirement, not an existing
plane. B.Cu USB routing would require a deliberately continuous GND reference on
adjacent In2; do not assume power islands are an adequate reference. Prefer a
same-layer USB pair with a continuous nearby GND reference and no stubs/splits.

The board retains nominal thickness 1.6 mm; no detailed dielectric/copper stackup
was authored by the available tool. Select the fabricator stackup before fixing
USB differential impedance. Existing project defaults are differential width
0.20 mm, gap 0.25 mm, via gap 0.25 mm; USB_DIFF single-track default is 0.25 mm.
These inherited numbers are **provisional, not a 90-ohm impedance claim**.

MAINS class clearance remains 3.2 mm, width 1.0 mm; SELV_POWER width 0.8 mm,
clearance 0.2 mm; USB_DIFF clearance 0.2 mm. All three fully qualified mains nets
are matched. Netclass clearance is not restricted to F.Cu, but it is not proof
of a cross-layer no-overlap isolation corridor. No existing keepout-zone object
or custom `.kicad_dru` was found. The future all-layer exclusion must cover pads,
tracks, vias and zones, and must be verified before routing/pouring. Do not use
a lower local zone clearance to evade the mains class.

Further gates, deliberately not hidden by unchanged placement:

- Define all-layer mains/SELV geometric exclusion and underside screw/metal
  envelopes using qualified mechanical dimensions. The 3.2 mm project rule is
  not itself a certification of creepage/clearance suitability.
- USB must avoid mains, SW nodes, VFD wiring and RF; route together without
  branches over uninterrupted GND. No numerical separation beyond existing
  netclass minima was invented or claimed to be enforced.
- U10 is the external-antenna **1U** variant. Do not apply the PCB-antenna module's
  keepout blindly. Obtain the chosen antenna/cable installation keepout. J40's
  RF feed to U40.16 has approximately 8.8 mm straight-line pad separation; keep
  it short, referenced and free of branches, not stripped of required RF ground.
- UART, SPI, I²C and safety-monitor routes need continuous SELV ground reference
  and separation from high-current loops. U24 currently lies on the underside
  near U40's RF area; check the actual route/reference arrangement.
- The initial C23-to-VIN distance was about 8.0 mm and its GND-pad distance about
  6.8 mm. C23 was moved to (104.8,73.0), rotation 0°, and C26 to (108.2,70.5),
  rotation −90°, both F.Cu. This reduces C23's VIN/GND distances to about
  3.4/2.4 mm and C26's BST distance from 4.1 to about 2.1 mm. Trial courtyard
  and reference-field overlaps were corrected; final DRC has no added placement
  errors. U20 and L20 remain fixed (SW-to-L20 about 4.7 mm). Keep these loops
  compact when routing and verify regulator startup/ripple under load.
- D41 still specifies SMBJ15A with a D_SMA footprint; resolve that package
  mismatch before ordering. Removal of D42 does not solve D41.

## Verification evidence

| Check | Before | Final |
| --- | --- | --- |
| ERC | 0 errors, 31 warnings | 0 errors, same 31 warning descriptions |
| PCB unrouted findings | 343 | 325 (expected, no routing performed) |
| Other PCB errors | 0 | 0 |
| PCB warnings | 191 | 181 |
| Schematic parity findings | 0 | 0 |
| Konnect synchronization | Initial conflicts described above | No-op, zero conflicts/changes |
| Footprints / nets | 135 / 144 | 126 / 140 |
| Routed track segments | 0 | 0 |

Final warnings: 105 library-footprint mismatch, 14 library-link issues,
10 silkscreen-edge, 26 silkscreen-over-copper, 26 silkscreen-overlap.
R62 adds one library-copy mismatch warning; the other remaining warning
signatures were already present. No waiver was added. Source serialization
changed substantially during KiCad save; placement/net evidence, not diff line
count, was used to check preservation.
`git diff --check` also reports four Konnect-generated whitespace-only lines in
schematic serialization. These were not hand-edited; ERC and KiCad exports parse
the files successfully. This is a formatting diagnostic, not an electrical waiver.

The read-only `hardware/tools/verify-pin-comms-handoff.mjs` checks fresh exported
nets against captured live pads: 227 assertions pass. The old netlist fails at
the expected U10.37 reassignment check. This is simulated/static verification,
not powered-board evidence. Full ERC/DRC JSON and the exact sync plan/apply
records are retained in the sibling evidence directory; limited console output
does not replace those full reports.

## Firmware handoff and prototype tests

Status update (2026-09-14): the firmware migration requested below was subsequently
adopted by inhibited contract v2. Follow the
[current dependency handoff](../hardware-dependency-handoff.md) for remaining
work; retain the original migration description as evidence of this hardware revision.

Firmware changes are mandatory and were not made here. Update/version the
interface contract, netlist evidence and PinMap generator; adopt GPIO43/44/3,
remove AUX initialization, disable UART0 console, and rename GPIO42 to
VFD_COMMS_ENABLE. Communication availability must be independent of motion
permission so STOP/readback can operate while safety is open. Preserve fail-closed
motion arbitration and active-low input handling; never interpret a sent STOP
as a confirmed physical stop. Keep the current upload/inhibit guard until qualified.

| Test | Prerequisites / procedure | Required result |
| --- | --- | --- |
| Software contract | Host simulation; regenerate pins and run contract/motion tests with open/closed/disconnected/conflicting inputs | No use of GPIO35–37, no GPIO3 output or UART0 conflict; unknown/fault inputs cannot permit motion |
| Service reset | Powered logic board only, mains/drive isolated; key open/closed, 3V3-before-12V and reverse sequencing, reset/watchdog/brownout | Defined input levels, reliable normal/USB boot, no unintended JTAG/outputs; record actual eFuses without burning them |
| GPIO43 boot | Powered logic plus simulated limit contacts; scope both sides of R62 during ROM boot and input initialization | Contention limited, correct input thresholds afterward, no motion permission until readings qualify |
| UART independence | Logic board and isolated peripheral/emulator, no drive energy; open safety, enable OE, exchange STOP/status, then lose communications | Valid 3V3/5V levels, STOP/readback still possible, communications failure inhibits motion; OE LOW on reset |
| External RUN authority | Qualified bench procedure with drive energy isolated; review VFD configuration and exercise serial RUN with hardwired inhibit open | Independent inhibit cannot be bypassed by MCU/web/RF/serial; no physical motion test authorized here |
| Layout gates | Selected fab stackup, antenna/cable drawings, enclosure and screws; implement geometric rules through suitable Konnect tools | Verified all-layer isolation, reference continuity, actual connector access and compact switching loops before user routing |

No additional user pin-choice is needed. Missing tool support, fabricator stackup,
antenna/mechanical data and physical tests prevent complete routing preparation.

## Changed artifacts

KiCad: `IO.kicad_sch`, `MCU_Storage_RevA.kicad_sch`,
`VFD_Interface_RevA.kicad_sch`, `Power_RevA.kicad_sch` (test-point IDs only),
and `ElevatorLift.kicad_pcb`. The `.kicad_pro` was not edited.
Project routing guidance: `hardware/.konnect/project.json`.
Documentation: root/hardware READMEs, hardware-dependency handoff, architecture
notes, this report and exported evidence. Utilities: read-only connectivity
checker and optional PNG-return support in `konnect-call.ps1`.
No commit or push was performed.
