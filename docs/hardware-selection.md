# Rev-A Hardware Selection

Updated 2026-09-14. This is the selected design, not a menu of interchangeable
parts. See the [capture register](schematic-capture-readiness.md) for components,
[sourcing matrix](jlcpcb-lcsc-sourcing.md) for order-time identifiers, and
[hardware dependencies](hardware-dependency-handoff.md) for qualification gates.

## Selected architecture

- ESP32-S3-WROOM-1U-N16R8 with external antenna, 16 MB flash and 8 MB octal PSRAM.
  GPIO35-37 are unavailable. Native USB is the programming path; J11 pins 3/4
  are NC, not a UART recovery interface.
- RAC10-12SK/277 isolated 12 V supply from protected AC input; R-78E5.0-0.5
  supplies 5 V and AP63203WU-7 supplies 3.3 V from 12 V.
- LM66100 ideal-diode paths isolate the two 5 V sources from backfeed.
  USB VBUS alone does not supply the separate 3.3 V regulator.
- TXS0104ED translates the established 3.3 V/5 V VFD UART. GPIO42 controls OE
  only. No MCU RUN collector or brake output is fitted.
- Encoder A/B use TLP291-4 conditioning with 2.2 kΩ LED resistors, 74HC14D
  cleanup and an LS7366R-S counter. The observed legacy 270 Ω circuit is
  historical reference, not the current resistor selection.
- PM004MNIATR MRAM stores critical records; RV-3028-C7 supplies offline time.
  Retained position is diagnostic data, not proof of absolute position after reset.
- Module flash holds WebUI assets. No external QSPI NOR or microSD is fitted;
  allocated flash capacity does not imply an implemented OTA workflow.
- RXM-418-LR and LICAL-DEC-MS001 retain the existing RF interface, with a wired
  LEARN node and local button. CONSMA002 serves this RF path, not Wi-Fi.
- Eight active-low, optocoupler-conditioned field inputs share controller GND.
  They are not a galvanic safety barrier. J43/J44 accept key, hold and direction
  contacts; their external wiring and independent stopping authority are unverified.
- One protected 12 V light MOSFET output, with onboard/external supply selection.
  Optional AUX and general-purpose relay/solenoid outputs are not fitted.

## Qualification, not alternative selection

The selected parts are not approved for operation merely because they are captured.
Keep the field-matched UART topology pending bench evidence; do not substitute
an isolator, transistor driver or RS-485 interface without an explicit redesign.

Open items include encoder switching margin/rate, power budget and sequencing,
RF straps and physical button mapping, connector/package checks, GPIO3/43 boot
behavior, external antenna/cable mechanics, and four-layer routing constraints.
The [external-box review](reviews/2026-09-13-manual-control-box.md) lists the
specific wiring and drive/brake evidence needed before operational firmware.

Earlier MCU, FRAM, bulk-flash, AC-light and alternate-power candidates have been
removed from current selection guidance. Their original references remain in Git
history and the datasheet collection; they are not substitute BOM entries.
