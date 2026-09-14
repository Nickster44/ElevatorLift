# Controller Software

**Buildable, hardware-inhibited development profile. Not deployable. No uploads.**

The current profile targets ESP32-S3-WROOM-1U-N16R8: 16 MB QIO flash, 8 MB octal
PSRAM and native USB CDC. `interface-contract.json` is the versioned wiring
contract; `include/PinMap.h` is generated, not independently edited. The netlist
contract test checks confirmed assignments against the supplied review export.
Contract v2 adopts the 2026-09-13 finalized mapping: limits GPIO43/44, key GPIO3,
communications-only OE GPIO42. GPIO35-37 have no usable definitions. AUX is removed.
UART0 debug/IDF logging is disabled and UART0 detached before configuring input pins;
native USB remains the application console. ROM/bootloader output predates setup
and cannot be suppressed by this application; R62 and boot-waveform testing remain gates.
GPIO42 starts LOW, then enables only the TXS0104E after UART1 initialization.
Only STOP, monitor and parameter READ commands are target-bound; no RUN or WRITE.

Upper/lower-limit and service-key active-low readings require 100 ms stability with sample gaps <=25 ms. A transition,
invalid level or missing sample invalidates readiness immediately. Pulled-up single
inputs cannot distinguish open contacts from broken wires/absent field power.
`FieldContinuityQualified=false` therefore keeps input qualification false, in
addition to the unchanged global motion/upload inhibit. No setting or web override
can remove either gate. The external hardwired safety loop is the required independent motion authority;
its installed topology and drive/brake behavior remain unverified.

## Implementation Boundaries

Manual switch release now requests EM01 STOP independently of motion readiness.
`ManualStop.h` samples raw active-low key/hold/direction/SAFETY_MON before UART,
HTTP, SPI and persistence work each loop. Key/hold release, direction loss/change,
both directions, safety loss, invalid input or a sampling gap requests STOP without
waiting for the 100 ms qualification filter. Repeated pending STOP requests coalesce
so bounded protocol retries still expire. UART transmission remains subject to TX
buffer availability; this is not a verified real-time or physical-stop guarantee.
Key takeover requests STOP and rejects WebUI floor calls. Manual RUN remains inhibited.

The isolated normal RF map is D0 floor 1, D1 light toggle, D2 floor 3, D3 floor 2,
D4 STOP. Diagnostic baud defaults to 9600 and is configurable in the model; the
decoder jumper configuration must match this UART rate, not remote RF timing.
Target RF capture/programming movement and the operational v3 profile remain incomplete.

| Layer | Implementation | Boundary |
| --- | --- | --- |
| `core/Supervisor.*` | Unknown startup, three floors, top HOME edge, measured stop, fault latch, service release, limits/progress/overshoot, guards | Host simulation; thresholds uncommissioned |
| `core/Em01.*`, `DiagnosticVfd.h` | UART1 diagnostic STOP, monitor and readback, bounded retries/freshness | No RUN/WRITE; `DriveScheduler` remains isolated; late same-type reply ambiguity requires bench review |
| `core/FieldInputs.h` | Debounced active-low electrical readings, freshness and separate continuity qualification | No continuity/power-loss claim; target qualification remains false |
| `core/Devices.*` | LS7366R mode/readback and wrap tracking; PM004 word-address SPI; RTC coherent UNIX read; CRC journals | Target adapters build; no physical verification |
| `core/Configuration.*` | Versioned settings/floors/names/calibration/epoch serialization | Read on target; commissioning/write workflow not exposed |
| `core/ControllerSession.h`, `SafetyLedger.h` | Durable motion-intent/fault barriers, configuration, homing/program exit and calibration coordination | Host integrated; operational target binding pending |
| `core/ParameterJobs.h` | Guarded serialized jobs, readback/cache persistence, calibration invalidation | Isolated; no target UART binding |
| `core/Rf.h` | Release-to-rearm, slot/epoch association, five-command mapping, learn/erase confirmation models | Capture UART, qualified timing and persistent registry still to integrate |
| `main.cpp` | Inhibited diagnostics, real counter/storage/RTC adapters, light set-state, AP/station, bounded HTTP, LittleFS | No motion, homing, RF learning, settings writes, parameter writes, reboot or OTA |

The isolated session still starts single-record calibration on program exit.
Explicit WebUI start and direction-specific records are pending; see
[motion requirements](../docs/motion-control-and-calibration.md).

Diagnostic traffic uses 100 ms slots with periodic STOP refresh due every 300 ms,
interleaving monitor/read transactions and reading TIME first. Parameter 13 is skipped
because readback is unresolved. Reads use raw protocol units and
timestamped cache values, not persisted commissioning settings. Three failed attempts
poison the session until reset; STOP-only refresh continues, automatic reconnect is not.
API STOP returns queued status, never a stopped claim. Telemetry requires a validated
monitor younger than 500 ms; `monitorStopped` is only drive evidence, not physical
stopping. Public `stoppedConfirmed` remains false in this unqualified profile.

`VfdTiming.h` separates periodic refresh from 150 ms reply retries and the drive's
TIME watchdog. Read-back watchdog state is exposed/logged as unknown, disabled,
too-short, outside-policy or within-software-policy, never as physical qualification.
The isolated installer write workflow now permits TIME=0010 only (1 second);
shorter settings conflict with the retry/refresh margin. No drive setting is written
automatically and no target parameter-write/RUN capability has been enabled.

## Build And Test

From repository root:

```powershell
node firmware/scripts/generate-contract.mjs --check
node firmware/tests/contract.mjs
./firmware/tests/run.ps1
python firmware/tests/test_build_guard.py
```

From `firmware`: `pio run`. The environment name is retained for existing tooling;
the board definition is `boards/elevator-n16r8.json`, not the N8 devkit definition.
PlatformIO upload/uploadfs targets deliberately fail. Do not remove that gate to
try hardware. See [hardware dependencies](../docs/hardware-dependency-handoff.md).

From `webapp`: `npm ci`, `npm run build:embedded`, then
`node scripts/stage-firmware.mjs`. From `firmware`, `pio run -t buildfs` builds the
filesystem image without uploading. Full update constraints are in
[delivery notes](../webapp/docs/embedded-delivery.md).

## Network And Authentication

Configure unique AP credentials and a random API token of at least 16 characters
in ignored `include/Secrets.h`. A blank/short token **disables all writes**.
Only the `X-Lift-Api-Token` header is accepted; never a URL query token. The UI
holds the entered token in memory, not persistent browser storage. There is no
TLS on the embedded listener: use a trusted, isolated LAN/AP and an authenticated
local hub; do not expose this API to the Internet.

The AP starts for setup, closes after station connection and returns within the
5-second network check interval on loss. Saving network credentials writes NVS
without reconnecting or restarting. Restart is explicitly required; remote reboot
is disabled in this profile. Credentials are never returned by the API.

## Storage

PM004MNIATR: 524,288 bytes; two-byte aligned accesses and 18-bit **word** addresses.
Startup verifies manufacturer bytes and mode registers; unknown latency or
protection is rejected, not reset blindly. Writes issue WREN, WRITE, WRDI, READ
and compare. No SPI-flash erase/page-program assumptions are used.

- Bytes 0-1151: two historical position/state records, saved once per second.
- Bytes 2048-3199: two configuration records.
- Bytes 4096-5247: two safety-ledger records (latched fault / unfinished motion).
- Bytes 6144-7295: reserved for two parameter-cache records; not target-bound.
- Bytes 8192-450559: 768 durable event slots, 576 bytes each, up to 544 payload bytes.
- Remaining capacity reserved; configurable retention and denser binary event format remain open.

Records use explicit little-endian fields, schema version, length, sequence,
CRC32 and a last-written commit marker. A torn new record cannot replace the
previous committed record. The safety ledger records fault transitions immediately;
its portable motion-intent barrier must be committed before any future RUN.
Fault snapshots also latch a prior-boot fault, but position
snapshots never establish incremental encoder continuity. Boot requires homing
before any future normal motion. RTC time is nullable; no NTP/build-time clock is
fabricated. RTC setting/backup configuration is not implemented.

See [software checklist](../docs/software-integration-checklist.md) for current
verification and the remaining integration work. Passing host tests is not lift
qualification or proof of hard-real-time behavior.
