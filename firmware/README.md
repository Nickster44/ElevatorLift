# Controller Software

**Buildable, hardware-inhibited development profile. Not deployable. No uploads.**

The current profile targets ESP32-S3-WROOM-1U-N16R8: 16 MB QIO flash, 8 MB octal
PSRAM and native USB CDC. `interface-contract.json` is the versioned wiring
contract; `include/PinMap.h` is generated, not independently edited. The netlist
contract test checks confirmed assignments against the supplied review export.
GPIO35-37 have no usable definitions. GPIO42 is held LOW and no VFD UART is
initialized: translator OE and RUN permission are coupled in the current design.

## Implementation Boundaries

| Layer | Implementation | Boundary |
| --- | --- | --- |
| `core/Supervisor.*` | Unknown startup, three floors, top HOME edge, measured stop, fault latch, service release, limits/progress/overshoot, guards | Host simulation; thresholds uncommissioned |
| `core/Em01.*`, `DriveScheduler.h` | Bounded framed transactions, checksum, monitor, retries, parameter readback, STOP priority | Not connected to target UART; late same-type reply ambiguity requires bench review |
| `core/Devices.*` | LS7366R mode/readback and wrap tracking; PM004 word-address SPI; RTC coherent UNIX read; CRC journals | Target adapters build; no physical verification |
| `core/Configuration.*` | Versioned settings/floors/names/calibration/epoch serialization | Read on target; commissioning/write workflow not exposed |
| `core/ControllerSession.h`, `SafetyLedger.h` | Durable motion-intent/fault barriers, configuration, homing/program exit and calibration coordination | Host integrated; operational target binding pending |
| `core/ParameterJobs.h` | Guarded serialized jobs, readback/cache persistence, calibration invalidation | Isolated; no target UART binding |
| `core/Rf.h` | Release-to-rearm, slot/epoch association, five-command mapping, learn/erase confirmation models | Capture UART, qualified timing and persistent registry still to integrate |
| `main.cpp` | Inhibited diagnostics, real counter/storage/RTC adapters, light set-state, AP/station, bounded HTTP, LittleFS | No motion, homing, RF learning, settings writes, parameter writes, reboot or OTA |

The old interrupt-counter, fabricated floor defaults, substring ACK, periodic
NVS-position and RAM-log scaffolds have been removed. Git history retains them.

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
