# VFD Communication Heartbeat Follow-up

## Evidence And Changes

EM01 Manual User_EN-V1.02, page 26 requires continuous orders or requests;
page 29 defines parameter 12 TIME in tenths of seconds, 0 disabling the watchdog.
Pages 27-28 describe repeating unanswered commands after 100 ms. This is separate
from periodically refreshing commands after successful replies.

The inhibited v2 UART1 target now refreshes STOP nominally every 300 ms, with
monitor/read traffic in 100 ms slots. Individual replies retain a 150 ms timeout
and three attempts. An in-flight transaction can delay periodic refresh; explicit
STOP still preempts it. These are software scheduling values, not measured deadlines.
Periodic STOP does not erase fresh monitor evidence. New explicit STOP intent does.

Failure remains latched: monitor/read traffic stops, bounded STOP transactions
continue, and later acknowledgements do not clear the failed session. No RUN is
resumed. TIME is read first and exposed with age and unknown/disabled/too-short/
outside-policy/within-software-policy states. Timeout and watchdog state changes
are logged. STOP transmission, acknowledgement and stopped evidence remain distinct.

The isolated operational scheduler uses 100 ms traffic slots. The isolated TIME
write workflow accepts only 1.0 s under the current development policy, replacing
the previous 0.2-1.0 s range. This is not the drive's supported range or a bench
qualification. No drive setting is automatically changed; target writes and RUN
remain unavailable, and the existing upload/motion inhibit stays intact.

## Files Changed In This Follow-up

- `firmware/src/core/VfdTiming.h` (new)
- `firmware/src/core/Em01.h`, `Em01.cpp`, `DiagnosticVfd.h`, `DriveScheduler.h`, `ParameterJobs.h`
- `firmware/src/main.cpp`, `firmware/tests/core_tests.cpp`
- `webapp/lib/controller.ts`, `webapp/app/page.tsx`
- `webapp/tests/controller.test.mjs`, `webapp/tests/browser.mjs`
- `README.md`, `firmware/README.md`, `webapp/README.md`
- `docs/vfd-serial-protocol.md`, `webapp/docs/api-contract.md`
- `docs/software-integration-checklist.md`, `docs/webapp-embedded-delivery.md`
- `webapp/docs/embedded-delivery.md`, this report

Other working-tree changes predate this follow-up or belong to concurrent work.
No KiCad files or mappings were edited by this follow-up.

## Verification On Windows

All commands passed locally on 2026-09-13:

- Webapp: `npm ci`, `npm run check`, `npm run lint`, `npm test` (13 tests).
- Webapp: `npx playwright install --with-deps chromium`, `npm run test:browser`.
  Mocked API, 1440x1000 and 390x844: offline behavior, transported STOP failures,
  malformed status rejection, reconnect, six views, watchdog display, no overflow.
- Webapp: `npm run build:embedded`, `node scripts/stage-firmware.mjs`.
  Bundle: 237,468 raw / 71,835 gzip bytes; six assets staged.
- Firmware: `pio run` (48,840 bytes RAM / 847,749 bytes flash), `pio run -t buildfs`.
- Root: `node firmware/scripts/generate-contract.mjs --check`,
  `node firmware/tests/contract.mjs`, `./firmware/tests/run.ps1` (3,953 assertions),
  `python firmware/tests/test_build_guard.py` (fake environment only).

Host tests cover successful repeated STOP/monitor/read traffic, TIME classifications
and freshness, retry exhaustion, continued STOP-only traffic, latched failure,
timer wrap, and TIME write rejection/readback in the isolated workflow.

## Remaining Dependencies

Measure worst-case UART intervals with HTTP, storage and logging load; confirm the
actual TIME value, watchdog alarm/stop configuration and drive firmware behavior.
Confirm optional parameter availability, lost-wire behavior, STOP response and
actual deceleration. A watchdog setting or acknowledgement is not proof of physical
stopping or shaft holding. No physical controller was contacted, uploaded, enabled
or moved. Operational v3 and target RUN remain unfinished, not qualified by this work.

Browser evidence uses mocked APIs, firmware builds are cross-builds, and host tests
are simulations. Ubuntu CI has not run remotely. Dependency audit still reports
21 findings (2 low, 2 moderate, 16 high, 1 critical); remediation remains separate.
Nothing was committed or pushed.
