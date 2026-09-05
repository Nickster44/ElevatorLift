# Rev-A Schematic Capture Status

This checklist records how the 2026-08-30 field observations were converted into
the current Rev-A schematic. The capture is complete enough for ERC review and
bench planning, but it is not a production BOM or safety approval.

## Confirmed Design Inputs

- Board envelope: 90 mm x 90 mm; corner mounting-hole centers 86 mm apart.
- Local rails required by the observed interfaces: isolated 12 V, regulated 5 V,
  and regulated 3.3 V.
- VFD serial reference: `TSW-104-07-G-S` four-pin connector ordered 5 V, GND,
  RX, TX and `TXS0104E` 3.3 V-to-5 V translation, pending bench verification.
- Encoder reference: 12 V open-collector A/B into `TLP291-4` isolation. Rev A
  uses 2.2 kOhm LED resistors rather than the observed legacy 270 ohm values;
  switching margin remains a bench gate.
- Legacy RF support: RXM-418-LR plus `LICAL-DEC-MS001`; `LEARN` needs a separate
  wire/pad because it is not carried by the existing header.
- Monitored inputs required: safety loop and HOME switch. Neither replaces the
  independent hardwired safety chain.
- Service controls: J43 is SERVICE KEY, HOLD-TO-RUN, GND and J44 is SERVICE UP,
  SERVICE DOWN, GND. Each field signal is asserted by a dry contact to GND and
  sensed through the U43 `TLP291-4` input bank. The intended hardware is a
  maintained key switch plus momentary hold and direction controls. This is not
  a safety-rated enabling circuit.

## Rev-A Capture Register

| Block | Reference part or interface | Sourcing plan | Capture status |
| --- | --- | --- | --- |
| Mains to 12 V | RECOM `RAC10-12SK/277` | Exact imported part with fused input, MOV, bulk capacitance and test point. Phoenix `1717732` 7.62 mm-pitch L/N/PE terminal is selected provisionally so the PCB can maintain the mains clearance rule. Check final 12 V load budget before approval. | Captured; safety and connector-footprint review gate |
| 12 V to 5 V | RECOM `R-78E5.0-0.5` | Exact imported part, 0.5 A rating. | Captured |
| 12 V to 3.3 V | `AP63203WU-7` 2 A buck | Standard KiCad symbol/footprint with selected 2.2 uH inductor and required capacitors. | Captured |
| 5 V source sharing | Two `LM66100DCKR` ideal diodes | One isolates the onboard R-78E source and one isolates USB VBUS. Either source can run the logic rail without backfeeding the other. | Captured |
| MCU | `ESP32-S3-WROOM-1U-N16R8` | Exact external-antenna module; its 16 MB flash and 8 MB PSRAM eliminate a separate Rev-A QSPI asset-flash device. | Captured |
| Persistent storage | `PM004MNIATR` 4 Mbit MRAM | Datasheet-derived project symbol; downloaded library asset still requested for independent verification. | Captured; library verification gate |
| VFD level translation | TI `TXS0104ED` | OE defaults low; direct UART nets and test pads are included. | Captured; bench gate |
| Encoder isolation | Toshiba `TLP291-4` | Two channels used for A/B; unused channels explicitly marked no-connect. | Captured; bench gate |
| Encoder counter | `LS7366R-S` | Datasheet-derived project symbol, SPI connection and local 4 MHz clock. | Captured; library verification gate |
| RF receiver/decoder | Linx `RXM-418-LR` and `LICAL-DEC-MS001` | Exact imported parts; harvested modules acceptable for prototype use. | Captured |
| RF LEARN | Diode-OR local service switch and MCU control | Pull-down gives a defined inactive state at reset. | Captured |
| Safety, HOME, limits and service inputs | 12 V optically isolated monitor inputs | Eight field inputs are consolidated on the IO page. Safety remains monitor-only. | Captured; field-voltage gate |
| Light and auxiliary outputs | Protected 12 V low-side MOSFET outputs | Gate pull-downs, fuses and TVS devices included; light rail can be externally supplied. | Captured; load gate |

## Completed Capture Sequence

1. Root hierarchy and connector assumptions established.
2. Isolated mains power and all local rails captured.
3. MCU, USB, MRAM, RTC and recovery interfaces captured.
4. VFD serial and isolated run/enable interface captured.
5. Isolated encoder front end and LS7366R counter captured.
6. RF, safety/home/limit/service inputs and protected outputs consolidated onto
   a single IO page.
7. Complete schematic hierarchy transferred to the 90 mm x 90 mm PCB, including
   86 mm mounting-hole centers and preliminary mains, SELV-power and USB net classes.

## Portability And Review Checkpoint — 2026-09-04

- The imported SnapEDA package is now stored under `hardware/SnapEDA-Library`,
  including all supplied STEP files. Both library tables use `${KIPRJMOD}` and
  no longer depend on `D:\Downloads`.
- Konnect successfully resolved representative imported ESP32/USB symbols,
  imported USB/RAC10 footprints, and the corrected project ESP32 footprint from
  the repository-owned libraries.
- Full schematic coverage: six reachable files, 229 symbol instances, 122 named
  nets, and zero unresolved symbols.
- Connectivity checks: zero shorted nets, zero orphan items and zero single-pin
  nets.
- ERC: zero errors and 31 warnings. The warnings are retained for review; they
  include KiCad CLI library-link diagnostics despite successful Konnect
  resolution, off-grid imported/custom pins, duplicate local/global label names,
  and MRAM power-flag pin-type notices.
- PCB coverage: 135 footprints, 496 pads and 144 named nets. The board is not
  routed; DRC therefore reports 343 unconnected items and is not a release pass.

## Explicit No-Go Gates

- No mains PCB layout or manufacturing release without AC tap, protective-earth,
  fuse, creepage/clearance, and enclosure review.
- No VFD connection to the lift without bench-verified header pinout, logic
  levels, and fault-safe behavior.
- No encoder/counter final circuit without measured encoder current and maximum
  pulse rate.
- No safety-loop recovery feature without an independently reviewed hardwired
  safety architecture.
- No claim that J43/J44 provide a safety-rated enabling or bypass circuit. A
  required personnel-protection enabling function needs redundant, independently
  reviewed safety hardware.
