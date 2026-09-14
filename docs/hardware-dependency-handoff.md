# Hardware Dependencies For Software Integration

Updated 2026-09-13. Software now uses contract v2,
`rev-a-2026-09-13-diagnostics-inhibited`. See the
[pin/comms/four-layer handoff](reviews/2026-09-13-pin-comms-fourlayer.md)
and its fresh exported connectivity evidence. Historical 2026-09-10 exports
remain historical; the software contract test now checks the finalized mapping evidence.

**HW-01 and HW-02 are closed at schematic/connectivity level only.** Live PCB
pad checks agree with the new export, and synchronization is a no-op. Firmware
motion integration and physical qualification gates remain open. Do not flash the old
profile: its AUX initialization drives GPIO3 LOW, now a SERVICE_KEY input.

| ID | Required decision / correction | Current software behavior | Evidence needed to close |
| --- | --- | --- | --- |
| HW-01 | Schematic closed: LIMIT_UP GPIO43/p37 through 1k R62; LIMIT_DOWN GPIO44/p36; SERVICE_KEY GPIO3/p15; GPIO35/36/37 NC | Adopted by v2; AUX removed; UART0 application console/logging disabled before input setup | Input truth tables/continuity; GPIO43 ROM boot-output and GPIO3 key/reset/rail-sequence tests |
| HW-02 | Schematic closed: GPIO42 VFD_COMMS_ENABLE connects only U24 OE and R24 pulldown; R25/U25/J25/TP26 removed | LOW at setup, HIGH after UART1 initialization for diagnostics only; no RUN or WRITE path | Verify STOP/readback with external hardwired safety open; reset/brownout/watchdog and independent external authority tests |
| HW-03 | Electrical closure-to-GND = MCU LOW confirmed; installed NO/NC, wire-fault behavior and independent final limits unresolved | Safety LOW=healthy; limits/key use active-low stability filtering, but FieldContinuityQualified=false. Broken limit wires can resemble clear limits | MB-01/03/04 in the external-box review; actual field contact schedule and independent final-limit trace |
| HW-04 | Confirm encoder polarity/scale and field conditioning; exported U32 LS7366R has 4 MHz local clock | SPI mode0, 1 MHz, x4/32-bit/free running; position stays invalid after reset | A/B phase, maximum rate, count-per-travel, filter latency, reset/power-loss behavior; safe no-motion encoder simulation |
| HW-05 | JP41 BAUD0/U41.3 and JP42 BAUD1/U41.4 verified; 1-2=LOW, 2-3=HIGH. Installed diagnostic shunts and D40/learn qualification remain open | Configurable diagnostic baud defaults to 9600; D0-D4 map is floor 1, light toggle, floor 3, floor 2, STOP. Physical mapping remains unverified | Match diagnostic jumpers to UART configuration, not remote timing; verify TX_ID, MODE_IND and learn/erase behavior. U41.7 is grounded for momentary active-HIGH outputs |
| HW-06 | Verify 12 V load budget, JP40 onboard/external supply selection and light output wiring; review remaining D41 footprint mismatch (D42 removed) | Light set-state GPIO41; AUX removed, GPIO3 input-only; no physical load feedback claimed | RAC10 baseline/peak load, upgraded-light external-supply selection, MOSFET protection/polarity and GPIO3 reset review |
| HW-07 | Confirm native USB recovery power path | USB19/20 reserved; build uses native USB CDC | Current export USB VBUS feeds 5 V but not the 3.3 V logic rail; define approved low-voltage bench power procedure before connecting a board |
| HW-08 | Verify RV-3028 backup source and configuration | Read-only UNIX time, POR invalidation, nullable timestamps | J12 backup power, permitted trickle-charge policy, power-loss retention test; no automatic EEPROM/charger writes |
| HW-09 | Confirm PM004 exact revision/mode defaults and SIO2/SIO3 strapping | Verify ID/mode registers, zero-latency SPI only; aligned 16-bit-word accesses | Local V1.34 sheet and assembled device behavior, unused-QPI-pin recommendations vs exported pulls, interrupted writes and retained memory |
| HW-10 | Routing/ERC/DRC, mechanical dimensions, connector access and antenna | Nothing marked manufacturing/deployment ready | Resolve review violations/unrouted nets and physically measure cabinet/90 mm outline/mounting/clearance |
| HW-11 | External keyed override and independent stopping/brake authority are not evidenced; cargo-only operational profile blocked | Diagnostic-only v2 remains inhibited; healthy SAFETY_MON cannot identify override, and SERVICE_KEY is not verified override feedback | External-box review MB-01 through MB-05: bypass/contact schedule, independent E-stop/final limits, VFD command precedence, brake behavior and fault response; subsequent qualified physical tests |

The field-used TXS0104E translation topology is not being rejected or silently
rewired. HW-02 concerns the *additional coupling to run permission*, not merely
UART logic levels. Preserve the independent hardwired safety chain in every option.
No firmware workaround may assert RUN just to obtain telemetry or parameters.
The board no longer provides an MCU-driven RUN collector. The external VFD
configuration and hardwired safety circuit must still be verified to prevent
serial commands from bypassing that authority.

## Software Adoption And Remaining Bench Gates

The mapping, input setup, UART0 suppression, and diagnostic UART items below are
implemented in v2 and regression-tested on host. GPIO HIGH alone cannot establish
wire continuity: `FieldContinuityQualified` remains false and no motion is allowed.
The active-low switch NO/NC arrangement and power-loss detection require physical
review; debounce is not a substitute. No eFuse or bootloader reconfiguration occurred.

- Contract v2 and generated PinMap adopt the new export; keep them synchronized.
- GPIO3/43/44 are inputs and AUX handling is removed. Preserve the
  existing active-low conventions, with unknown/unvalidated inputs inhibiting motion.
- UART0 application console use is disabled; J11 pins 3/4 are NC. Native
  USB programming remains available, subject to the existing bench-power gate.
- VFD_COMMS_ENABLE separates communication availability from motion permission.
  Preserve GPIO42 LOW at startup, then enable UART for STOP/readback
  independently of the safety state. A transmitted STOP is not proof of stopping.
- Keep default JTAG eFuses: GPIO3 is externally pulled HIGH when the key is open
  and LOW when closed. Do not enable GPIO3-controlled pad-JTAG selection; it can
  repurpose GPIO39-42. No eFuses were changed during this work.
- Keep uploads/motion inhibited until prototype reset, input, communication,
  external safety-authority, and remaining hardware gates are qualified.

## Physical Verification Boundary

### Requested Cargo Manual-Control Profile: Pending Safety Architecture Evidence

The [external manual-control box evidence review](reviews/2026-09-13-manual-control-box.md)
contains the fresh netlist, mode/input and failure tables, RF strap truth table,
stopping-authority boundary, and exact non-energized owner evidence checklist.
No external-box as-wired circuit or installed drive/brake configuration was found
in the inspected project evidence. HW-11 remains open. The intended hardwired
authority named in the contract must not be represented as physically verified.

The requested external manual-control box owns its keyed safety-loop override.
Its asserted override supplies the same SAFETY_MON state as the existing healthy
loop. Preserve the existing truth table: GPIO1 LOW = healthy, GPIO1 HIGH =
non-healthy. The exported handoff describes contact-to-GND closed = optocoupler
ON = MCU LOW, consistent with `digitalRead(Pins::SafetyLoop) == LOW` in firmware.
This records the requested interface behavior, not verification of the external box.

The requested operational revision is not enabled. Contract v2 and its motion/
upload inhibit remain unchanged. Before removing software protections for manual
travel, provide the external-box circuit and independent stopping architecture:
emergency stop, terminal travel limits, drive/brake interruption, and behavior on
key/hold release, controller failure, lost UART, and loss of field power. Specifically
identify which protections remain effective with the keyed override engaged.
A healthy monitor level alone cannot establish those properties, and a queued
serial STOP is not an independent means of stopping.

The owner has now specified a configurable 9600 diagnostic baud default and normal
D0-D4 actions: floor 1, light toggle, floor 3, floor 2, STOP. The host RF model adopts
that map; assembled decoder jumper matching and physical capture are still unverified.
TX_ID remains a reusable learned slot plus
association epoch, not a permanent transmitter identity. Existing EM01 operation
encoding is present in `core/Em01.cpp`; this review does not claim new protocol
qualification or invent missing physical direction/drive-configuration evidence.

The subsequent hardware evidence review changed documentation only and exported
read-only evidence through Konnect. No operational-profile, RUN path, protection
bypass, GPIO, or KiCad design change was made. Fresh checks passed: 227 connectivity
assertions against the new export and prior IPC evidence, contract generation/check,
and 2866 host-test assertions. ERC reported zero errors at error severity; PCB DRC
still reports 325 unrouted connections and 181 warnings. None verifies the external
box, physical field polarity, drive stopping, or brake operation.

The subsequent software update adds target-bound
manual switch-release STOP and input-intent reporting without enabling RUN or
removing the upload/profile inhibit. Its verification is separate from the hardware
review recorded above.

Nothing in this software work energizes mains, uploads a controller, enables a
drive or requests real motion. All motion tests use host-generated samples.
Driver transcripts are simulated; an ESP32 build does not verify supply rails,
SPI timing, counter phase, VFD electrical levels, RF timing or brake operation.
Deployment remains blocked until both this handoff and the software checklist
are closed under an appropriate supervised qualification procedure.
