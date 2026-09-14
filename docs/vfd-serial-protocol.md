# EM01 VFD Serial Protocol Notes

Source: `_old_resources/EM01 Manual User_EN-V1.02.pdf`, pages 26-30.

The old controller used the EM01 ASCII serial protocol at 9600 baud. Messages are framed with `(` and `)` and end with two checksum characters.

## Checksum

The checksum is the 8-bit sum of all characters in the framed payload, including `(` and `)`, before the checksum characters are appended.

The high and low nibbles are encoded by adding ASCII `0`, not by normal hexadecimal `A-F` conversion. This is why some valid checksum characters are `:`, `;`, `<`, `=`, `>`, or `?`.

Example:

```text
Payload:  (10100)
Sum:      0x143
Low byte: 0x43
Command:  (10100)43
```

## Commands

| Operation | Payload | Example | Expected answer |
| --- | --- | --- | --- |
| Run forward | `(1FFFF)` | `(10100)43` for 10.0 Hz | `(1)82` |
| Run reverse | `(2FFFF)` | `(21000)44` for 100.0 Hz | `(2)83` |
| Stop | `(3)` | `(3)84` | `(3)84` |
| Monitor | `(0)` | `(0)81` | `(0VVVFFFFTTTCCCS)xx` |
| Set parameter | `(4NNVVVV)` | `(4130001)::` | `(4)85` |
| Get parameter | `(5NN)` | `(500)>6` | `(5VVVV)xx` |

`FFFF` is a 4-character ASCII frequency value in tenths of Hz. For example, `0100` means 10.0 Hz.

## Monitor Response

The monitor response format is:

```text
(0VVVFFFFTTTCCCS)xx
```

| Field | Meaning |
| --- | --- |
| `VVV` | DC bus voltage |
| `FFFF` | Output frequency |
| `TTT` | Heat sink temperature |
| `CCC` | Average current |
| `S` | Status |

Status values listed in the manual:

| Value | Meaning |
| --- | --- |
| `0` | Inverter stopped |
| `1` | Running forward |
| `2` | Running reverse |
| `3` | Overvoltage alarm |
| `4` | Undervoltage alarm |
| `5` | Temperature alarm |
| `6` | Current alarm |
| `7` | No serial message received |
| `8` | Overload alarm |

## Parameters Used By Old Firmware

| Parameter | Name | Old command | Meaning |
| --- | --- | --- | --- |
| 03 | `ACC` | `(4030010):9` | Acceleration, tenths of second |
| 04 | `DEC` | `(4040010)::` | Deceleration, tenths of second |
| 10 | `FMAX` | `(4101200):9` | Max frequency, tenths of Hz |
| 12 | `TIME` | `(4120001):9` | Serial protocol timeout, tenths of second |
| 13 | `RELE` | `(4130001)::` / `(4130000):9` | Expansion relay on/off |

## Full Parameter Access Plan

The EM01 protocol supports generic set and get commands, so the controller should expose a complete VFD parameter page in the WebUI. The WebUI should read all parameters at startup/service-page load, cache the most recent values, and allow writes only through range-checked controls.

Known setting parameters from the manual:

| Parameter | Name | Min | Max | Default | Meaning |
| --- | --- | ---: | ---: | ---: | --- |
| 00 | `OVERV` | 0100 | 0400 | 0390 | Overvoltage alarm value |
| 01 | `UNDER` | 0100 | 0400 | 0200 | Undervoltage alarm value |
| 02 | `TEMPALL` | 0020 | 0100 | 0080 | Temperature alarm value |
| 03 | `ACC` | 0001 | 0599 | 0050 | Acceleration, tenths of second |
| 04 | `DEC` | 0001 | 0599 | 0050 | Deceleration, tenths of second |
| 05 | `BOOST` | 0000 | 0090 | 0008 | Voltage boost setting |
| 06 | `IN` | 0005 | 0100 | 0036 | Nominal current, tenths of ampere |
| 07 | `TSOVRA` | 0000 | 0060 | 0000 | Overload time, seconds |
| 08 | `PSOVRA` | 0100 | 0150 | 0150 | Overload percent |
| 09 | `VMAX` | 0250 | 2000 | 0500 | Frequency at rated input voltage, tenths of Hz |
| 10 | `FMAX` | 0000 | 2000 | 1000 | Max frequency, tenths of Hz |
| 11 | `FMIN` | 0000 | 2000 | 0000 | Min frequency, tenths of Hz |
| 12 | `TIME` | 0000 | 0599 | 0000 | Serial protocol timeout, tenths of second |
| 13 | `RELE` | 0000 | 0001 | 0000 | Expansion relay state |

Read-only expansion values from the manual:

| Parameter | Name | Meaning |
| --- | --- | --- |
| 13 | `INPUT` | Input and relay status when read as an expansion-card input value |
| 14 | `POT1` | Analog input 1, 0000-0255 |
| 15 | `POT2` | Analog input 2, 0000-0255 |
| 16 | `DAC` | Analog output value, 0000-0255 |

Parameter 13 appears in both the writeable setting list and read-only expansion-card status list in the manual. The firmware should keep this ambiguity visible in the metadata and validate behavior against the actual VFD.

## WebUI Guardrails

- Separate user settings from installer/advanced VFD parameters.
- Show units and scaling instead of raw four-digit values where possible.
- Require the lift to be stopped before writing drive parameters.
- Read back a parameter after every write and log the result.
- Keep a cached copy of VFD parameters in nonvolatile storage.
- Provide export/import for configuration backup.
- Treat current/overload parameters as a calibrated load-limit feature, not a certified weighing system.

## Design Notes

- Keep periodic command refresh separate from bounded retries of one unanswered transaction.
- Page 26 requires continuous orders or requests. Parameter 12 `TIME` is the drive's
  missing-communication watchdog in tenths of a second; zero disables that function.
  It is not the controller's reply timeout. Pages 27-28 describe repeating an
  unanswered command after 100 ms; the current reply deadline is a conservative 150 ms.
- The inhibited target refreshes STOP approximately every 300 ms, interleaving monitor
  and parameter reads in 100 ms traffic slots. TIME is read first, not automatically
  written. An in-flight transaction is allowed its bounded retries; an explicit STOP
  preempts it. Periodic refresh does not erase still-fresh monitor evidence.
- `core/VfdTiming.h` centralizes 150 ms reply timeout, three attempts, 500 ms monitor
  freshness, 100 ms traffic slots and 300 ms STOP refresh. These are scheduled targets,
  not measured real-time guarantees. Worst-case UART/network/storage delays require bench tests.
- Exhausted retries latch communication failure. The target continues STOP-only refresh;
  it does not automatically resume reads or RUN, reset faults, or imply physical stopping.
- The isolated parameter-write workflow now accepts TIME=0010 (1 second) only: the
  existing 1-second ceiling is retained and the too-short 0.2-0.9-second choices removed.
  This development policy is not a universal EM01 requirement or hardware qualification.
  No TIME setting is changed automatically; target writes remain inhibited.
- Parse complete frames and verify checksums before acting on responses.
- Treat VFD alarm status as a latched controller fault.
- Serial STOP is not independent stopping authority. External hardwired circuitry owns that authority; GPIO42 controls UART translation only. The current mapping has no MCU RUN/brake output.
