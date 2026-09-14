# Firmware / WebUI Handoff

Current software contract: v2, diagnostics-inhibited. Reviewed 2026-09-14.
See [software checklist](../../docs/software-integration-checklist.md) and the
separate [hardware dependencies](../../docs/hardware-dependency-handoff.md).

## Closed In Software

- No automatic demo fallback, fake initial health, local-only STOP, fake telemetry,
  fabricated calibration, timer-based pairing success or local configuration success.
- Validated contract-v2 status in API-v1 envelopes, unavailable capabilities and stale/disconnected states.
- Controller-owned floor numbers, nullable position and decimal-string counts.
- Idempotent light set-state, real network save, bounded durable log read and errors.
- Native static WebUI serving, asset staging and N16R8 flash partition build.
- Host-tested replacement motion/protocol/storage logic; no old pulse-input runtime.

## Still Required Before Operational Integration

1. HW-01/HW-02 wiring is adopted by software contract v2. Complete input continuity,
   boot/reset and independent safety-authority bench qualification before an operational profile. Never merely
   flip `hardwareReady` or capability flags. Review hardwired enable and watchdog
   behavior independently of software; demonstrate timing under slow clients.
2. Diagnostic STOP/monitor/readback now uses UART1 independently of safety, with
   communications-only GPIO42. Integrate operational `DriveScheduler`, serialized parameter
   jobs and alarms. Portable parameter jobs now validate metadata, enforce service
   guards, invalidate calibration before writes and persist verified readback/cache;
   these are not operational target bindings.
   STOP ACK is not stopped; physical stop needs fresh zero-status/zero-frequency
   monitoring plus stable encoder observations. EM01 has no transaction ID.
3. Bind the supervisor's top-HOME reference handshake to the counter origin;
   commission scale/polarity, landing/home coordinates, tolerances, speeds and
   progress/time limits. Replace isolated program-exit calibration with explicit
   WebUI start after valid floor programming and persist direction-specific offsets.
   The session currently implements a single-calibration model and durable pre-RUN
   intent, not the full requested operational flow. No fabricated defaults.
4. Expose guarded settings/floor/fault-reset/homing/calibration handlers. Finish
   configuration migration and fault durability under every reset point, including
   physical power interruption during the immediate fault-ledger write. The target
   now records fault transitions immediately, independently of snapshots. Faults must not
   disappear after STOP or reboot. Web changes must await real results.
5. Finish RXM/LICAL serial TX_ID capture, debounce/correlation with output lines,
   verified five-button mapping, MODE_IND interpretation and learn/erase state
   confirmation. Portable learn/erase confirmation models are host-tested, but their
   timing is unqualified and they are not an integrated receiver. Persist
   epoch/association/nickname records; re-learn/erase/restore invalidates associations.
   TX_ID is a reusable slot, never a permanent transmitter identity.
6. Complete authenticated event attribution (source/operator/RF slot+epoch),
   paginated export, configurable retention, RTC setting/backup verification and
   write-protected/absent-device tests on target. Current events use bounded JSON
   payloads inside integrity-checked MRAM records, not the final dense binary format.
7. Implement backup/restore, reboot and OTA only with verified stopped/service
   guards, version compatibility, durable transaction results and recovery tests.
8. Add API/UI support for each capability as it becomes operational. The current
   setup/drive-write/remotes/recovery screens intentionally remain unavailable.

## Acceptance Evidence

`firmware/tests/run.ps1` tests correct outcomes; it does not rerun the historical
defect-accepting expectations. `webapp/tests/controller.test.mjs` and
`browser.mjs` cover malformed/disconnected/reconnected APIs and actual STOP
requests. The 2026-09-10 reproduction harness remains untouched as historical
evidence tied to its baseline commit, not a current qualification suite.
