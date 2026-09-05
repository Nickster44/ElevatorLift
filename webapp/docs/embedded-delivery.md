# ESP32 Embedded Delivery

## Resource budget

`npm run build:embedded` produces a minified static browser application and `.gz` copies under `dist/embedded`.

Measured September 2026 output:

| Bundle | Size |
| --- | ---: |
| HTML + CSS + JavaScript, raw | approximately 247 KB |
| HTML + CSS + JavaScript, gzip | approximately 74 KB |
| Share of 16 MB module flash | approximately 0.44% |

The exact application, OTA, NVS, and LittleFS partition table remains a firmware decision. Reserve at least 256 KB for the WebUI so normal growth does not immediately require repartitioning. A 512 KB LittleFS partition offers healthier revision-A margin if the OTA layout permits it.

The ESP32 does not execute React. It reads static bytes from flash and sends them over HTTP; decompression and JavaScript execution happen in the client browser. Web-server RAM use comes primarily from TCP/HTTP buffers, small JSON documents, and filesystem chunks—not the 247 KB raw bundle.

## Release asset rules

- Package only `.gz` versions of HTML, CSS, and JavaScript when transparent gzip serving is enabled. Do not waste flash on duplicate raw copies.
- Serve the HTML entry point as `text/html` with `Content-Encoding: gzip` and `Cache-Control: no-cache`.
- Serve hashed CSS and JavaScript as `text/css` and `text/javascript`, with `Content-Encoding: gzip` and `Cache-Control: public, max-age=31536000, immutable`.
- Reject unknown paths instead of performing MCU-side templating.
- Generate an asset manifest during firmware integration so firmware does not hard-code content hashes manually.

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
