# JLCPCB / LCSC Sourcing Matrix

This is the working sourcing matrix for the first controller PCB. The intent is to bias the design toward parts that can be ordered through LCSC and assembled by JLCPCB, while still keeping technically better fallback options visible.

Status meanings:

- `Strong`: LCSC part exists and package is assembly-friendly; still verify JLC Basic/Extended status in the assembly quote.
- `Likely`: LCSC part exists, but final assembly support, stock depth, or package rules need order-time verification.
- `Weak`: usable as a design reference or fallback, but not ideal for a JLC-populated board.
- `TBD`: needs final electrical decision before part selection.

| Block | Preferred part | LCSC candidate | Status | Local datasheet | Notes |
| --- | --- | --- | --- | --- | --- |
| MCU / Wi-Fi | ESP32-S3-WROOM-1U-N16R8 | `C3013946` | Strong | `datasheets/ESP32-S3-WROOM-1_WROOM-1U_datasheet.pdf` | External antenna version is preferred because the PCB may sit inside a metal VFD enclosure. LCSC lists the module as in stock. |
| Quadrature counter | LS7366R-S | `C3827808` | Likely | `datasheets/LS7366R-S_quadrature_counter_datasheet.pdf` | Keeps encoder edge counting out of MCU interrupt timing. Confirm JLC assembly availability; if weak, preserve the footprint and consider hand placement for rev A. |
| Critical NVM | Siproin PM004MNIATR | `C5444277` | Strong | `datasheets/Siproin_PM004MNIATR_4Mbit_SPI_MRAM_datasheet.pdf` | 4 Mbit SPI/QPI MRAM, 2.7 V to 3.6 V, SOP-8, good China/LCSC fit. This is the preferred MRAM candidate. |
| Critical NVM fallback | Everspin MR25H40CDF | `C246235` | Weak | None local | Technically excellent, but LCSC stock depth and price are poor compared with Siproin. Use only if qualification demands it. |
| Deferred bulk storage | Winbond W25Q128JVSIQ | `C97521` | Strong | `datasheets/Winbond_W25Q128JV_128Mbit_SPI_Flash_datasheet.pdf` | Not fitted in Rev A; retain only as a later-revision option if the N16R8 module's 16 MB flash proves insufficient. |
| RTC | Micro Crystal RV-3028-C7-32.768kHz-1ppm-TA-QC | `C3019759` | Strong | `datasheets/MicroCrystal_RV-3028-C7_RTC_datasheet.pdf` | LCSC lists stock. Useful for timestamped logs when there is no network/NTP connection. |
| VFD UART isolation alternate | TI ISO6721BDR | `C5216430` | Strong | `datasheets/TI_ISO6721_dual_digital_isolator_datasheet.pdf` | Not fitted in the captured TXS0104E reference topology. Retain as an option only if bench testing establishes a complete isolated-side power and signal strategy. |
| VFD UART isolation alternate | Chipanalog CA-IS3721 / CA-IS372x | `C528650` | Likely | `datasheets/Chipanalog_CA-IS372x_dual_digital_isolator_datasheet.pdf` | China-source alternate with 150 Mbps family rating and wide supply range. Check exact suffix, channel direction, and default output state. |
| VFD UART translator | TI TXS0104ED | Verify at order time | Likely | `datasheets/TI_TXS0104E_level_translator_datasheet.pdf` | Captured to match the field-used 3.3 V/5 V interface. Assembled-board header pinout, levels and fault behavior remain bench gates. |
| VFD alternate driver | MMBT3904 class NPN or small logic MOSFET | `C20526` for MMBT3904 | Strong | Datasheet not local | Not fitted by default. Retain only as a redesign option for a different VFD electrical interface or an unexpected assembled-board test result. |
| Industrial digital input | TI ISO1211DR | `C2674102` | Likely | `datasheets/TI_ISO1211_ISO1212_digital_input_receiver_datasheet.pdf` | Candidate for 24 V to 60 V field inputs. Do not use by default if final wiring can be low-voltage dry contacts. |
| Encoder conditioning | TI SN74LVC2G17DBVR | `C10429` | Strong | `datasheets/TI_SN74LVC2G17_schmitt_buffer_datasheet.pdf` | Schmitt buffer after pullups, current limiting, filtering option, and ESD protection. Also review China-source equivalents if cost matters. |
| Legacy RF receiver | Linx / TE RXM-418-LR | `C6670221` | Likely | `datasheets/Linx_RXM-418-LR_receiver_datasheet.pdf` | Preserves existing remote compatibility. Confirm JLC assembly support and RF module placement rules; RF commands must remain non-safety commands. |
| 120 VAC to 12 V module | RECOM RAC10-12SK/277 | `C5199922` | Strong | `datasheets/RECOM_RAC10-12SK277_ACDC_module_datasheet.pdf` | JLCPCB lists this part for assembly as an Extended, wave-soldered AC/DC module. It powered the previous installation; Rev A can move lighting to its jumper-selectable external 12 V feed if load margin requires it. |
| 120 VAC to 5 V fallback | LS05-13B05R3 class module | `C41381028` | Likely | `datasheets/Mornsun_LS05-13B05R3_ACDC_module_datasheet.pdf` | Useful only if the 12 V light/output strategy changes. Verify supplier datasheet, creepage, clearance, approvals, and height before layout release. |
| 3.3 V buck | Diodes AP63203WU-7 | `C780769` | Likely | `datasheets/Diodes_AP63200_AP63203_buck_datasheet.pdf` | Fixed 3.3 V, 2 A, TSOT-23-6. If stock is weak at order time, AP63200 adjustable (`C2071868`) or a China-source 3.3 V buck can be substituted. |
| 12 V light switch | AO3400A N-channel MOSFET | `C20917` | Strong | Datasheet not local | JLC Basic SOT-23 MOSFET. Good candidate for a protected low-side 12 V light output if wiring allows low-side switching. |
| AC light driver | LiteOn MOC3063S-TA1 plus external triac | `C77950` | Strong | `datasheets/LiteOn_MOC3063S_optotriac_datasheet.pdf` | Compact replacement for a bulky light relay if the load is compatible with triac leakage and zero-cross switching. Final triac, snubber, fuse, and creepage still need design. |
| RS-485 fallback | MAX3485ESA+T | `C18148` | Strong | `datasheets/Maxim_MAX3483_MAX3485_RS485_datasheet.pdf` | Include only if the VFD serial interface is RS-485 or if a universal VFD interface option is desired. China equivalents can reduce cost. |

## Current BOM Direction

For a JLCPCB-friendly revision A, use these as the default major parts unless bench testing invalidates them:

- ESP32-S3-WROOM-1U-N16R8 for the MCU/Wi-Fi module.
- Siproin PM004MNIATR for critical MRAM.
- LS7366R-S for quadrature counting, with a fallback plan if JLC assembly support is weak.
- N16R8 internal flash for WebUI/static storage and OTA; do not fit W25Q128JVSIQ in Rev A.
- RV-3028-C7 for offline timestamps.
- TXS0104ED for the field-matched VFD UART topology; keep a discrete driver or ISO6721/CA-IS372x architecture as a future-interface or unexpected-test redesign option only.
- AP63203WU-7 for 3.3 V from the isolated 12 V rail.
- RXM-418-LR receiver path because legacy remotes are mandatory.
- AO3400A-class protected MOSFET output for the 12 V lift light.

## Order-Time Checks

Before converting this into schematic symbols and PCB footprints:

1. Export the candidate BOM to JLCPCB and verify Basic/Extended status, assembly availability, minimum quantity, and stock.
2. Confirm each package exactly matches the KiCad footprint, not just the family name.
3. Re-check LCSC/JLC inventory for the MCU module, MRAM, LS7366R, RECOM AC/DC module, and RF receiver because those are the highest-risk supply items.
4. Confirm mains module safety approvals and board-level creepage/clearance against the enclosure and installation constraints.
5. Confirm the assembled field-matched VFD serial circuit and connector behavior before release; revisit isolation or RS-485 only for a different VFD interface requirement.
