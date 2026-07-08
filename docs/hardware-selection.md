# Hardware Selection Draft

This document identifies the first-pass hardware blocks and candidate ICs/modules needed to build the new controller. These are not final schematic decisions yet; they are the parts to evaluate before KiCad capture.

## Summary Recommendation

Build the first board around:

- ESP32-S3 module with external antenna connector.
- SPI quadrature counter IC.
- SPI MRAM for critical state and recent logs.
- Optional external flash footprint for WebUI assets/OTA/longer logs.
- Isolated/protected VFD UART.
- Conditioned open-collector encoder input path.
- Local fallback AP Wi-Fi setup flow.
- 120 VAC to isolated DC power module if the board fits inside the VFD housing.
- Compact light output only unless interlocks/solenoids are reintroduced.

## Candidate Parts

| Block | Preferred candidate | Why it fits | Notes |
| --- | --- | --- | --- |
| MCU/Wi-Fi | Espressif ESP32-S3-WROOM-1U | Wi-Fi AP/station support, multiple UART/SPI/I2C peripherals, native USB, external antenna connector for metal enclosure use | Use a U.FL/IPEX pigtail to move antenna outside or to a plastic window |
| Compact MCU alternate | Espressif ESP32-S3-MINI-1U | Smaller external-antenna ESP32-S3 module | Less flash/PSRAM flexibility depending on variant |
| Quadrature counter | LSI/CSI LS7366R | 32-bit quadrature counter with SPI host interface | Good match for offloading encoder counting from MCU |
| Encoder conditioning | SN74LVC2G17 or similar Schmitt buffer | Cleans slow/noisy open-collector encoder edges before counter input | Add pullups, series resistors, RC option, and ESD/TVS protection |
| Critical nonvolatile memory | Everspin MR25H40 | 4 Mbit SPI MRAM, 40 MHz, nonvolatile, unlimited endurance, power-loss retention | Preferred over flash/EEPROM for frequent position/log writes |
| Lower-cost memory alternate | Infineon/Fujitsu MB85RS4MT | 4 Mbit SPI FRAM with high endurance | FRAM, not MRAM; acceptable fallback if MRAM cost/sourcing is poor |
| Web assets / extended logs | Winbond W25Q128JV or similar QSPI NOR | Cheap high-density storage for static WebUI files, OTA image staging, and noncritical logs | Do not use for high-frequency critical position snapshots |
| RTC timestamps | Micro Crystal RV-3028-C7 | Very low power I2C RTC with UNIX time counter and backup support | Useful when AP-only/offline and no NTP is available |
| VFD UART isolation | TI ISO6721 / ISO6721-Q1 | Dual-channel digital isolator suitable for UART TX/RX | Pair with isolated power if a true isolated interface is required |
| Industrial digital inputs | TI ISO1211 / ISO1212 | Isolated digital input receiver family for industrial input modules | Candidate if field inputs are 24 V or higher; exact input voltages still need confirmation |
| RF receiver | TE/Linx RXM-418-LR | Maintains compatibility path with existing 418 MHz remotes | Requires firmware validation/decoding; RF commands are not safety signals |
| AC/DC power | Mean Well IRM-05-5 or IRM-05-12 | PCB-mount isolated AC/DC module, compact, universal AC input | Need load budget before choosing 5 V versus 12 V output |
| 3.3 V rail | TI TPSM82822/TPSM82823 or Diodes AP63203 | Compact buck regulator options depending on upstream DC rail | TPSM8282x is easy from 5 V; AP63203 handles a wider input range |
| AC light output | MOC3063/MOC3163 class optotriac plus triac, or compact relay/SSR | Compact path for 120 VAC lighting output | Use relay/SSR if load type or leakage current makes triac unsuitable |

## Architecture Notes

### MCU And Wireless

Use an external-antenna ESP32-S3 module rather than a PCB antenna module if the board is inside the metal VFD enclosure. The antenna should be routed to a suitable external or non-metal location. The ESP32-S3-WROOM-1U is the baseline because the WebUI, AP fallback, station mode, VFD UART, SPI counter, SPI MRAM, optional flash, and I2C RTC all fit its peripheral set.

### Position Counting

The likely SKF encoder outputs two 90-degree phase-shifted open-collector square waves. The counter path should be:

```text
Encoder A/B -> protection/current limit -> pullups -> optional RC filter -> Schmitt buffer -> LS7366R -> SPI -> MCU
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

Use optional QSPI NOR flash or SD only for noncritical bulk storage. A removable SD card is not required for the first board unless the design needs long-term removable logs. External QSPI flash is a better first option for WebUI assets and OTA staging because it avoids sockets and removable-media reliability issues.

### VFD Interface

The old controller drove the VFD from `Serial1` directly at 9600 baud. Before finalizing the schematic, confirm whether the VFD serial pins are TTL-level, RS-232-like, RS-485-like, or a vendor-specific isolated logic port.

Recommended default:

- Include pads/footprint option for a digital isolator on VFD TX/RX.
- Include series resistors and ESD protection.
- Keep VFD serial routing away from mains/motor output wiring.
- Add a hardware enable/stop path independent of serial commands if the VFD supports it.

### Inputs

Split inputs into classes:

- Encoder A/B: fast conditioned logic into the quadrature counter.
- Safety loop/final limits: hardwired motion authority plus firmware monitoring.
- User buttons/home/service inputs: protected low-voltage inputs if possible.
- RF receiver data: logic input with validation in firmware.

If existing field wiring uses 24 V or higher, evaluate ISO1211/ISO1212 input channels. If all local switches can be converted to low-voltage dry-contact inputs, simpler protected GPIO front ends may be smaller and cheaper.

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

Start with a 5 W budget check. If ESP32 Wi-Fi, RF receiver, MRAM, counter, isolator, and light-output control are the only loads, 5 W is likely enough. If external relays, sensors, or auxiliary outputs are added, revisit the power budget.

## Open Decisions Before Schematic Capture

1. Confirm exact VFD serial electrical interface.
2. Measure available space and mounting options inside the VFD housing.
3. Confirm available 120 VAC tap point and grounding/chassis strategy.
4. Confirm exact encoder model, supply voltage, cable length, and pullup voltage.
5. Confirm whether existing button/safety/home/limit wiring is low voltage, 24 V, or 120 VAC.
6. Confirm lift light voltage, current, and load type.
7. Decide whether RXM-418-LR compatibility is mandatory on revision A.
8. Decide whether to include optional QSPI flash, microSD footprint, or both.
9. Decide how much board area can be reserved for isolation/protection versus compactness.

## Source References

- Espressif ESP32-S3-WROOM-1 / 1U datasheet: https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
- Espressif ESP32-S3-MINI-1 / 1U datasheet: https://documentation.espressif.com/esp32-s3-mini-1_mini-1u_datasheet_en.pdf
- LSI/CSI LS7366R datasheet: https://lsicsi.com/wp-content/uploads/2021/06/LS7366R.pdf
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

