# Field Verification Log

This log separates observed legacy hardware from design decisions for the new
controller. An observed implementation is useful evidence, but it is not an
automatic approval for the new safety or production design.

## 2026-08-30 Site Inspection Notes

| Area | Observation | Design impact | Status |
| --- | --- | --- | --- |
| PCB envelope | Legacy board is approximately 90 mm x 90 mm. Corner mounting-hole centers are 86 mm apart in both axes, placing their centers approximately 2 mm from each board edge. | Rev-A board outline target is 90 mm x 90 mm. Keep the existing 86 mm square mounting pattern. | Measured/estimated; verify with calipers before fabrication release. |
| Mounting clearance | About 40 mm of enclosure height is available in most areas. The underside needs about 12 mm clearance at the mounting screws. | Keep tall parts away from the standoff/screw keepouts; record exact screw head and standoff dimensions before mechanical release. | Estimated. |
| Mount holes | Hole diameter was described as approximately 2.56 mm. | Treat 2.56 mm as a provisional hole diameter only; select the finished-hole size after identifying the screw size and measuring a hole directly. | Unconfirmed. |
| Legacy controller | Particle Xenon, Linx RF receiver/transceiver hardware, and encode/decode hardware are fitted on headers. Spare legacy boards are available for possible harvesting. | Provide a documented legacy-RF interface option. Harvested parts require electrical, functional, and mechanical inspection before use; do not make them the sole sourcing path for production. | Observed. |
| RF learn access | The decoder `LEARN` signal is not present on its header and is routed to a physical button. The legacy installation uses a soldered wire from that board to the main PCB. | Provide a dedicated `RF_LEARN` interface pad/header and safe-default MCU control. Retain a local physical Learn button or service pads. | Observed. |
| VFD serial connector | A four-pin male header, likely 2.54 mm pitch, carries VFD RX/TX through a TI `TXS0104E`; no discrete transistor or series resistor is used on this path. | The Rev-A circuit has been cross-checked against this field topology and includes a defined OE pull-down and test pads. Verify the assembled board and connector before lift connection. | Electrical topology confirmed against field design; connector pitch, pin order, levels, and assembled-board behavior remain bench gates. |
| Legacy power | An accessory I/O board supplies 24 VDC through Ethernet-style cabling. The legacy controller uses a RECOM `R-78E5.0-0.5` non-isolated 24 V-to-5 V regulator. | The captured combined board takes protected 120 VAC + neutral into `RAC10-12SK/277`, then uses the same R-78E family from 12 V to 5 V and AP63203 from 12 V to 3.3 V. | Legacy input observed; new 12 V-fed converter arrangement selected for Rev A. |
| Encoder | Four-wire encoder wiring appears to use 12 V supply. A/B are pulled up to 12 V through 270 ohm resistors, then pass through 270 ohm series resistors into a `TLP291-4` optocoupler. An additional optocoupler and an HC74 flip-flop are visible but their function is unknown. | Reproduce the electrical behavior only after measuring encoder current, A/B polarity, pulse rate, and optocoupler outputs. The new design must provide protected isolated/translated logic compatible with the LS7366R counter. | Part markings and resistor values observed; signal routing unconfirmed. |
| Safety/reference wiring | The safety-loop wire and HOME limit switch are intended to terminate at the new controller. | Include both as monitored inputs while preserving independent hardwired safety authority. | Requirement confirmed; voltage, contact type, and chain topology remain open. |
| EMI/shielding | No deliberate shielding or ground plane was evident on the legacy PCB. | Rev A must add ground planes, separation of mains/VFD/encoder/RF regions, surge/ESD protection, and a chassis/shield plan. | Observed. |

## Evidence Still Required Before Schematic Release

1. Measure the VFD four-pin header pinout, common/reference, idle voltage, and cable length with the drive powered and disconnected from the controller as appropriate.
2. Verify the assembled field-matched `TXS0104E` stage, connector pinout, levels, and fault behavior on a bench fixture before connecting a new controller to the lift.
3. Measure encoder supply, A/B high/low voltage, low-state current, pulse rate at maximum speed, cable length, and shield termination. Trace the outputs of the `TLP291-4` and HC74.
4. Identify the safety-loop and HOME switch voltage, contact topology, and which motion authority remains hardwired when the MCU is unpowered.
5. Confirm the AC tap, neutral, protective-earth connection, disconnect relationship, fuse location, and wire gauge.
6. Measure the light load voltage, steady current, inrush, connector, and return path before sizing the 12 V supply and output switch.
7. Measure actual mounting-hole diameter and screw/standoff dimensions.

## Referenced Datasheets

- `TI_TXS0104E_level_translator_datasheet.pdf` — local copy; 3.3 V/5 V auto-direction translator reference.
- `RECOM_R-78E5.0-0.5_5V_regulator_datasheet.pdf` — local copy; selected Rev-A 12 V-to-5 V regulator, with legacy 24 V use retained as historical evidence.
- Toshiba `TLP291-4` official datasheet — local download pending because Toshiba's server rejected automated retrieval.
- TE/Linx `LICAL-DEC-MS001` official datasheet — local download pending because TE's server rejected automated retrieval.
