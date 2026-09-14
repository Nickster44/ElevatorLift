# Rev-A schematic review summary

Review date: 2026-09-04

Historical snapshot. Use the [hardware README](../README.md) and
[current dependency handoff](../../docs/hardware-dependency-handoff.md) for
current implementation and unresolved gates; the counts below describe this date only.

## Automated results

- Hierarchy: 6 reachable sheets, no missing child files, and no sheet-pin mismatches.
- Coverage: 229 symbol instances, 122 named nets, and 0 unresolved symbols.
- KiCad ERC: 0 errors and 31 warnings.
- Connection audit: 0 findings.
- Shorted-net scan: 0 shorts on every child sheet.
- Orphan scan: 0 orphaned wires, labels, junctions, or no-connect markers.
- Decoupling audit: 0 findings after adding the main 5 V ideal-diode input capacitor and encoder-field 12 V decoupling.
- Project-local `SnapEDA` and `ElevatorLift_Custom` libraries resolve through
  `${KIPRJMOD}`; the imported library package is now stored in the repository.

## Remaining library/layout gates

- `CONSMA002` on J40 uses the project footprint derived from the TE drawing; a
  matching 3D model remains optional and its mechanics should be checked during
  enclosure review.
- `ED300/3`, `PM004MNIATR`, `LS7366R-S`, and `RV-3028-C7` manufacturer assets remain requested for independent mechanical/pin-number verification. Valid provisional footprints are assigned where possible.
- The off-grid ERC warnings on U11/U32 and nearby power symbols arise from
  temporary datasheet-derived/imported pin geometry. Connectivity analysis finds
  no orphan or single-pin nets, but the final library qualification should still
  replace or correct those assets before release.
- The ERC library-link warnings are a KiCad CLI/configuration diagnostic in the
  automated invocation; Konnect independently resolves the same project-local
  libraries and representative assets successfully. Confirm once more by opening
  the project on the destination KiCad installation.
- The repeated local/global rail-label warnings are intentional capture style and do not represent separate or shorted rails.
- Per-sheet bulk-capacitance warnings are a limitation of the sheet-local design audit. The shared rails have bulk capacitance on the Power sheet and local ceramic decoupling at each load block.

## Bench and safety gates before PCB release

- Verify the VFD header order and idle/active voltages on a bench harness before connecting the lift.
- Verify encoder voltage, polarity, pulse rate, and optocoupler switching margin with the 2.2 kOhm field resistors.
- Verify each field input's dry-contact assumptions and normally-open/normally-closed behavior.
- Measure light current and inrush; use the external lamp-supply input unless a larger isolated AC/DC module is qualified.
- Complete mains tap, protective-earth/chassis, fuse, creepage/clearance, enclosure, and hardwired safety-chain review.

The schematic is ready for component-library replacement and bench-verification planning. It is not ready for manufacturing or installation on the lift.
