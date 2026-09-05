# Hardware / KiCad Workspace

This folder is the hardware design workspace. The KiCad 10 project now contains a complete Rev-A schematic capture for review and bench verification. It is not yet approved for PCB layout or manufacturing.

## Files

| File | Purpose |
| --- | --- |
| `ElevatorLift.kicad_pro` | KiCad project |
| `ElevatorLift.kicad_sch` | Root hierarchical schematic |
| `Power_RevA.kicad_sch` | Fused AC input and isolated 12 V, 5 V and 3.3 V rails |
| `MCU_Storage_RevA.kicad_sch` | ESP32-S3, USB, MRAM, RTC and service interfaces |
| `VFD_Interface_RevA.kicad_sch` | 3.3 V/5 V UART translation and isolated run/enable output |
| `Encoder_Counter_RevA.kicad_sch` | Isolated encoder inputs and LS7366R counter |
| `IO.kicad_sch` | Linx RF, monitored field inputs, light output and auxiliary output |
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

Use `docs/hardware-requirements-matrix.md` and the main README physical verification checklist as the source of truth. The current schematic assumptions are:

- Target PCB outline: 90 mm x 90 mm; corner mounting-hole centers 86 mm apart. Keep 12 mm underside screw/standoff keepouts; verify the estimated 2.56 mm hole diameter before layout release.
- Main supply: fused 120 VAC + neutral to exact RECOM `RAC10-12SK/277`; protected 12 V distribution uses `MF-MSMF075/24X`; 5 V uses an `R-78E5.0-0.5`; and 3.3 V uses an AP63203 stage with Bourns `SRP7028A-2R2M`.
- 5 V source sharing: separate `LM66100DCKR` ideal diodes isolate the onboard 5 V converter and USB VBUS before they join the logic rail.
- VFD serial: `TXS0104E` translation and `TSW-104-07-G-S` header ordered 5 V, GND, RX, TX. Pinout and bench behavior remain release gates.
- Encoder: 12 V field inputs use 2.2 kOhm LED resistors into `TLP291-4` isolation. This intentionally replaces the observed legacy 270 ohm values to avoid excessive optocoupler current; verify reliable switching on the actual encoder.
- RF: RXM-418-LR and `LICAL-DEC-MS001` support is mandatory; include a separate wired `LEARN` interface plus a local service button/pads.
- Light output: protected 12 V MOSFET output with an onboard/external supply selector. The 10 W RAC10 supply is too small for a 750 mA lamp plus the controller, so use the external lamp input or qualify a larger isolated supply.
- Service station: J43 accepts dry-contact SERVICE KEY and HOLD-TO-RUN inputs with a shared GND; J44 accepts dry-contact SERVICE UP and SERVICE DOWN inputs with a shared GND. Use a maintained key switch and momentary controls. These are optically isolated MCU inputs, not a safety-rated enabling circuit.

## PCB Preparation Status

- The current board contains 135 footprints (131 schematic parts plus four mounting holes) and 144 named nets. Four provisional 2.6 mm NPTH mounting holes are placed at `(2,2)`, `(88,2)`, `(88,88)`, and `(2,88)` mm.
- Net classes are active for 3.2 mm-clearance mains routing, 0.8 mm SELV power routing, and the USB differential pair.
- The downloaded ESP32 module footprint contained invalid 0.2 mm plated holes. `ElevatorLift_Custom:ESP32-S3-WROOM-1U-N16R8_CorrectedV2` replaces it with the correct perimeter geometry and nine symbol-compatible exposed-pad segments.
- The provisional mains connector is now Phoenix Contact `1717732`, 7.62 mm pitch. The former 5 mm-pitch part could not satisfy the selected 3.2 mm mains clearance rule.
- Footprints remain in an unrouted staging area. Functional placement, mains/SELV partitioning, copper zones, routing and a clean DRC are the next PCB phase; the current DRC is intentionally not a manufacturing-release result.

The 2026-09-04 automated review reached all six sheets and found no shorted
nets, orphan items, single-pin nets or unresolved schematic symbols. ERC has
zero errors and 31 warnings, primarily library-link diagnostics from the CLI,
off-grid imported/custom pins, duplicate local/global labels and MRAM power-flag
pin-type notices. The PCB review reports 343 unrouted connections and therefore
correctly returns NOT READY. See `reports/schematic-review-summary.md` and
`reports/pcb-layout-readiness.md`.

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

See `docs/field-verification-log.md` and `docs/schematic-capture-readiness.md` for the confirmed observations, reusable-parts register, and the remaining release gates.

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
