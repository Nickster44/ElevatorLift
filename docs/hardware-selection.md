# Hardware Selection And Rev-A Decision Record

This document records the selected Rev-A hardware direction and the alternatives that remain useful for qualification. The schematic capture exists; entries marked as alternatives are not fitted unless bench or sourcing work invalidates the current choice. Exact sourcing candidates are tracked in `docs/jlcpcb-lcsc-sourcing.md`, and local datasheet copies are under `datasheets/`.

## Summary Recommendation

Build the first board around:

- Exact ESP32-S3-WROOM-1U-N16R8 module with external antenna connector.
- LS7366R-S SPI quadrature counter.
- PM004MNIATR SPI MRAM for critical state and recent logs.
- Module flash for WebUI assets and OTA; no separate Rev-A bulk-flash IC.
- TXS0104E 3.3 V/5 V VFD UART reference topology, subject to bench qualification.
- 12 V optically isolated encoder input path with 2.2 kOhm LED resistors.
- Local fallback AP Wi-Fi setup flow.
- 120 VAC to isolated DC power module if the board fits inside the VFD housing.
- Compact light output only unless interlocks/solenoids are reintroduced.

## Candidate Parts

| Block | Preferred candidate | Why it fits | Notes |
| --- | --- | --- | --- |
| MCU/Wi-Fi | Espressif `ESP32-S3-WROOM-1U-N16R8` | Wi-Fi AP/station support, 16 MB flash, 8 MB PSRAM, native USB and external antenna connector | Use a U.FL/IPEX pigtail to move the antenna outside the metal enclosure or to a qualified RF window |
| Compact MCU alternate | Espressif ESP32-S3-MINI-1U | Smaller external-antenna ESP32-S3 module | Less flash/PSRAM flexibility depending on variant |
| Quadrature counter | LSI/CSI LS7366R | 32-bit quadrature counter with SPI host interface | Good match for offloading encoder counting from MCU |
| Encoder conditioning | 12 V field pullups/contacts into `TLP291-4`, 2.2 kOhm LED resistors, `74HC14` cleanup | Isolates the observed field interface before the counter | Deliberate deviation from the observed legacy 270 ohm path; measure switching margin and maximum pulse rate |
| Critical nonvolatile memory | Siproin PM004MNIATR | 4 Mbit SPI/QPI MRAM, nonvolatile, high endurance, LCSC/JLC-friendly SOP-8 sourcing path | Preferred over flash/EEPROM for frequent position/log writes |
| MRAM fallback | Everspin MR25H40CDF | 4 Mbit SPI MRAM, 40 MHz, nonvolatile, unlimited endurance, power-loss retention | Technically strong but currently weaker for JLCPCB cost/stock than the Siproin part |
| Lower-cost memory alternate | Infineon/Fujitsu MB85RS4MT | 4 Mbit SPI FRAM with high endurance | FRAM, not MRAM; acceptable fallback if MRAM cost/sourcing is poor |
| Web assets / extended logs | N16R8 module internal flash | Avoids a separate memory IC while providing 16 MB flash and 8 MB PSRAM | W25Q128JV remains a later-revision reference only |
| RTC timestamps | Micro Crystal RV-3028-C7 | Very low power I2C RTC with UNIX time counter and backup support | Useful when AP-only/offline and no NTP is available |
| VFD UART interface | TI `TXS0104E` | Matches the observed 3.3 V-to-5 V legacy topology and four-pin 5 V/GND/RX/TX header | Bench-confirm header order, direction, idle levels and whether the VFD actually requires an opto-current driver |
| Industrial digital inputs | TI ISO1211 / ISO1212 | Isolated digital input receiver family for industrial input modules | Candidate if field inputs are 24 V or higher; exact input voltages still need confirmation |
| RF receiver | TE/Linx RXM-418-LR | Maintains compatibility path with existing 418 MHz remotes | Requires firmware validation/decoding; RF commands are not safety signals |
| RF decoder | Linx LICAL-DEC-MS001 | Existing five-button remote decoding, retained learned-address memory, TX_ID identity output, LEARN control, and MODE_IND status | Route all five data outputs, TX_ID, LEARN, and MODE_IND; supports 40 learned addresses and erase-all rather than individual deletion |
| AC/DC power | RECOM RAC10-12SK/277 | Previously used 12 V, 10 W supply; JLCPCB lists it as an assembly candidate | Good baseline for 12 V light plus logic, but limited margin for future solenoids |
| 3.3 V rail | Diodes AP63203WU-7 or AP63200 adjustable variant | Compact buck regulator options depending on upstream DC rail | AP63203 is a fixed 3.3 V JLC candidate; AP63200 adjustable is a fallback if stock changes |
| 12 V light output | Protected MOSFET switch | Compact path for the believed 12 V, about 750 mA lift light | Confirm load type, inrush, and whether low-side switching is acceptable |

## Architecture Notes

### MCU And Wireless

Use the selected external-antenna `ESP32-S3-WROOM-1U-N16R8` because the board is expected to sit inside the metal VFD enclosure. Route a suitable pigtail to an external or non-metal antenna location. Its 16 MB flash and 8 MB PSRAM cover the Rev-A WebUI/OTA requirement without another bulk-memory IC.

### Position Counting

The likely SKF encoder outputs two 90-degree phase-shifted open-collector square waves. The captured Rev-A counter path is:

```text
12 V encoder A/B -> 2.2 kOhm LED resistors -> TLP291-4 -> 74HC14 cleanup -> LS7366R -> SPI -> MCU
```

The LS7366R gives the MCU a stable count register to read instead of relying on high-rate interrupt service. The schematic should preserve test points for raw A/B, conditioned A/B, and SPI signals.

### MRAM, Flash, And Logs

Use MRAM as the source of truth for:

- Current position snapshots.
- Motion state recovery.
- Floor positions.
- Calibration and stop offsets.
- VFD parameter cache.
- Local run speed and service settings.
- Recent binary event/fault ring buffer.

The preferred first-pass MRAM part is Siproin `PM004MNIATR` because it is a 4 Mbit SPI/QPI MRAM with an LCSC sourcing path and an assembly-friendly SOP-8 package. Everspin `MR25H40CDF` remains a reference/fallback part, but its LCSC stock depth and cost are worse.

Use the N16R8 module's flash for Rev-A WebUI assets, OTA staging and noncritical storage. A removable SD card or external QSPI NOR may be reconsidered in a later revision only if measured capacity or retention requirements justify the extra device.

### VFD Interface

The old controller drove the VFD from `Serial1` at 9600 baud. The observed legacy PCB uses a TI `TXS0104E` between the 3.3 V controller side and a four-pin VFD header, with no discrete transistor or series resistor observed. This is a useful working reference, but the VFD pinout, common/reference, idle levels, and input type have not yet been bench-verified.

Recommended default:

- Start with the observed 3.3 V-to-5 V `TXS0104E` topology, including an OE pull-down, test pads, and a configurable protection/series-resistor footprint.
- Do not add an opto-input current driver unless the bench test proves the VFD interface requires it; preserve space for that option if practical.
- Include series resistors, ESD protection, and a defined return/reference path.
- Keep VFD serial routing away from mains/motor output wiring.
- Add a hardware enable/stop path independent of serial commands if the VFD supports it.

### Inputs

Split inputs into classes:

- Encoder A/B: fast conditioned logic into the quadrature counter.
- Safety loop/final limits: hardwired motion authority plus firmware monitoring.
- User buttons/home/service inputs: protected low-voltage inputs if possible.
- RF receiver/decoder: five command inputs plus TX_ID and MODE_IND inputs, with a safe-default-low MCU output wired to the separate physical `LEARN` node rather than assuming it is available on the legacy header.

The current service station uses dry contacts to the J43/J44 GND returns: a maintained SERVICE key switch, momentary HOLD-TO-RUN, and momentary UP/DOWN controls. These optically isolated inputs are MCU commands, not a certified enabling circuit. If a safety-rated enabling function is required, use a redundant three-position enabling device with an independently reviewed safety relay/controller rather than relying on these GPIO paths.

### Outputs

Do not replicate the old four-relay accessory board by default. Start with:

- One light-control output sized for the actual lift light load.
- Optional low-voltage output header for future external relay/SSR module.
- No general-purpose solenoid/interlock relay bank unless the safety/control architecture requires it.

Gate interlocks should not be treated as casual MCU-controlled outputs. If they return, they need a safety/control review.

### Power

If the control board fits inside the VFD housing, use a PCB-mount isolated AC/DC module fed from a fused 120 VAC leg. Include:

- Fuse or fusible resistor as appropriate.
- MOV/surge protection.
- Common-mode/EMI filtering as needed.
- Creepage/clearance review.
- Protective earth/chassis strategy.
- Separate noisy/high-voltage area from SELV logic.

Start with the RECOM `RAC10-12SK/277` 12 V, 10 W module as the baseline because it was used successfully in the previous accessory board and is listed by JLCPCB. The believed 12 V, about 750 mA lift light consumes most of that supply's continuous current rating, so future solenoids or auxiliary outputs require either a larger supply, a separate auxiliary supply, or an external output module.

## Remaining Qualification Decisions

1. Bench-map the VFD four-pin header and confirm the `TXS0104E` levels, polarity, reference, and fault behavior.
2. Confirm the measured 90 mm x 90 mm board envelope, 86 mm hole centers, hole diameter, and enclosure keepouts.
3. Confirm available 120 VAC tap point and grounding/chassis strategy.
4. Confirm exact encoder model, cable length, 12 V/270 ohm/TLP291-4 implementation, optocoupler outputs, and HC74 function on the old board.
5. Confirm whether existing button/safety/home/limit wiring is low voltage, 24 V, or 120 VAC.
6. Confirm lift light voltage, current, inrush, and load type.
7. Confirm RXM-418-LR receiver wiring, antenna, and remote encoding behavior.
8. Confirm the N16R8 internal flash/PSRAM budget after the production WebUI bundle and OTA partition table are measured; external bulk storage is not fitted in Rev A.
9. Complete functional PCB placement and confirm how much area can be reserved for isolation/protection versus compactness.

## Source References

- Espressif ESP32-S3-WROOM-1 / 1U datasheet: https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
- Espressif ESP32-S3-MINI-1 / 1U datasheet: https://documentation.espressif.com/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
- LSI/CSI LS7366R datasheet: https://lsicsi.com/wp-content/uploads/2021/06/LS7366R.pdf
- Siproin PM004MNIATR LCSC page: https://www.lcsc.com/product-detail/C5444277.html
- Everspin MR25H40 product page: https://www.everspin.com/products/series/mr25h40
- Infineon/Fujitsu MB85RS4MT FRAM datasheet: https://www.mouser.com/datasheet/2/1113/MB85RS4MT_DS501_00053_1v0_E-2329137.pdf
- Micro Crystal RV-3028-C7 datasheet: https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf
- TI ISO6721 product page: https://www.ti.com/product/ISO6721
- TI ISO1212 product page: https://www.ti.com/product/ISO1212
- TE/Linx RXM-418-LR product page: https://www.te.com/en/product-RXM-418-LR.html
- Mean Well IRM-05 datasheet: https://www.meanwell.com/Upload/PDF/IRM-05/IRM-05-SPEC.pdf
- TI SN74LVC2G17 product page: https://www.ti.com/product/SN74LVC2G17
- TI TPSM82822 product page: https://www.ti.com/product/TPSM82822
- Diodes AP63203 datasheet: https://www.diodes.com/datasheet/download/AP63200-AP63201-AP63203-AP63205.pdf
- onsemi MOC3163M datasheet: https://www.onsemi.com/download/data-sheet/pdf/moc3163m-d.pdf
