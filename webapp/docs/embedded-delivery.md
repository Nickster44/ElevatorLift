# ESP32 Embedded Delivery

## Resource budget

`npm run build:embedded` produces a minified static browser application and `.gz` copies under `dist/embedded`.

Measured September 2026 output:

| Bundle | Size |
| --- | ---: |
| HTML + CSS + JavaScript, raw | 233,572 bytes |
| HTML + CSS + JavaScript, gzip | 70,809 bytes |
| Staging limit (raw plus compressed) | 2 MiB |

The current `firmware/partitions.csv` reserves two 4 MiB application slots,
LittleFS at 0x810000 of size 0x7e0000, NVS/OTA data and a 64 KiB coredump region.
It fits exactly within 16 MiB. OTA-sized slots do not mean an OTA updater is implemented.

The ESP32 streams static bytes. React and gzip decompression execute in the browser.

## Release asset rules

- This development package includes raw and gzip variants for clients without gzip. Both count toward the 2 MiB staging limit.
- Serve the HTML entry point as `text/html` with `Content-Encoding: gzip` and `Cache-Control: no-cache`.
- Serve hashed CSS and JavaScript as `text/css` and `text/javascript`, with `Content-Encoding: gzip` and `Cache-Control: public, max-age=31536000, immutable`.
- Reject unknown paths instead of performing MCU-side templating.
- `scripts/stage-firmware.mjs` creates a SHA256 manifest and removes obsolete previously-manifested assets. Firmware checks every packaged hash and API/contract version at boot before serving the UI. It never reformats LittleFS automatically. Hashes detect corruption, not malicious firmware or asset replacement.

## Server constraints

- HTTP work must remain nonblocking and subordinate to safety monitoring, motion service, VFD communication, watchdog servicing, and position persistence.
- Stream static files in small chunks. Do not load complete assets, backups, or log exports into internal SRAM.
- Return `/api/status` from an already available state snapshot; do not synchronously poll hardware in the handler.
- Page or stream logs and configuration exports.
- Apply request-body and total-response limits. Reject oversized restore documents before parsing.
- Limit simultaneous clients and idle connection duration. Revision-A validation should cover at least two browsers connected while the motion simulator and all periodic firmware services run.
- Keep write authentication, origin/CSRF defenses, and command rate limits independent from frontend behavior.

## Release verification

1. Run `npm run check`, `npm run build`, and `npm run build:embedded`.
2. Record the compressed total and fail packaging if it exceeds the selected LittleFS budget.
3. Verify cold load, cached reload, fallback AP, station mode, and reconnect behavior on the target ESP32-S3.
4. Exercise malformed requests, interrupted file transfers, slow clients, repeated status polling, and configuration-upload limits.
5. Confirm that web traffic cannot measurably delay stop handling, safety-state transitions, VFD servicing, persistence, or watchdog feeding.

## Update Strategy

Current operation is **build-only**: `npm run build:embedded`,
`node scripts/stage-firmware.mjs`, then `pio run -t buildfs`. Upload targets are
blocked. A release must pair application and filesystem artifacts from the same
build and record both hashes plus contract version. Never update while moving.

No OTA/network update or rollback endpoint exists yet. Before adding one, require
guarded confirmed stop, independent motion inhibition, authenticated/signed image
validation, compatibility checks, power-interruption recovery and a confirmed
boot before accepting the new application. The current single LittleFS partition
does not provide atomic A/B asset rollback; solve that before advertising seamless
OTA. A missing/corrupt/incompatible bundle returns an error while diagnostics stay
available. Never let an asset update or UI reconnect enable motion.
