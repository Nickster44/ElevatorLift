# Hardware Dependencies For Software Integration

2026-09-11; contract v1, `rev-a-review-inhibited`. **No KiCad files were modified.**
Source: 2026-09-10 integration review, exported netlist and current source.
`firmware/tests/contract.mjs` verifies the confirmed MCU nets in that export.

| ID | Required decision / correction | Current software behavior | Evidence needed to close |
| --- | --- | --- | --- |
| HW-01 | Reassign LIMIT_UP, LIMIT_DOWN and SERVICE_KEY away from GPIO35/36/37, unavailable on N16R8 | No GPIO definitions or reads; values null, all motion inhibited | Reviewed new nets, exported netlist, updated contract/version and input truth-table tests; software does not choose replacements |
| HW-02 | Separate U24 TXS0104E OE from U25 RUN-permission control currently on GPIO42 | GPIO42 always LOW; no VFD UART initialization/transmission; STOP endpoint reports delivery unavailable | Independent UART-service and hardwired-run-permission design, reset/brownout/watchdog/power-sequencing truth table, bench evidence with drive energy isolated |
| HW-03 | Confirm safety/limit/home/service input polarity, filtering, fail-open behavior and independent final-limit authority | Confirmed safety GPIO1/HOME GPIO2 are sampled; unavailable inputs never treated healthy | Wiring trace, connector/pin labels, inactive/active/disconnected samples, hardwired safety review |
| HW-04 | Confirm encoder polarity/scale and field conditioning; exported U32 LS7366R has 4 MHz local clock | SPI mode0, 1 MHz, x4/32-bit/free running; position stays invalid after reset | A/B phase, maximum rate, count-per-travel, filter latency, reset/power-loss behavior; safe no-motion encoder simulation |
| HW-05 | RF JP41/JP42 baud straps and physical five-button mapping; verify D40 package and learn circuit | Slot/epoch and learn logic isolated; no guessed serial baud or output-to-button mapping | Strap state, 8-bit TX_ID framing, D0-D4 mapping, MODE_IND waveform, learn/erase timing, LATCH behavior |
| HW-06 | Verify 12 V load budget, JP40 onboard/external supply selection and light output wiring; review D41/D42 footprint mismatch | Light set-state GPIO41; auxiliary GPIO3 held LOW; no physical load feedback claimed | RAC10 baseline/peak load, upgraded-light external-supply selection, MOSFET protection/polarity and bootstrap GPIO3 review |
| HW-07 | Confirm native USB recovery power path | USB19/20 reserved; build uses native USB CDC | Current export USB VBUS feeds 5 V but not the 3.3 V logic rail; define approved low-voltage bench power procedure before connecting a board |
| HW-08 | Verify RV-3028 backup source and configuration | Read-only UNIX time, POR invalidation, nullable timestamps | J12 backup power, permitted trickle-charge policy, power-loss retention test; no automatic EEPROM/charger writes |
| HW-09 | Confirm PM004 exact revision/mode defaults and SIO2/SIO3 strapping | Verify ID/mode registers, zero-latency SPI only; aligned 16-bit-word accesses | Local V1.34 sheet and assembled device behavior, unused-QPI-pin recommendations vs exported pulls, interrupted writes and retained memory |
| HW-10 | Routing/ERC/DRC, mechanical dimensions, connector access and antenna | Nothing marked manufacturing/deployment ready | Resolve review violations/unrouted nets and physically measure cabinet/90 mm outline/mounting/clearance |

The field-used TXS0104E translation topology is not being rejected or silently
rewired. HW-02 concerns the *additional coupling to run permission*, not merely
UART logic levels. Preserve the independent hardwired safety chain in every option.
No firmware workaround may assert RUN just to obtain telemetry or parameters.

## Physical Verification Boundary

Nothing in this software work energizes mains, uploads a controller, enables a
drive or requests real motion. All motion tests use host-generated samples.
Driver transcripts are simulated; an ESP32 build does not verify supply rails,
SPI timing, counter phase, VFD electrical levels, RF timing or brake operation.
Deployment remains blocked until both this handoff and the software checklist
are closed under an appropriate supervised qualification procedure.
