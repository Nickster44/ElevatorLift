# Old System Review

Source reviewed: `_old_resources/old_code.ino.txt`

## Platform

- Particle Xenon.
- `SYSTEM_MODE(SEMI_AUTOMATIC)`.
- USB debug serial at 115200 baud.
- VFD serial on `Serial1` at 9600 baud.
- Relay/input serial on `Serial2` at 9600 baud.
- EEPROM used for floor positions and speed-dependent stopping thresholds.

## Original Pin Use

| Name in code | Pin | Direction | Meaning |
| --- | --- | --- | --- |
| `Top` | `A3` | Input | Top button; floor 3 request or jog up |
| `Right` | `A4` | Input | Right button; floor 4 request or relay toggle |
| `Bottom` | `A5` | Input | Bottom button; jog down or relay toggle |
| `Left` | `A2` | Input | Left button; floor 2 request |
| `Center` | `D2` | Input | Stop, set position, or hold for homing |
| `RFLine` | `D3` | Output | RF line driven from serial input bit |
| `STEPUPpin` | `D6` | Interrupt input/output configured | Rising edge increments `currentPos` |
| `STEPDOWNpin` | `D8` | Interrupt input/output configured | Rising edge decrements `currentPos` |
| Safety | `A1` | Input pullup | Low means safety loop closed; high means broken |

## Relay/Input Serial Data

The old firmware expects `Serial2` input lines ending in newline. This serial channel went to an Arduino Nano accessory board in the disconnect box. The Nano acted as an I/O extender and relay-control board for the main Particle Xenon controller. The main controller maps received characters into `dataArray`:

| Index | Name | Use |
| --- | --- | --- |
| 0 | `S1pos` | Speed factor bit 0 |
| 1 | `S2pos` | Speed factor bit 1 |
| 2 | `S3pos` | Speed factor bit 2 |
| 3 | `PROGpos` | Program mode |
| 4 | `RFpos` | RF line output |
| 5 | `HOMEpos` | Home input |
| 6-9 | `RELAY1pos`..`RELAY4pos` | Relay/input status positions |

Relay commands sent from the Particle controller to the Nano are formatted as `(bbbb)\n`, where the four bits represent the relay output bitmap. The relays were 120 VAC, 10 A mechanical relays used for solenoids, gate/interlock control, and lift lighting.

The new design may not need the same relay bank. The current known retained load is the lift light. If gate interlocks are added later, they should be treated as part of the safety/control design rather than copied as general-purpose relays.

## Motion Model

The sketch keeps `currentPos` as a signed pulse count. Up pulses increment the count and down pulses decrement it.

When moving to a saved floor:

1. `gotoSavedPos()` sets `nextPos`, starts a 50 ms timer, sets `run = true`, and records which floor relay should turn on after stop.
2. `gotoNextPos()` calculates total distance and a stop threshold.
3. It sends repeated VFD run commands in the selected direction.
4. When the current position reaches the stopping threshold or the remaining distance is less than the threshold, it calls `callStop()`.
5. `stopFunc()` sends `(3)84` every 50 ms until it sees a stop acknowledgment or reaches 20 tries.

The old program-mode exit also performed a practical stop-distance calibration. After floor positions were set, the controller ran toward whichever end of travel was farther away, allowed the lift to reach normal speed for a few seconds, commanded a stop, measured the difference between the stop-command position and final stopped position, and saved that measured deceleration travel as the calibration value. This worked well enough that the redesign should keep the same measured stop-distance concept, but store the result in MRAM with validation metadata.

## VFD Initialization Seen In Setup

The sketch writes these commands at boot:

| Command | Likely meaning from EM01 protocol |
| --- | --- |
| `(4030010):9` | Set parameter 03 acceleration to 0010 |
| `(4040010)::` | Set parameter 04 deceleration to 0010 |
| `(4120001):9` | Set parameter 12 timeout protocol to 0001 |
| `(4101200):9` | Set parameter 10 max frequency to 120.0 Hz |
| `(0000)\n` to `Serial2` | Clear relay outputs |

## Credible Failure Contributors

The first-floor overshoot was not conclusively identifiable from source alone, but the following are worth treating as high-priority redesign targets:

- `currentPos` is modified in interrupt handlers but is not declared `volatile` and is read by timer/main-loop code without a snapshot function.
- The software position count is lost on reset or brownout.
- The stopping decision is threshold-based and depends on a repeated serial stop command being accepted by the VFD.
- There is no independent verification that VFD output frequency actually reached zero before the state changes.
- Stop acknowledgment parsing searches for a substring pattern in a growing serial buffer rather than parsing complete frames.
- The program has many shared flags (`run`, `calculate`, `calibrate`, `gotoHome`, `programModeRunSpeed`, `stringComplete2`) updated by different execution contexts.
- Blocking `delay(300)` calls in button handlers slow response to input changes.
- The homing logic checks `dataArray[HOMEpos] == 0`; because the array defaults to zero, stale or missing input can look like "home reached."
- The expression `currRelayValue & 0x02 == 0x02` is parsed as `currRelayValue & (0x02 == 0x02)`, so it checks bit 0 instead of bit 1.
- EEPROM write/read validation only checks a `version` byte and does not include CRC, generation counters, or redundant records.

## Redesign Implications

- Treat MCU motion commands as supervisory. Safety loop, final limits, VFD enable, braking, and emergency stop should have hardwired behavior.
- Use an external quadrature counter with a consistent sample/clear/read sequence.
- Store position snapshots and event logs in MRAM with CRC and monotonically increasing sequence numbers.
- Parse VFD frames with a bounded state machine and timeout handling.
- Require explicit state transitions: idle, precheck, moving, stopping, stopped, homing, fault.
- Add fault latching for missing position movement, unexpected movement, VFD alarm, stop timeout, safety loop open, and limit violations.
