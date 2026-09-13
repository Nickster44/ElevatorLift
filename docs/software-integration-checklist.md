# Software Integration Checklist

2026-09-11 milestone; reproducibility rechecked 2026-09-12.
**Partial software integration, not deployment qualification.**
The starting review reproduced defects in baseline `1c37a65`. Its 37 host checks
and eight WebUI observations are evidence of defects, not passing safety tests.
The original review directory and hardware files remain untouched.

## Milestone 1: Interface And Build

- [x] Versioned JSON contract with separate confirmed pins and unresolved decisions.
- [x] Generated pin header; netlist/GPIO conflict regression test.
- [x] Explicit N16R8 board, QIO flash/OPI PSRAM, native USB and 16 MB partitions.
- [x] Upload/uploadfs guard; no build/upload command connects to hardware.
- [ ] Resolve HW-01/HW-02, qualify pin/power/reset behavior before operational profile.

## Milestone 2: Motion And Devices

- [x] Portable supervisor: unknown startup, three floors, upward HOME edge reference,
  directional limits, progress/wrong-direction/overshoot/idle-motion faults,
  STOP priority, fault latching, service hold release/timeout, guarded settings/reset.
- [x] Measured calibration chooses farther end, requires sustained configured speed,
  records STOP position, waits for fresh stopped evidence plus encoder stability,
  checks distance and invalidates on configuration changes. No PID.
- [x] EM01 bounded frames/checksums, three attempts, timeout poisoning, parameter
  readback, telemetry timestamps and STOP acknowledgement distinct from stopped.
- [x] LS7366R configuration/readback, latched count read, modular 32-bit deltas and
  mode/PLS health checks. No separate up/down pulse ISRs.
- [x] PM004MNIATR word-address/2-byte transfers, ID/mode checks and write verification.
- [x] Versioned CRC/sequence/commit-marker redundant records, configuration codec,
  target snapshot/event journals; no position validity restored from storage.
- [x] RTC POR validity and double-read consistency, nullable wall time.
- [x] Isolated RF slot/epoch, mapping/arbitration, boot-held suppression and learn pulse.
- [x] Isolated controller session: durable pre-RUN intent, configuration/calibration
  persistence, fault ledger and guarded reset/program-exit coordination.
- [x] Isolated parameter jobs: service guards, invalidate calibration before writes,
  verified readback, durable cache and configuration updates.
- [x] Isolated RF erase confirmation model; timing remains unqualified.
- [ ] Operational UART scheduler/parameter jobs/cache, counter-reference handshake,
  program exit workflow and calibration persistence wired into target APIs.
- [x] Immediate durable fault-transition ledger and conservative boot replay.
- [ ] Physical power-interruption qualification of the ledger and pre-RUN barrier.
- [ ] Full RF UART capture, baud/mapping confirmation, MODE_IND/erase, persistent registry.
- [ ] RTC setting/backup policy, dense binary log format, retention/paged export.
- [ ] Qualified supervisor scheduling, watchdog and worst-case execution/network latency.

## Milestone 3: API, UI And Assets

- [x] Replaced old live/demo UI behavior with connecting/disconnected/stale states.
- [x] Runtime validation, authoritative floor identity/capabilities, null telemetry,
  no local-only STOP, no fabricated setup/calibration/pairing success.
- [x] Header authentication fails closed when token missing/short; strict move/light/
  network form validation, idempotent light command and real network save results.
- [x] Bounded HTTP parser and per-loop work, same-origin asset streaming and cache headers.
- [x] Static bundle staging, SHA256 manifest, LittleFS image build and OTA-sized slots.
- [ ] Guarded firmware/UI setup, calibration, drive editing, RF, backup/restore and reboot.
- [x] Runtime SHA256 bundle manifest/version gate; corrupt assets are not served.
- [ ] Atomic app/assets update procedure.
- [ ] Target slow-client/load testing; HTTP/TCP bounds are not proof of motion deadlines.

## Executed Tests

| Test | Result | Scope |
| --- | --- | --- |
| `npm ci` | Pass | Clean dependency install; audit warnings remain below |
| `npx playwright install --with-deps chromium` | Pass | Browser binaries matching locked Playwright 1.58.2 |
| `node firmware/scripts/generate-contract.mjs --check` | Pass | Generated pins match JSON |
| `node firmware/tests/contract.mjs` | Pass | Exported nets, unavailable pins, inhibit and N16R8 |
| `./firmware/tests/run.ps1` | Pass, 2619 assertions | Portable C++ simulation, including 581 torn-write cut points |
| `pio run` | Pass | ESP32-S3 cross-build, not target execution |
| `npm run check` / `npm run lint` | Pass | TypeScript / static checking |
| `npm test` | Pass, 9 tests | Runtime API validation, failures and SSR |
| `npm run build:embedded` | Pass | Approximately 234 KB raw / 71 KB gzip; exact sizes in generated manifest |
| Asset stage + `pio run -t buildfs` | Pass | LittleFS image, no upload |
| `npm run test:browser` with locked Playwright Chromium | Pass, 1440x1000 and 390x844 | Mocked-API disconnect, real STOP transport/failure reporting, malformed status, reconnect, six views and no horizontal overflow; not target or motion verification |
| `python firmware/tests/test_build_guard.py` | Pass | Upload guards exercised with a fake build environment, no hardware access |

The software CI workflow is defined but has not run remotely; nothing was pushed.
It now installs Chromium and runs `npm run test:browser` after the WebUI build.
The named command builds and owns a localhost-only preview, waits for readiness,
and closes it after the test. Additional local failure-injection checks verified
missing-browser failure, signal interruption, occupied-port rejection, nonzero
failure exits, and no remaining listener on port 5178. The old ad hoc browser-pass
claim is superseded by this locked-dependency run; no external module or Edge
override was used. See the clean-checkout sequence in [webapp README](../webapp/README.md).

All listed validation commands were rerun locally on Windows on 2026-09-12.
Ubuntu execution awaits CI. The installer requires network access; Playwright
1.63.0's download timed out locally, while the committed 1.58.2 pin installed and
passed normally. Browser coverage is Chromium only. `npm ci` reports 21 dependency
audit findings (2 low, 2 moderate, 16 high, 1 critical); dependency remediation is
separate work, not a reason to bypass tests or enable deployment.

Motion tests cover unknown reset, HOME behavior, stale inputs/communication,
missing sensors, storage failure, wrong direction, no progress, overshoot,
conflicting requests, latched faults and service release. Protocol tests check
documented vectors, bad framing/checksums, timeouts/late replies, monitor and
write/readback. Storage tests exercise all byte interruption points, corrupt
records, counter wrap, MRAM address bounds and invalid/missing RTC.

## Remaining Physical Work

All hardware behavior remains physically unverified. See the numbered
[hardware handoff](hardware-dependency-handoff.md). No KiCad edits, mains,
drive enable, target upload or real motion occurred. No Git push was performed.
