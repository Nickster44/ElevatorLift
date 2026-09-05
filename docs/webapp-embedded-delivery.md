# Embedded Webapp Delivery Plan

## Current Measured Bundle

Run `npm run build:embedded` from `webapp/` to create a browser-only production bundle under `webapp/dist/embedded/` and precompress each HTML, CSS, and JavaScript asset with gzip.

Measured on the July 2026 interface revision:

| Asset class | Raw | Gzip |
| --- | ---: | ---: |
| HTML + CSS + JavaScript | 246,526 bytes | 73,795 bytes |

The selected `ESP32-S3-WROOM-1U-N16R8` has 16 MB flash and 8 MB PSRAM. A gzip-only copy of the current webapp is about 0.44% of total module flash. The exact application/OTA/LittleFS partition table still needs to be selected, but the current interface is comfortably small enough for internal flash and does not require external QSPI NOR.

The ESP32 does not execute React. It stores and streams static bytes; the user's browser decompresses the files and runs the interface. MCU RAM use is therefore limited to the HTTP server, TCP buffers, small JSON responses, authentication state, and filesystem reads. PSRAM is not required solely to serve this bundle.

## Firmware Serving Rules

- Store only the `.gz` production assets in LittleFS when the firmware server supports transparent gzip delivery. Do not store duplicate raw and compressed copies in the release image.
- Serve hashed JavaScript/CSS asset names with `Content-Encoding: gzip` and `Cache-Control: public, max-age=31536000, immutable`.
- Serve the HTML entry point with `Content-Encoding: gzip` and `Cache-Control: no-cache` so firmware updates can reference new hashed assets immediately.
- Use explicit MIME types (`text/html`, `text/css`, `text/javascript`, and `application/json`) and reject unknown paths rather than rendering templates on the MCU.
- Keep API JSON compact and bounded. Stream or page logs/exports instead of constructing large responses in RAM.
- Limit simultaneous clients and apply request/body size limits. Configuration uploads must be versioned, size checked, parsed incrementally where practical, and validated before committing to MRAM.
- Keep HTTP handling nonblocking and subordinate to motion/safety servicing. Static transfers should read small filesystem chunks; no request handler may wait on VFD replies, RF learning, calibration, or motion completion.
- The current UI polls the small status endpoint every three seconds. Firmware should return a cached status snapshot quickly. Server-Sent Events can be considered later, but are not required for rev A.
- Authentication and CSRF/origin controls remain required for every write endpoint. Service recovery requires separate installer and cabinet-local authorization and cannot be enabled by a browser request alone.

## Release Checks

1. Run the embedded build and record raw/gzip totals.
2. Fail the firmware asset step if gzip assets exceed the reserved LittleFS budget.
3. Confirm a cold page load, cached reload, AP fallback, and station mode on the actual ESP32-S3.
4. Load-test at least two browser clients while motion simulation, VFD polling, encoder reads, persistence, and watchdog servicing remain active.
5. Verify that interrupted transfers, malformed requests, and repeated polling cannot delay a stop or safety-state transition.
