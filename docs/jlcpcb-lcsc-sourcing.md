# Rev-A Sourcing References

Updated 2026-09-14. These are previously recorded catalogue identifiers for
selected parts, **not current stock, price or assembly-service guarantees**.
Recheck the exact suffix, package and assembly quote at order time. No
substitution is approved by this document.

| Block | Selected part | Recorded LCSC identifier | Qualification note |
| --- | --- | --- | --- |
| MCU | ESP32-S3-WROOM-1U-N16R8 | C3013946 | Corrected project footprint; external antenna variant |
| Counter | LS7366R-S | C3827808 | Confirm package and assembly availability |
| MRAM | PM004MNIATR | C5444277 | Confirm exact revision, package and interface mode |
| RTC | RV-3028-C7-32.768kHz-1ppm-TA-QC | C3019759 | Verify selected suffix against footprint |
| VFD translator | TXS0104ED | Recheck | Preserve field-matched UART topology |
| RF receiver | RXM-418-LR | C6670221 | Assembly or qualified harvested-part plan |
| RF decoder | LICAL-DEC-MS001 | Recheck | Exact 20-SSOP package; no transmitter IC required |
| AC/DC supply | RAC10-12SK/277 | C5199922 | Verify through-hole assembly and approvals |
| 3.3 V buck | AP63203WU-7 | C780769 | No automatic adjustable-variant substitution |
| Light MOSFET | AO3400A | C20917 | Check captured package, load/inrush and thermal margin |

The [capture register](schematic-capture-readiness.md) and exported design BOM
govern the remaining parts, including R-78E5.0-0.5, LM66100, TLP291-4,
74HC14D, connectors and protection. This is not a complete purchasing BOM.

## Before ordering

1. Export the current design BOM and reconcile exact manufacturer suffixes and
   quantities with the PCB, not an old candidate list.
2. Verify supplier stock, minimum quantities and assembly eligibility.
3. Check each footprint against the manufacturer drawing. Resolve D41's
   SMBJ15A/SMA mismatch and the open
   [component-library requests](component-library-request.md).
4. Complete power, mains isolation, enclosure and interface qualification gates.
   Do not order a production release from the unrouted board.

Unused alternate MCU/memory, optotriac, RS-485, industrial-input, UART-isolator
and AC/DC candidates have been removed from this working sourcing list.
Historical datasheets are retained as references only, not fitted parts.
