# PCB Layout Readiness — 2026-09-04

Historical snapshot, not current layout guidance. Current counts, pin changes and
open routing gates are in the [hardware README](../README.md) and
[September 13 handoff](../../docs/reviews/2026-09-13-pin-comms-fourlayer.md).

## Completed

- Six-sheet hierarchy transferred to the board: 131 schematic footprints plus four mounting holes, 496 pads and 144 named nets.
- Board outline set to 90 mm x 90 mm. Provisional 2.6 mm mounting holes retain the measured 86 mm center-to-center spacing.
- Net classes applied to the actual hierarchical net names:
  - `MAINS`: 1.0 mm default track, 3.2 mm clearance.
  - `SELV_POWER`: 0.8 mm default track, 0.2 mm clearance.
  - `USB_DIFF`: 0.25 mm default track, 0.2 mm clearance.
- Corrected project footprints created for the ESP32-S3 module, CONSMA002 RF connector, zero-padded Samtec VFD header and provisional 7.62 mm mains terminal.
- ERC baseline: 0 errors and 31 warnings. Warnings are KiCad CLI library-link diagnostics, off-grid custom/imported symbol endpoints, duplicate local/global-label names and MRAM power-flag pin-type notices.

## Current DRC Interpretation

Functional placement is preliminary and no copper has been routed. The complete
2026-09-04 review reached all 135 footprints and reports 343 unconnected items,
191 other design-rule violations and a `NOT READY` verdict. Most findings are
expected at this phase, but every error must be resolved or explicitly reviewed
after placement and routing; this is not a manufacturing score or waiver.

The automated cluster and force-directed placements were evaluated and rejected: common power/ground nets collapsed the functional blocks into overlapping clusters and did not preserve the required mains/SELV partition.

## Required Layout Sequence

1. Lock the four mounting holes and define 12 mm underside fastener keepouts.
2. Place the mains terminal, fuse, MOV and RAC10 module as an isolated mains island; preserve the 3.2 mm rule and consider slots where needed.
3. Place edge connectors according to enclosure cable access, then reserve the ESP32 U.FL cable path and the separate 418 MHz CONSMA002 edge location.
4. Place the 12 V, 5 V and 3.3 V converters and their local capacitors/inductor.
5. Place ESP32, USB protection/connector, MRAM and RTC; keep USB D+/D- short and matched.
6. Place VFD, encoder and IO/RF blocks; keep field-side optocoupler copper separated from logic-side copper.
7. Route mains and power first, then USB, clocks/SPI, field signals and remaining logic. Add ground zones only after isolation boundaries are established.
8. Run iterative DRC, schematic/PCB parity review and a final mechanical/3D inspection.

## Release Gates

- Verify mounting-hole diameter and the 12 mm underside keepout against the existing hardware.
- Verify Phoenix `1717732` body/drill geometry from the downloaded manufacturer asset.
- Confirm AC tap, PE/chassis handling, fuse coordination, enclosure insulation and applicable safety standard.
- Bench-confirm VFD pinout/levels and encoder current/pulse-rate assumptions.
- Independently verify the `PM004MNIATR`, `LS7366R-S` and `ED300/3` library assets.
