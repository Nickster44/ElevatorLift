# Hardware / KiCad Workspace

This folder is the hardware design workspace. The KiCad 10 project now contains a Rev-A schematic capture and an unrouted four-layer PCB for review. It is not yet approved for PCB layout or manufacturing.

## Files

| File | Purpose |
| --- | --- |
| `ElevatorLift.kicad_pro` | KiCad project |
| `ElevatorLift.kicad_sch` | Root hierarchical schematic |
| `Power_RevA.kicad_sch` | Fused AC input and isolated 12 V, 5 V and 3.3 V rails |
| `MCU_Storage_RevA.kicad_sch` | ESP32-S3, USB, MRAM, RTC and service interfaces |
| `VFD_Interface_RevA.kicad_sch` | 3.3 V/5 V UART translation; communications-only enable, no MCU RUN output |
| `Encoder_Counter_RevA.kicad_sch` | Isolated encoder inputs and LS7366R counter |
| `IO.kicad_sch` | Linx RF, monitored field inputs and light output; optional AUX removed |
| `ElevatorLift.kicad_pcb` | PCB work-in-progress: 90 mm x 90 mm outline, 86 mm corner-hole centers, and the complete schematic footprint/net transfer |
| `ElevatorLift_Custom.kicad_sym` / `ElevatorLift_Custom.pretty` | Project-owned custom symbols and corrected/provisional footprints |
| `SnapEDA-Library` | Repository-owned copy of imported symbols, footprints and supplied STEP files |
| `sym-lib-table` / `fp-lib-table` | Project library registrations using portable `${KIPRJMOD}` paths |
| `architecture-blocks.md` | Schematic page/block plan |

## Current Schematic Pages

1. Root hierarchy and design notes.
2. Power (`Power_RevA`).
3. MCU and storage (`MCU_Storage_RevA`).
4. VFD interface (`VFD_Interface_RevA`).
5. Encoder counter (`Encoder_Counter_RevA`).
6. Consolidated RF, field inputs and outputs (`IO`).

The superseded standalone RF, safety, output and connector child sheets were
removed after their contents were consolidated into the active Rev-A hierarchy.

## Rev-A Capture Assumptions

Use [the hardware requirements matrix](../docs/hardware-requirements-matrix.md) and the main README physical verification checklist as the source of truth. The current schematic assumptions are:

- Target PCB outline: 90 mm x 90 mm; corner mounting-hole centers 86 mm apart. The reported 12 mm underside and approximately 40 mm upper clearances are enclosure estimates, not defined keepout radii. Verify screw-head/standoff envelopes and the estimated 2.56 mm hole diameter before layout release.
- Main supply: fused 120 VAC + neutral to exact RECOM `RAC10-12SK/277`; protected 12 V distribution uses `MF-MSMF075/24X`; 5 V uses an `R-78E5.0-0.5`; and 3.3 V uses an AP63203 stage with Bourns `SRP7028A-2R2M`.
- 5 V source sharing: separate `LM66100DCKR` ideal diodes isolate the onboard 5 V converter and USB VBUS before they join the 5 V rail. USB VBUS alone does not power the separate 3.3 V regulator; an approved bench-power procedure remains necessary.
- VFD serial: the `TXS0104E` circuit matches the field design, with the `TSW-104-07-G-S` header ordered 5 V, GND, RX, TX. Assembled-board pinout and bench behavior remain release gates.
- Encoder: 12 V field inputs use 2.2 kOhm LED resistors into `TLP291-4` isolation. This intentionally replaces the observed legacy 270 ohm values to avoid excessive optocoupler current; verify reliable switching on the actual encoder.
- RF: RXM-418-LR and `LICAL-DEC-MS001` support is mandatory; include a separate wired `LEARN` interface plus a local service button/pads.
- Light output: protected 12 V MOSFET output with an onboard/external supply selector. The previous 750 mA lighting operated from the 10 W RAC10 supply; verify Rev-A load margin, and select the external 12 V lamp input if controller demand or upgraded lighting exceeds the onboard budget.
- Service station: J43 accepts dry-contact SERVICE KEY and HOLD-TO-RUN inputs with a shared GND; J44 accepts dry-contact SERVICE UP and SERVICE DOWN inputs with a shared GND. Use a maintained key switch and momentary controls. These are optocoupler-conditioned MCU inputs sharing controller GND, not an independent galvanic barrier or a safety-rated enabling circuit.
- Current MCU assignment: LIMIT_UP GPIO43/module pin 37 through 1k R62; LIMIT_DOWN GPIO44/pin 36; SERVICE_KEY GPIO3/pin 15. GPIO35/36/37 remain NC for octal PSRAM. J11 UART pins 3/4 are now NC; use native USB. Preserve active-low inputs and default JTAG eFuses; see the reset/boot details in the current handoff.
- GPIO42 is VFD_COMMS_ENABLE and connects only U24 OE plus R24's 10k pulldown. R25/U25/J25/TP26 and AUX R59/R60/Q41/F41/D42/J47 are removed. The separate external hardwired safety loop remains the required RUN authority, subject to VFD configuration verification.

## PCB Preparation Status

- The current board contains 126 footprints (including four mounting holes) and 140 nets. The 90 x 90 mm outline runs from `(97.26,52.29325)` to `(187.26,142.29325)` mm. The four provisional 2.6 mm NPTH holes retain 86 mm spacing and a 2 mm offset from each adjacent edge.
- Net classes retain 3.2 mm mains clearance and 0.8 mm SELV power track width (0.2 mm clearance), plus the provisional USB differential-pair settings.
- The downloaded ESP32 module footprint contained invalid 0.2 mm plated holes. `ElevatorLift_Custom:ESP32-S3-WROOM-1U-N16R8_CorrectedV2` replaces it with the correct perimeter geometry and nine symbol-compatible exposed-pad segments.
- The provisional mains connector is now Phoenix Contact `1717732`, 7.62 mm pitch. The former 5 mm-pitch part could not satisfy the selected 3.2 mm mains clearance rule.
- Existing footprint positions and sides are preserved except for a local C23/C26 move that shortens U20's input/bootstrap loops; new R62 was also placed. Four layers are enabled: F.Cu, In1.Cu (SELV GND reference), In2.Cu (power/constrained signals), B.Cu. No tracks or zones were created. The nominal 1.6 mm board thickness is not a selected fabricator stackup.
- Current verification: ERC 0 errors/31 unchanged warnings; PCB DRC 325 unrouted errors, 181 warnings, no other errors, zero schematic parity findings. The final Konnect synchronization dry run is a no-op. These checks do not make the board layout-ready.
- See [the current handoff](../docs/reviews/2026-09-13-pin-comms-fourlayer.md) for exact pin mapping, prototype gates, placement concerns, all-layer isolation/antenna keepouts still to implement, and the provisional USB rule. Konnect's `.konnect/project.json` records routing guidance, not enforced KiCad custom rules.

Dated review results are evidence, not a live release status. See the
[external-box review](../docs/reviews/2026-09-13-manual-control-box.md) for the
latest field-interface evidence and unresolved independent stopping authority.

## Portable Project Libraries

Open `ElevatorLift.kicad_pro`, not an individual child sheet. Both project
library tables use `${KIPRJMOD}` paths:

- `SnapEDA` resolves to `hardware/SnapEDA-Library`.
- `ElevatorLift_Custom` resolves to the custom symbol file and footprint folder
  beside the project.

The repository-owned SnapEDA directory is a byte-for-byte copy of the imported
download and includes the supplied STEP files. Standard KiCad libraries are
still required, so install KiCad 10 with its normal symbol and footprint
packages on the destination computer.

See [field observations](../docs/field-verification-log.md) and the
[capture register](../docs/schematic-capture-readiness.md) for the confirmed observations, reusable-parts register, and the remaining release gates.

## KiCad Tooling

This project is pinned to KiCad 10.0.4. Use `tools\kicad.ps1` rather than a
generic `kicad-cli` command so automated checks do not accidentally run against
the also-installed KiCad 8 version.

```powershell
# From the repository root
powershell -ExecutionPolicy Bypass -File hardware\tools\kicad.ps1 -Action Version
powershell -ExecutionPolicy Bypass -File hardware\tools\kicad.ps1 -Action Validate
powershell -ExecutionPolicy Bypass -File hardware\tools\kicad.ps1 -Action Gui
```

`Validate` writes KiCad-native ERC and DRC JSON reports to `hardware\reports`
by default. The reports are generated design-review artifacts and should be
regenerated whenever the schematic or board changes.

The helper scripts auto-detect conventional OneDrive/Documents KiCad plugin
locations. On a different layout, set `KONNECT_PLUGIN_ROOT` to the installed
Konnect directory and `KICAD_10_BIN` to the KiCad 10 `bin` directory. Neither
environment variable is required merely to open and edit the project in KiCad.
Use PowerShell 7 for the Konnect batch helper; the basic KiCad launcher also
supports Windows PowerShell 5.1.
