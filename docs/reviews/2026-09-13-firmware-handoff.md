# Finalized Hardware Handoff: Software Adoption

Contract v2 adopts the supplied 2026-09-13 connectivity evidence (227 checks).
This report records software simulation/build evidence, not powered-board testing.
No KiCad files, eFuses, hardwired safety circuitry, uploads, commits or pushes were
changed/performed by this software task. Existing hardware-agent changes are preserved.

## Behavior

- Upper/lower limits use GPIO43/44; active-low service key uses GPIO3, input-only.
  No AUX runtime remains. GPIO35/36/37 remain unavailable.
- UART0 application debug, IDF logging and ROM-print channels are suppressed before
  configuring inputs. Native USB remains the console. Actual ROM/bootloader output
  predates setup; R62 contention and boot waveforms require bench verification.
- GPIO42 is LOW at initialization, then enables the UART translator after UART1
  initialization. It is never RUN permission. UART diagnostics are independent
  of the safety input. The external hardwired loop remains the sole motion authority.
- Inputs need 100 ms stability and <=25 ms intersample gaps. Invalid/missing samples
  and transitions invalidate readiness. Unknown key state also blocks the supervisor.
  Open contacts, broken wires and absent field power can all read HIGH. Therefore
  field continuity qualification remains false in the target, independently of
  the preserved global motion inhibit. There is no web/configuration bypass.
- The diagnostic scheduler exposes only STOP, monitor and READ. STOP is prioritized;
  monitoring/readback alternate at 100 ms scheduling intervals, one transaction at
  a time. Parameter 13 readback remains unsupported. Three failed attempts poison
  the session until reset; STOP remains possible. No automatic retry-session reset
  or RUN/write binding was added.
- API envelope v1 now requires hardware contract v2. STOP returns 202 queued status
  or 503 if UART unavailable. Transmitted, acknowledged and fresh monitor-stopped
  evidence are distinct; physical `stoppedConfirmed` remains false. Parameter cache
  values and ages are real readbacks when available, otherwise null/unavailable.
- The WebUI validates v2, shows active-low readings separately from qualification,
  expires readback display after 5 seconds, preserves failed STOP reporting and
  never interprets acceptance/acknowledgement as permission or physical stopping.

## Verification

All commands below passed locally on Windows with Node 24 and the pinned toolchains.
CI configuration is unchanged; Ubuntu execution is not claimed.

| Command | Result |
| --- | --- |
| `npm ci` in webapp | Pass; 21 audit findings remain (2 low, 2 moderate, 16 high, 1 critical) |
| `npm run check` / `npm run lint` | Pass |
| `npm test` | 11 tests pass |
| `npx playwright install --with-deps chromium` | Pass |
| `npm run test:browser` | Pass at 1440x1000 and 390x844; mocked API only |
| `npm run build:embedded` | Pass; 235,521 raw / 71,305 gzip bytes |
| `node scripts/stage-firmware.mjs` | Pass; generated manifest uses contract v2 |
| `pio run` in firmware | Pass; 48,816 RAM bytes, 844,293 flash bytes |
| `pio run -t buildfs` | Pass; no upload |
| `node firmware/scripts/generate-contract.mjs --check` | Pass |
| `node firmware/tests/contract.mjs` | Pass; finalized evidence, pin aliases, UART0 suppression, inhibit |
| `./firmware/tests/run.ps1` | 2,866 assertions pass |
| `python firmware/tests/test_build_guard.py` | Pass; fake build environment only |

New regressions cover input startup, gaps, invalid levels, all open/closed/unknown
combinations without continuity evidence, conflicting limits, key loss during
motion, diagnostic-only frame whitelist, STOP ACK versus monitor status, timeout
poisoning, STOP after communication loss, v1 rejection, v2 qualification checks,
queued STOP UI behavior, cache validation and unsupported readback display.
Existing disconnect/reconnect, failed STOP, malformed status and overflow assertions
remain intact. Broader firmware safety tests still use host simulations.

## Changed Files In This Task

Firmware:
`firmware/interface-contract.json`, `firmware/include/PinMap.h`,
`firmware/scripts/generate-contract.mjs`, `firmware/tests/contract.mjs`,
`firmware/platformio.ini`, `firmware/src/main.cpp`,
`firmware/src/core/FieldInputs.h`, `firmware/src/core/DiagnosticVfd.h`,
`firmware/src/core/Supervisor.h`, `firmware/src/core/Supervisor.cpp`,
`firmware/tests/core_tests.cpp`, `firmware/README.md`.

Web:
`webapp/lib/controller.ts`, `webapp/app/page.tsx`,
`webapp/tests/fixtures/status.json`, `webapp/tests/controller.test.mjs`,
`webapp/tests/browser.mjs`, `webapp/scripts/stage-firmware.mjs`,
`webapp/README.md`, `webapp/docs/api-contract.md`,
`webapp/docs/firmware-integration-gaps.md`, `webapp/docs/embedded-delivery.md`.

Project documentation:
`README.md`, `docs/hardware-dependency-handoff.md`,
`docs/software-integration-checklist.md`, `docs/webapp-embedded-delivery.md`,
`docs/reviews/2026-09-13-firmware-handoff.md`.

Ignored web build/screenshots and firmware build/asset outputs were regenerated.

## Remaining Bench Dependencies

1. Verify actual NO/NC field wiring, open-wire/field-power-loss behavior and a means
   to establish ongoing continuity. A stable GPIO is not continuity evidence.
2. Scope GPIO43/R62 at boot and validate input thresholds after UART0 release.
   Test GPIO3 open/closed key during reset, brownout and both rail sequences;
   inspect JTAG eFuses without changing them.
3. Verify UART1 levels/framing, translator OE reset behavior and STOP/monitor/readback
   with safety open using a non-energized peripheral emulator. Qualify EM01 late
   same-type reply ambiguity, echo behavior, turnaround and timeout policy.
4. Independently verify external hardwired inhibit cannot be bypassed by serial
   commands. This software test does not establish that property electrically.
5. Commission filtering/timing, encoder/RTC/MRAM behavior, supervisor scheduling,
   watchdog and worst-case network/bus latency. Remaining operational motion,
   parameter-write, RF and commissioning/recovery integrations are still inhibited.

Full hardware gates remain in [the hardware handoff](../hardware-dependency-handoff.md).
