# Manual Switch Release To STOP

This is a partial implementation milestone, not the requested operational v3 release.
Contract v2, finalized GPIO mapping, SAFETY_MON LOW=healthy, and motion/upload
inhibits remain. No KiCad edits, controller connection, upload, commit or push occurred.

## Implemented

- Target raw manual-input sampling precedes UART, HTTP, SPI and logging each loop.
- Startup, key takeover/removal, hold release, direction change/loss/conflict,
  safety loss, invalid input and >25 ms sampling gaps queue EM01 STOP operation 3
  using the existing checksum implementation. There is no release debounce delay.
- STOP preempts the pending diagnostic transaction. Repeated STOP requests coalesce
  while STOP is pending, preserving the three-attempt timeout instead of restarting
  it continuously. STOP remains available after a communication timeout.
- Stop transmission is serviced before the corresponding durable manual event is
  written. Event results say queued, not physically stopped. Existing faults are
  not cleared. A stalled loop or unavailable UART still prevents immediate delivery;
  this is software ordering, not a qualified hard-real-time stopping guarantee.
- Web floor calls are rejected while the manual key is asserted or manual input
  levels are unknown. Input intent appears in API/WebUI separately from motion
  permission; no manual RUN binding is enabled.
- The isolated RF normal-mode model now maps D0 floor 1, D1 light toggle, D2 floor 3,
  D3 floor 2, D4 STOP. Learned-slot/association-epoch checks and duplicate suppression
  remain. Default configurable diagnostic baud is 9600; decoder jumper/UART rate
  matching is distinct from RF remote timing. No target RF UART was started.

## Tests Executed

| Command | Local result |
| --- | --- |
| `npm ci` | Pass; 21 audit findings remain |
| `npm run check` | Pass |
| `npm run lint` | Pass |
| `npm test` | 12 tests pass |
| `npx playwright install --with-deps chromium` | Pass |
| `npm run test:browser` | Pass; 1440x1000 and 390x844, mocked API |
| `npm run build:embedded` | Pass; 236,179 raw / 71,491 gzip bytes |
| `node scripts/stage-firmware.mjs` | Pass |
| `pio run` | Pass; 48,832 RAM bytes, 845,805 flash bytes |
| `pio run -t buildfs` | Pass; no upload |
| `node firmware/scripts/generate-contract.mjs --check` | Pass |
| `node firmware/tests/contract.mjs` | Pass, including manual STOP ordering checks |
| `./firmware/tests/run.ps1` | 3,223 assertions pass |
| `python firmware/tests/test_build_guard.py` | Pass; fake build environment only |

Tests cover all 32 binary key/hold/up/down/safety input combinations, request
removal and STOP on release, conflicting/unknown inputs, sample gaps, monitor
preemption, repeated STOP requests, retry exhaustion, STOP after timeout, RF normal
map/duplicate behavior, and API rejection of contradictory manual intent. Browser
tests retain failed STOP, accepted STOP without physical-stop claims, disconnect/
reconnect, malformed status and responsive overflow checks. Physical switch timing,
UART delivery and actual mechanical stopping were not tested. Ubuntu CI was not run.

## Changed Files

- `firmware/src/core/ManualStop.h` (new)
- `firmware/src/core/DiagnosticVfd.h`
- `firmware/src/core/Rf.h`
- `firmware/src/main.cpp`
- `firmware/interface-contract.json` (RF specification metadata only; still v2)
- `firmware/tests/core_tests.cpp`
- `firmware/tests/contract.mjs`
- `firmware/README.md`
- `webapp/lib/controller.ts`
- `webapp/app/page.tsx`
- `webapp/tests/fixtures/status.json`
- `webapp/tests/controller.test.mjs`
- `webapp/tests/browser.mjs`
- `webapp/docs/api-contract.md`
- `webapp/README.md`
- `webapp/docs/embedded-delivery.md`
- `README.md`
- `docs/hardware-dependency-handoff.md`
- `docs/software-integration-checklist.md`
- `docs/webapp-embedded-delivery.md`
- This report.

Ignored build, manifest and screenshot outputs were regenerated.

## Outstanding Larger Request

Operational v3 activation, target RUN/write binding, target RF capture/learning,
RF programming D4 floor capture, WebUI programming/calibration, direction-specific
calibration persistence/predictive stopping, and full motion/response audit coverage
are not completed by this milestone. Existing host-only components are not a claim
that those target workflows work. The supplied EM01 operation definitions are not
a missing-frame blocker for this STOP implementation; no alternative frames were
invented. Echo/late-response behavior and parameter 13 readback remain bench/protocol
qualification concerns recorded in the prior handoff.

The physical external box and means of holding/stopping the load on controller,
drive or power failure remain unverified. This milestone implements the requested
serial STOP behavior; it does not establish independent stopping authority or
remove protections based on that behavior.
