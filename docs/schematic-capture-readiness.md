# Rev-A Capture Register and Release Gates

Updated 2026-09-14. Schematics and a four-layer, unrouted PCB exist. Capture is
not physical qualification, a production BOM, or a safety approval.

## Captured blocks

| Block | Selected part / implementation | Remaining gate |
| --- | --- | --- |
| AC input | Phoenix 1717732 provisional L/N/PE terminal; fuse, MOV; RAC10-12SK/277 | AC tap, enclosure, earth, isolation geometry, footprint and load review |
| 5 V | R-78E5.0-0.5; LM66100 source sharing with USB VBUS | Backfeed/startup measurements; USB alone does not power 3.3 V |
| 3.3 V | AP63203WU-7 from 12 V; SRP7028A-2R2M | Load, ripple, thermal and compact-loop routing |
| MCU/USB | ESP32-S3-WROOM-1U-N16R8; USB4145-03-0230-C_REVA2 at J10 | Corrected footprint, access, power procedure and reset qualification |
| MRAM | PM004MNIATR, project-local symbol | Package/pin verification and power-interruption tests |
| RTC | RV-3028-C7 | Backup source and package verification |
| VFD UART | TXS0104ED; J24 5V/GND/RX/TX; OE on GPIO42 only | Assembled levels, drive configuration and independent stopping authority |
| Encoder | TLP291-4, 2.2 kΩ LED resistors, 74HC14D, LS7366R-S and 4 MHz clock | Actual encoder phase/current/rate and counter qualification |
| RF | RXM-418-LR, LICAL-DEC-MS001, wired/local LEARN, CONSMA002 | Jumper configuration, physical mapping, timing and D40 package |
| Field inputs | U42/U43 optocoupler conditioning; shared GND; LOW on contact closure | NO/NC and broken-wire behavior; external-box safety architecture |
| Light | Protected low-side MOSFET; selectable external 12 V feed | Load/inrush margin and D41 SMBJ15A versus SMA footprint mismatch |

AUX and the MCU RUN collector have been removed. J11 pins 3/4 are NC.
LIMIT_UP/LIMIT_DOWN/SERVICE_KEY use GPIO43/44/3; GPIO35-37 are reserved for
octal PSRAM. Service terminals J43/J44 are monitored commands, not independent
safety-rated enabling contacts.

## Current evidence and portability

The root plus five child sheets are listed in the [hardware README](../hardware/README.md).
Libraries are repository-owned and registered with `${KIPRJMOD}`; copy the
whole hardware directory, not just the project file. Standard KiCad 10 libraries
are also required. Imported assets still need package and mechanical verification.

The [September 13 hardware report](reviews/2026-09-13-pin-comms-fourlayer.md)
records four copper layers, 126 footprints, 140 nets, zero tracks/zones, a no-op
synchronization and preserved mechanics. The subsequent
[external-box review](reviews/2026-09-13-manual-control-box.md) refreshed the
netlist: 227 connectivity assertions passed, ERC had zero errors at error
severity, and DRC reported 325 unrouted errors plus 181 warnings.
These are dated checks, not a live readiness status. The earlier warning-inclusive
ERC recorded 31 warnings; none was waived.

## Release gates

- Resolve all-layer mains/SELV exclusion, mechanical envelopes, antenna/cable
  constraints and the fabricator stackup before routing release. USB geometry
  is provisional, not verified controlled impedance.
- Resolve component-library/package issues and verify assembled power and signals
  before connection to the lift.
- Establish independent E-stop/final-limit/drive/brake authority, including
  override and failure behavior, before any operational profile.
- Keep firmware motion/upload inhibits until the
  [hardware dependency handoff](hardware-dependency-handoff.md) and supervised
  qualification procedure are satisfied.

Historical capture counts and completed planning steps are no longer repeated
here. Dated reviews retain the evidence and limitations.
