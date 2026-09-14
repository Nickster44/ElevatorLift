# SnapEDA library import record

## Registered KiCad libraries

The SnapEDA download is vendored in the repository and registered in the
**project** library tables with the nickname `SnapEDA`. KiCad can use its
symbols as `SnapEDA:<symbol>` and its footprints as `SnapEDA:<footprint>`.

| Library type | Source path | Registered project table |
| --- | --- | --- |
| Symbols | `${KIPRJMOD}/SnapEDA-Library/SnapEDA-Library.kicad_sym` | `hardware/sym-lib-table` |
| Footprints | `${KIPRJMOD}/SnapEDA-Library/Footprints.pretty` | `hardware/fp-lib-table` |

The complete download package, including supplied STEP files, is stored at
`hardware/SnapEDA-Library`. The original `D:\Downloads` folder is no longer
required. The project tables were updated through Konnect to use portable
`${KIPRJMOD}` paths, and representative ESP32, USB, RAC10 and custom footprint
assets resolve from the project directory.

The imported footprints currently contain no active KiCad 3D-model association,
so the STEP files travel with the project but will not appear automatically in
the 3D viewer until their models are associated and aligned during footprint
qualification.

## Imported coverage

Symbols and matching footprints were downloaded for:

- ESP32-S3-WROOM-1U-N16R8
- RAC10-12SK/277
- R-78E5.0-0.5
- RXM-418-LR
- TSW-104-07-G-S and TSW-106-07-G-S
- TPD2EUSB30ADRTR
- RC0603FR-075K1L
- SMBJ15A

The current design uses `LICAL-DEC-MS001` at U41 with `SnapEDA:20-SSOP`.
The earlier missing-footprint description is superseded; the installed asset
still requires manufacturer package verification.

## Selected USB connector

`USB4145-03-0230-C_REVA2` is now the selected USB-C receptacle. The MCU sheet
uses it as J10 with the matching `SnapEDA:GCT_USB4145-03-0230-C_REVA2`
footprint. The former USB4120-03-C selection is superseded and should not be
used on this revision.
