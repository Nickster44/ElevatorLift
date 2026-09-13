# Controller API v1

Status contract version 1 corresponds to `firmware/interface-contract.json`.
`webapp/lib/controller.ts` is the executable response validator, and
`tests/fixtures/status.json` is an inhibited example. Firmware serializes JSON
with ArduinoJson, including SSID escaping. No live-to-demo fallback exists.

## Status

`GET /api/status` returns a cached input snapshot, API/contract versions,
16-hex-character boot identity, sequence, uptime and sample age. Three floors have stable numbers 1-3, names
and nullable decimal-string coordinates. Position is a signed decimal string;
`positionValid=false` means it must not be displayed as a known landing.
`currentFloor`/`targetFloor` are nullable. No browser count thresholds are used.

Permission fields: `motionAllowed`, `canMoveUp`, `canMoveDown`, `blockedReasons`,
`stoppedConfirmed`, `deploymentReady`, safety/home and nullable limit/service-key
inputs. The Rev-A inhibited profile always denies motion and returns unknown
limits/key, not false/healthy. Its blocked reasons explicitly include HW-01/HW-02.

`capabilities` are boolean flags, not a promise inferred from visible controls.
`telemetry=null` means unavailable; if present it has a separate `ageMs`.
`light.on` is the commanded output, not physical load feedback.
`calibration.valid=false` and null distance mean no usable calibration.
The browser polls without overlap, aborts after 1800 ms, rejects input sample age
over 100 ms, and expires received status after 2500 ms. Failed polls do not
advance the last-valid timestamp. Unmounted/obsolete polls cannot update state.
Duplicate or backward sequences within one boot are rejected. Freshness uses a
monotonic browser clock and is checked again when dispatching non-STOP commands.

## Current Routes

| Route | Result |
| --- | --- |
| GET `/api/status` | v1 status |
| GET `/api/capabilities` | v1 boolean capabilities |
| GET `/api/network` | Redacted network status and SSID |
| POST `/api/network` | Validated SSID/password saved in NVS; restart required, no automatic restart |
| POST `/api/light` | Idempotent `on=true` or `on=false`; returns status |
| POST `/api/move` | Strict floor 1-3; current profile returns 409 `hardware_unresolved` |
| POST `/api/stop` | Supervisor stop request; current profile returns 503 `stop_delivery_unavailable_hardware_inhibited`, never claims physical stop |
| GET `/api/logs/recent` | Up to 16 durable events, or 503 when MRAM is unavailable |
| GET `/api/vfd/parameters` | Metadata only; `available=false`, no fabricated readback |
| Other `/api/` routes | 501 `unsupported_capability` |

All POSTs require a configured token >=16 characters in `X-Lift-Api-Token`.
Empty/short configured tokens fail closed. Tokens in query strings are unsupported.
Write bodies use `application/x-www-form-urlencoded`; query parameters may also
be used. Duplicate arguments, nonnumeric floors, truncating/wrapping values and
unknown light/network fields are rejected. There is no toggle endpoint.

Error shape: `{"apiVersion":1,"error":"machine_readable_reason"}`. Authentication
failure is 401, malformed input 400, conflict 409, unavailable path 503,
unsupported feature 501. A write response is not a motion-complete indication.
Non-STOP writes are rate limited to one per 250 ms (429 when exceeded); STOP is
excluded from this rate limit.

## HTTP And Update Boundaries

One active client, 2048-byte headers, 512-byte form body, 16 arguments,
256-character request target, 128 input bytes and 512 output bytes serviced per
loop. Headers must finish within 500 ms; responses are closed after 5 seconds.
Duplicate headers, chunked requests and non-form bodies are rejected. No CORS
access or browser request forwarding is provided. Responses carry no-store;
hashed static assets use immutable caching, gzip and a self-origin CSP.

These are allocation/work bounds, **not a qualified motion deadline**: ESP32
Wi-Fi socket writes, NVS operations, bus calls and scheduling still need measured
worst-case latency and supervisor-task/watchdog integration before motion can be
enabled. Do not expose this listener on a public network.

## Planned Routes

Settings/floors, homing/program exit/calibration, fault reset, parameter read/write
jobs, RF registry/learn/erase, backup/restore, and reboot remain unsupported on
the target. Their portable logic is described in the software checklist. Add
controller handlers, tests and UI controls together; do not advertise a capability
merely because a portable class exists. REST automation uses the same supervisor
and authentication as the WebUI. MQTT/Google Home remain hub-side future work.
