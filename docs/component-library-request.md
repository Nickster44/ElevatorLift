# Component library and source request list

This list has been reset to contain only parts whose final manufacturer library
assets are still useful. The schematic already uses verified KiCad or imported
SnapEDA assets for the ESP32, USB connector/protection, RECOM supplies, Linx RF
parts, VFD header, TXS0104E, TLP291 devices, AP63203, LM66100 and standard
passives.

Do not install a downloaded footprint until its pin numbering, pad/drill geometry,
body outline and courtyard have been checked against the manufacturer drawing.

## Files still requested

| Priority | Exact part number | Files wanted | Why |
| --- | --- | --- | --- |
| High | `ED300/3` | KiCad/EasyEDA footprint and 3D model; symbol optional | This is the stocked 3-position, 5.00 mm field terminal used for encoder, switches and outputs. The schematic currently uses an Altech 5.00 mm provisional footprint. Manufacturer identity and exact body/drill dimensions are still needed. |
| High | Phoenix Contact `1717732` (`GMKDS 1,5/3-7,62`) | Manufacturer footprint/drawing and 3D model | Selected for the mains L/N/PE entry because its 7.62 mm pitch can support the project mains-clearance rule. A datasheet-derived provisional project footprint is installed; verify drill, body and courtyard before layout release. |
| High | `PM004MNIATR` | Manufacturer/EasyEDA symbol, SOP-8 footprint and 3D model | A datasheet-derived project-local symbol is already wired for the mandatory 4 Mbit MRAM. A downloaded asset is wanted as an independent pin-number and package check before PCB layout. |
| High | `LS7366R-S` | Manufacturer/EasyEDA symbol, SOIC-14 footprint and 3D model | A datasheet-derived project-local symbol is already wired. A downloaded asset is wanted as an independent pin-number/package check. |
| Medium | `RV-3028-C7` | Manufacturer/EasyEDA footprint and 3D model | The RTC uses the standard KiCad symbol and provisional `LGA-8_2x3mm_P0.5mm` footprint. A manufacturer model would improve final mechanical verification. |

## Already imported or resolved

| Part number | Current project asset/status |
| --- | --- |
| `ESP32-S3-WROOM-1U-N16R8` | Imported SnapEDA symbol retained. The downloaded footprint's invalid 0.2 mm plated-hole array was replaced by project footprint `ESP32-S3-WROOM-1U-N16R8_CorrectedV2`; verify its exposed-pad segmentation during PCB review. |
| `USB4145-03-0230-C_REVA2` | Imported SnapEDA symbol and vertical GCT footprint; placed as the USB-C connector. |
| `TPD2EUSB30DRTR` | Imported SnapEDA symbol/footprint; placed at the USB data lines. |
| `RAC10-12SK/277` | Imported SnapEDA symbol/footprint; exact part is placed on the power sheet. |
| `R-78E5.0-0.5` | Imported SnapEDA symbol/footprint; exact part is placed on the power sheet. |
| `RXM-418-LR` | Imported SnapEDA symbol/footprint; exact receiver is placed. |
| `LICAL-DEC-MS001` | Imported SnapEDA symbol and 20-SSOP footprint; exact decoder is placed. |
| `TSW-104-07-G-S` | Imported symbol retained; project footprint `SAMTEC_TSW-104-07-G-S_ZeroPadded` resolves the imported symbol's `01`-`04` numbering. |
| `TSW-106-07-G-S` | The optional UART recovery interface uses the compatible KiCad 1x06, 2.54 mm vertical-header footprint. |
| `CONSMA002` | Project footprint `SMA_TE-Linx_CONSMA002_RightAngle` was created from the TE customer drawing and assigned to the 418 MHz Linx antenna input. A 3D model remains optional. |
| `AP63203WU-7` | Standard KiCad symbol and TSOT-23-6 footprint are sufficient. |
| `LM66100DCKR` | Standard KiCad symbol and SC-70-6 footprint are sufficient. |
| `TXS0104ED` | Standard KiCad symbol and SOIC-14 footprint are sufficient. |
| `TLP291` / `TLP291-4` | Standard KiCad symbols and Toshiba-compatible SOIC footprints are sufficient. |

## Selected real parts that do not need downloaded symbols

- Mains terminal: Phoenix Contact `1717732` (`GMKDS 1,5/3-7,62`); final downloaded mechanical asset is still requested above.
- Primary fuse holder: Schurter `0031.8201`, with a T500 mA 250 VAC 5 x 20 mm fuse.
- Mains MOV: Bourns `MOV-14D241K`.
- 3.3 V buck inductor: Bourns `SRP7028A-2R2M`, 2.2 uH. KiCad includes the exact series footprint.
- Protected 12 V distribution fuse: Bourns `MF-MSMF075/24X`, 0.75 A hold, 24 V, 1812 PPTC.
- USB-C CC resistors: Yageo `RC0603FR-075K1L`, 5.1 kOhm, 1%.

## Information to include with each download

- Exact manufacturer and full package suffix.
- Manufacturer datasheet or product drawing.
- KiCad/EasyEDA symbol and footprint files, plus 3D model when available.
- For connectors, a product photo and mating-part information.
