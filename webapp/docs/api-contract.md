# Webapp API Contract

The production WebUI and controller API share an origin. Read endpoints return JSON. Every state-changing request must accept `X-Lift-Api-Token` when controller authentication is configured, apply firmware prechecks, return an authoritative result, and append an audit event.

See [firmware and WebUI integration gaps](firmware-integration-gaps.md) for the required capability negotiation, authoritative safety/landing state, VFD freshness, RF confirmation behavior, and integration acceptance tests.

## Used by the frontend today

### `GET /api/status`

Polled every three seconds. Expected fields:

```json
{
  "state": "idle",
  "position": 18420,
  "target": 18420,
  "targetFloor": 2,
  "normalRunTenthsHz": 450,
  "safetyOk": true,
  "home": false,
  "lowerLimit": false,
  "upperLimit": false,
  "lastVfdCommand": "S00A0000",
  "lastFault": "",
  "networkMode": "station",
  "stationIp": "192.168.4.28",
  "apIp": ""
}
```

The firmware should return a cached snapshot quickly; this request must never wait for VFD or storage operations.

### `POST /api/move?floor=N`

Requests a move to stable numeric floor `N`. The firmware resolves the floor position and nickname, then accepts or rejects the command after all motion prechecks. Expected success status: `202`. Expected rejection: `409` with a specific machine-readable reason.

### `POST /api/stop`

Requests a controlled stop. Expected success status: `202` with the new controller status. This is an operational command, not a substitute for the independent emergency-stop circuit.

### `POST /api/light/toggle`

Requests a light-state toggle. Expected JSON response:

```json
{ "on": true }
```

The returned state is authoritative; the browser should not assume the output changed.

## Firmware contracts still required

| Endpoint | Purpose and important rules |
| --- | --- |
| `GET/POST /api/settings` | Read and validate speeds, homing behavior, stop offset, and retention settings. Writes must report per-field validation errors. |
| `GET/POST /api/floors` | Stable numeric floor, editable nickname, and signed 64-bit encoder position. Renaming must not change the floor identifier. |
| `GET /api/vfd/parameters` | Return all documented metadata, cached/read-back value, freshness, and access level. |
| `GET/POST /api/vfd/parameter` | Stopped-only, range-checked, authenticated writes with VFD read-back confirmation and logging. |
| `GET /api/logs/recent` | Bounded/paged events including source, acceptance result, reason, position, and remote identity when relevant. |
| `GET/POST /api/remotes` | WebUI nickname and observation registry keyed by captured `TX_ID`; not an enumeration of decoder memory. |
| `POST /api/remotes/learn` | Start or cancel the decoder's 17-second Learn Mode and report `MODE_IND` state and remaining time. |
| `POST /api/remotes/erase-all` | Deliberate 10-second erase-all sequence. Must require confirmation and return progress/result; individual address deletion is unsupported. |
| `GET /api/network` and `POST /api/network` | Read connection state and store station credentials without returning stored passwords. |
| `GET /api/config/backup` | Export versioned controller settings, VFD values, floors, calibration, network preferences, and remote profiles. Exclude credentials by default and exclude decoder memory. |
| `POST /api/config/restore` | Validate size, schema version, ranges, conflicts, and CRC before committing. Unknown remote profiles remain pending until their IDs are observed. |
| Restricted recovery endpoints | May configure or observe a recovery session, but can never authorize motion without cabinet-local keyed enable and continuous physical hold-to-run. |

## Error shape

New endpoints should converge on a consistent response:

```json
{
  "error": "move_rejected",
  "reason": "safety_loop_open",
  "message": "Motion is unavailable while the safety loop is open."
}
```

The short codes are stable for UI and automation logic. Human-readable text may evolve. Never return API tokens, Wi-Fi passwords, raw secrets, or unbounded logs.

## Preview behavior

Failure to retrieve valid JSON from `/api/status` enables representative preview data. Preview actions may animate locally and must remain visibly identified as disconnected; they are not evidence that a controller command succeeded.
