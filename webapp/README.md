# Elevator Lift Web Console

The WebUI is a same-origin client of the versioned controller API. It starts
disconnected, never switches to demo mode, and never manufactures motion,
calibration, telemetry, pairing, logging or command success. Unsupported hardware
paths are visibly unavailable. A valid HTTP status response is not motion permission.

`lib/controller.ts` validates runtime status shape, versions, counts, capabilities,
freshness and contradictory permission fields. Encoder counts are decimal strings
to preserve signed 64-bit values. The server owns floor identity and position.
STOP always attempts a real request, even when status is disconnected; failure
is shown and never changes the displayed motion state to idle.

Hardware contract v2 adopts the 2026-09-13 handoff. Stable active-low input readings
are displayed separately from field qualification (still false). Diagnostic UART
STOP/monitor/readback is supported while safety is open; no RUN or write capability
is enabled. Queued STOP and acknowledgement never imply physical stopping.

## Development

Node 22.13+ and npm:

```powershell
npm ci
npm run check
npm run lint
npm test
npm run build:embedded
```

`npm run dev` starts the full development app. For the exact embedded entry:
`npm exec vite -- --config vite.embedded.config.ts --host 127.0.0.1 --port 5178 --strictPort`.
Without an API, the page remains disconnected with commands unavailable; it is
not a simulator. No server connects to physical lift hardware during tests.

## Reproducible Browser Test

From a clean checkout at repository root, with Node 22.13+ and npm installed:

```powershell
cd webapp
npm ci
npx playwright install --with-deps chromium
npm run test:browser
```

The install command downloads the browser matching the locked Playwright version;
on Ubuntu it also installs OS dependencies (sudo privileges may be required).
The runner builds the embedded bundle, starts Vite preview only on
`http://127.0.0.1:5178`, waits for readiness, runs the test, and closes its server
on success, failure, or interruption. Port conflicts fail instead of reusing an
unknown server; stop any existing preview on 5178 first. The run has a 120-second
deadline after the build. No module, browser-channel or target-URL overrides are used.

`tests/browser.mjs` retains desktop/mobile disconnect, real STOP transport with
failure reporting, malformed-status rejection, reconnection, and view overflow
assertions. All controller API requests are mocked: this is browser evidence,
not target or motion verification. Screenshots go to ignored `test-results/`.
CI runs this command after the WebUI build; `npm test` remains the separate
unit/SSR suite. Neither command deploys or contacts a physical controller.

## Embedded Delivery

`build:embedded` emits static files plus gzip variants. On 2026-09-13 the bundle
measured 237,468 raw bytes / 71,835 gzip bytes. `node scripts/stage-firmware.mjs`
copies the assets and a hash manifest into ignored `firmware/data`. Then
`pio run -t buildfs` builds the LittleFS image. No deployment/upload is authorized.

See [API](docs/api-contract.md), [remaining integration work](docs/firmware-integration-gaps.md)
and [delivery constraints](docs/embedded-delivery.md). The current UI deliberately
does not enable unimplemented setup, drive-write, RF or restore workflows.
