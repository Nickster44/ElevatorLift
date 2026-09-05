# Home Automation Integration Plan

## Recommended Architecture

Keep the lift controller local-first and let a general home automation hub bridge it to Google Home, watches, voice assistants, and other ecosystems.

```text
Google Home / phone / watch
        |
        v
Home Assistant or equivalent local automation hub
        |
        v
Lift controller local REST API, later optional MQTT
        |
        v
Motion precheck -> command accepted/rejected -> VFD control -> event log
```

The lift controller should not depend directly on Google cloud services, Matter commissioning, or a vendor-specific voice-assistant SDK. Those systems can change independently. The lift should remain controllable from its local WebUI and local API.

## Rev-A Integration Surface

The controller should expose:

- `http://lift.local/` WebUI using mDNS when the local network supports it.
- `GET /api/status` for read-only state.
- `POST /api/move?floor=N` for guarded floor requests.
- `POST /api/stop` for guarded stop requests.
- `X-Lift-Api-Token` header on write endpoints when a token is configured.
- Clear JSON errors for rejected, unauthorized, busy, faulted, or unsafe requests.

Current firmware includes optional token checking. If `LIFT_API_TOKEN` is blank, write endpoints remain open for development. A production build should set a long random token and require Home Assistant or another trusted automation hub to include it in the `X-Lift-Api-Token` header.

## Home Assistant Path

Home Assistant is the preferred integration hub because it can run locally, expose selected entities or scripts to Google Home, and bridge to many other systems later.

Recommended setup:

1. Run Home Assistant on a local hub such as Home Assistant Green, Home Assistant Yellow, Raspberry Pi with SSD, or a small mini PC.
2. Add REST commands or a custom integration that calls the lift controller API.
3. Create explicit scripts/buttons:
   - `Lift to floor 1`
   - `Lift to floor 2`
   - `Lift to floor 3`
   - `Lift stop`
4. Expose only those scripts/buttons to Google Assistant.
5. Use Google Home routines for nicer spoken phrases if needed.

Do not expose raw settings, calibration, VFD parameter writes, or service functions to Google Home. Those should remain installer/WebUI-only.

Example Home Assistant REST command shape:

```yaml
rest_command:
  lift_move:
    url: "http://lift.local/api/move?floor={{ floor }}"
    method: post
    headers:
      X-Lift-Api-Token: !secret lift_api_token

  lift_stop:
    url: "http://lift.local/api/stop"
    method: post
    headers:
      X-Lift-Api-Token: !secret lift_api_token
```

Example scripts:

```yaml
script:
  lift_to_floor_1:
    alias: Lift to floor 1
    sequence:
      - service: rest_command.lift_move
        data:
          floor: 1

  lift_to_floor_2:
    alias: Lift to floor 2
    sequence:
      - service: rest_command.lift_move
        data:
          floor: 2

  lift_stop:
    alias: Lift stop
    sequence:
      - service: rest_command.lift_stop
```

Expose the scripts, not the raw REST command, to Google Home. This keeps the voice/watch surface intentionally small.

## MQTT Option

MQTT is the best next protocol to add after REST because it gives Home Assistant a clean publish/subscribe model and supports discovery.

Proposed topics:

```text
lift/status
lift/availability
lift/event
lift/command/move
lift/command/stop
homeassistant/button/lift_floor_1/config
homeassistant/button/lift_floor_2/config
homeassistant/button/lift_floor_3/config
homeassistant/button/lift_stop/config
```

Recommended behavior:

- Publish retained discovery/config topics only when MQTT is enabled.
- Publish availability as `online` / `offline`.
- Publish status JSON after every state change and periodically while moving.
- Accept move commands only when authenticated through broker credentials and after normal controller prechecks.
- Log every MQTT-originated command the same way as WebUI and RF commands.

MQTT is optional for rev A. REST is sufficient to integrate with Home Assistant first.

## Google Home Path

Use Google Home through Home Assistant rather than from the lift controller directly.

Two practical options:

- Home Assistant Google Assistant integration: expose selected Home Assistant scripts/entities to Google Home. This may use cloud account linkage, with optional local fulfillment depending on setup.
- Matter bridge through Home Assistant or a companion bridge: potentially more local, but adds another layer and should be considered later after the REST/Home Assistant path works.

The lift controller should never need to know that Google Home exists. From the controller's point of view, it receives an authenticated local command from a trusted local hub.

## Command Safety Rules

All outside-control paths must follow the same rules:

- No command bypasses hardwired safety devices.
- No command bypasses firmware motion prechecks.
- Reject move commands in fault, service, calibration, stopping, moving, or unknown-position states.
- Reject commands when safety loop or limit state is invalid.
- Log command source, requested floor/action, accepted/rejected result, and reason.
- Require explicit stop command support.
- Keep calibration and VFD parameter writes out of voice-assistant exposure.

## Hardware Impact

No extra hardware is required on the lift controller beyond reliable Wi-Fi and the external antenna path already planned for the selected `ESP32-S3-WROOM-1U-N16R8`.

The property should have one always-on local automation hub if Google Home/watch control is desired. Recommended devices:

- Home Assistant Green for the simplest appliance-like setup.
- Home Assistant Yellow if expandability is valuable.
- Raspberry Pi 5 with SSD if a DIY platform is acceptable.
- Small mini PC if maximum reliability and headroom are desired.
