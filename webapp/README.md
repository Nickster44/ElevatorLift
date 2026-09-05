# Elevator Lift Web Console

Browser-based operation and service interface for the ESP32-S3 elevator-lift controller. The webapp is developed independently from the motion firmware, then emitted as a small static bundle that the controller can serve from flash.

## Current status

The first design revision includes:

- A responsive overview with safety state, car position, floor controls, Stop, lift-light control, VFD telemetry, and recent activity.
- Editable floor nicknames and encoder positions in the lift-setup view.
- Stop-distance calibration status and all 17 documented EM01 VFD parameters.
- Current and overload-limit controls presented as lift protection, not a certified weight measurement.
- RF remote views based on five-button remotes, observed `TX_ID` identities, nicknames, Learn Mode, and decoder erase-all behavior.
- Event-log, network, API-token, configuration backup/restore, and restricted service-recovery designs.
- Representative preview data whenever the firmware API is unavailable.

Only status polling, move, stop, and light-toggle requests currently have frontend HTTP calls. Other controls intentionally demonstrate the planned workflow while their firmware contracts are being completed. See [API contract](docs/api-contract.md) for the exact boundary.

The known cross-team gaps are tracked in [firmware and WebUI integration gaps](docs/firmware-integration-gaps.md). Until those contracts are implemented, simulated controls must remain preview-only or unavailable and must not claim that connected hardware changed state.

The WebUI never grants motion authority. Firmware and independent hardware remain responsible for authentication, state validation, safety-loop monitoring, final limits, VFD health, watchdog behavior, and fail-safe motion removal. See [product and safety decisions](docs/product-and-safety.md).

## Development

Requirements: Node.js 22.13 or later and npm.

```powershell
npm install
npm run dev
```

Open `http://localhost:3000`. If `/api/status` is unavailable at the same origin, the interface identifies itself as using preview data.

Useful checks:

```powershell
npm run check
npm run build
npm run build:embedded
```

- `build` validates the full development/Sites application.
- `build:embedded` creates the static MCU release assets and gzip copies in `dist/embedded`.
- Generated outputs, dependencies, logs, and TypeScript build metadata are ignored by Git.

## ESP32 delivery

The September 2026 embedded build measures approximately 247 KB raw and 74 KB gzip. This is about 0.44% of the selected ESP32-S3-WROOM-1U-N16R8 module's 16 MB flash. React runs in the user's browser; the MCU only serves static files and bounded JSON responses.

The firmware release should package only the gzip assets, stream them from LittleFS, and serve hashed files with long-lived caching. Full storage, header, concurrency, and verification guidance is in [embedded delivery](docs/embedded-delivery.md).

## Project map

| Path | Purpose |
| --- | --- |
| `app/page.tsx` | Current interface, state, demo behavior, and API requests |
| `app/globals.css` | Responsive industrial visual system |
| `embedded/` | Static browser entry point used for MCU builds |
| `scripts/compress-embedded.mjs` | Creates maximum-compression gzip assets and reports size |
| `vite.embedded.config.ts` | Static embedded-build configuration |
| `docs/api-contract.md` | Connected and proposed controller endpoints |
| `docs/firmware-integration-gaps.md` | Shared firmware/WebUI contract gaps and acceptance tests |
| `docs/embedded-delivery.md` | ESP32 storage and serving requirements |
| `docs/product-and-safety.md` | Durable WebUI behavior and safety decisions |

The broader controller requirements remain in the repository-level `docs/`, `firmware/`, and `hardware/` folders.
