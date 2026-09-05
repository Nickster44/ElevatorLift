# Datasheet Library

This folder holds local datasheet copies for the first-pass controller ICs and modules. Treat these as design references, not as a released BOM. Before schematic release or JLCPCB assembly order, re-check the latest manufacturer revision, LCSC stock, JLC assembly support, package, and lifecycle status.

| File | Block | Candidate part | Source URL |
| --- | --- | --- | --- |
| `ESP32-S3-WROOM-1_WROOM-1U_datasheet.pdf` | MCU / Wi-Fi | ESP32-S3-WROOM-1U-N16R8 | https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf |
| `LS7366R-S_quadrature_counter_datasheet.pdf` | Quadrature counter | LS7366R-S | https://lsicsi.com/wp-content/uploads/2021/06/LS7366R.pdf |
| `Siproin_PM004MNIATR_4Mbit_SPI_MRAM_datasheet.pdf` | Critical nonvolatile memory | PM004MNIATR | https://atta.szlcsc.com/upload/public/pdf/source/20260107/FA25787C4C17263044D220C5BFBDF06F.pdf |
| `Winbond_W25Q128JV_128Mbit_SPI_Flash_datasheet.pdf` | Deferred bulk-storage reference; not fitted in Rev A | W25Q128JVSIQ | https://datasheet.lcsc.com/datasheet/pdf/d009d960f1daec6149edaa35b7dae856.pdf?productCode=C97521 |
| `MicroCrystal_RV-3028-C7_RTC_datasheet.pdf` | RTC | RV-3028-C7-32.768kHz-1ppm-TA-QC | https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf |
| `TI_ISO6721_dual_digital_isolator_datasheet.pdf` | VFD UART isolation | ISO6721BDR | https://www.ti.com/lit/ds/symlink/iso6721.pdf |
| `Chipanalog_CA-IS372x_dual_digital_isolator_datasheet.pdf` | VFD UART isolation alternate | CA-IS3721 / CA-IS372x family | https://datasheet.lcsc.com/datasheet/pdf/604064461aa0c12ec9a7680be1fb8a63.pdf?productCode=C528650 |
| `TI_ISO1211_ISO1212_digital_input_receiver_datasheet.pdf` | Industrial input receiver | ISO1211DR / ISO1212 | https://www.ti.com/lit/ds/symlink/iso1211.pdf |
| `Linx_RXM-418-LR_receiver_datasheet.pdf` | Legacy RF receiver | RXM-418-LR | https://datasheet.lcsc.com/datasheet/pdf/35ec37f75e2d2c95669ebbe0ea837597.pdf?productCode=C6670221 |
| `Mornsun_LS05-13B05R3_ACDC_module_datasheet.pdf` | 120 VAC to 5 V supply reference | LS05-13B05R3 series | https://fiduspower.com/media/downloadable/files/attachment//l/s/ls05-13bxxr3.pdf |
| `RECOM_RAC10-12SK277_ACDC_module_datasheet.pdf` | 120 VAC to 12 V supply | RAC10-12SK/277 | https://g.recomcdn.com/media/Datasheet/pdf/.fNZH1XvY/.t0407127c408a9a98940f/Datasheet-122/RAC10-K_277.pdf |
| `Diodes_AP63200_AP63203_buck_datasheet.pdf` | 3.3 V buck regulator | AP63203WU-7 | https://datasheet.lcsc.com/datasheet/pdf/94820cc9e44613233ddcc1a15801eca3.pdf?productCode=C2071868 |
| `TI_SN74LVC2G17_schmitt_buffer_datasheet.pdf` | Encoder input conditioning | SN74LVC2G17DBVR | https://www.ti.com/lit/ds/symlink/sn74lvc2g17.pdf |
| `LiteOn_MOC3063S_optotriac_datasheet.pdf` | AC light output driver | MOC3063S-TA1 | https://dfsimg1.hqewimg.com/group5/M00/17/1F/wKhk3WYsy4-ACPLCAAL-o50Ymog025.pdf |
| `Maxim_MAX3483_MAX3485_RS485_datasheet.pdf` | Optional RS-485 VFD interface | MAX3485ESA+T | https://www.farnell.com/datasheets/2002060.pdf |
| `TI_TXS0104E_level_translator_datasheet.pdf` | Observed VFD serial level translation | TXS0104E | https://www.ti.com/lit/ds/symlink/txs0104e.pdf |
| `RECOM_R-78E5.0-0.5_5V_regulator_datasheet.pdf` | Selected Rev-A 12 V to 5 V regulator; legacy board used it from 24 V | R-78E5.0-0.5 | https://recom-power.com/pdf/Innoline/R-78E-0.5.pdf |
| `Toshiba_TLP291-4_photocoupler_datasheet.pdf` | Observed encoder isolation | TLP291-4 | https://toshiba.semicon-storage.com/info/TLP291-4_datasheet_en_20191129.pdf?did=12858&prodName=TLP291-4 (local download pending) |
| `Linx_LICAL-DEC-MS001_decoder_datasheet.pdf` | Legacy RF decoder | LICAL-DEC-MS001 | https://www.te.com/en/product-LICAL-DEC-MS001.html (local download pending) |

## Notes

- The preferred MRAM candidate is currently Siproin `PM004MNIATR`, because it has a strong LCSC path and is much cheaper than the Everspin fallback.
- The RECOM `RAC10-12SK/277` is now the baseline AC/DC supply because the old accessory board used it and JLCPCB lists it as assembly part `C5199922`.
- The LS05-13B05R3 PDF remains a 5 V fallback reference. LCSC currently lists a `DEXU Electronics LS05-13B05R3` candidate; verify the final selected supplier datasheet before releasing mains layout.
- The observed VFD stage is a TI `TXS0104E`, not yet a confirmed opto-input current driver. Its datasheet is now local; validate the actual VFD header electrically before finalizing that block.
- The legacy encoder path uses a `TLP291-4`; the official Toshiba PDF reference is recorded, but the vendor server rejected automated download. Do not substitute an unverified mirror.
- The Linx/TE `LICAL-DEC-MS001` decoder datasheet is now explicitly required because the separate physical `LEARN` node must be brought into the new design. The official TE PDF reference is recorded, but automated download was rejected.
- The optotriac and MAX3485 PDFs were downloaded from alternate direct PDF mirrors because the vendor/LCSC direct links did not download cleanly in this environment.
