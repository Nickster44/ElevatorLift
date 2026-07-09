# Firmware Starter

This is a PlatformIO starter firmware for an ESP32-S3 reference target. It is intended to prove the controller architecture before the final KiCad board is selected.

The current hardware direction is ESP32-S3-WROOM-1U with an LS7366R quadrature counter, SPI MRAM, RXM-418-LR RF receiver input, local AP/station Wi-Fi, and a VFD UART driver that can source/sink enough current for the VFD opto-isolated serial input.

## Files

| File | Purpose |
| --- | --- |
| `platformio.ini` | PlatformIO build target |
| `include/PinMap.h` | Provisional ESP32-S3 pins |
| `include/LiftConfig.h` | Motion constants and default floor table |
| `include/Secrets.example.h` | Fallback AP credential template |
| `src/EventLog.*` | Recent event log scaffold |
| `src/LiftSettings.*` | Persistent local lift settings |
| `src/NetworkConfig.*` | Stored station network settings and AP fallback defaults |
| `src/VfdParameters.*` | VFD parameter metadata and WebUI/API definitions |
| `src/VfdProtocol.*` | EM01 VFD command framing/checksum |
| `src/PositionStore.*` | Persistent state placeholder using ESP32 NVS |
| `src/main.cpp` | Main state machine, web API, inputs, and VFD servicing |

## Network Setup

The controller starts a local fallback access point. Connect to that AP, open the controller page, save the station network SSID/password, then reboot from the page/API. If the controller cannot connect to the saved station network, or if the station connection is later lost, it enables the fallback AP again.

Copy `include/Secrets.example.h` to `include/Secrets.h` only if you want to change the fallback AP credentials at build time:

```cpp
#pragma once
#define WIFI_AP_SSID "LiftControllerSetup"
#define WIFI_AP_PASSWORD "change-me-1234"
```

`Secrets.h` is intentionally ignored by Git.

## Build

```powershell
pio run
```

## API Surface

- `GET /api/status`
- `GET /api/network`
- `POST /api/network`
- `POST /api/reboot`
- `GET /api/settings`
- `POST /api/settings`
- `GET /api/logs/recent`
- `GET /api/vfd/parameters`
- `GET /api/vfd/parameter?number=N`
- `POST /api/vfd/parameter`
- `POST /api/move?floor=N`
- `POST /api/stop`

Future API work should include token authentication, an automation-safe local REST surface, and optional MQTT/Home Assistant integration. Any WebUI, RF, or automation motion request must pass the same motion prechecks and be logged.

## Current Limitations

- `PositionStore` uses ESP32 NVS as a placeholder; replace it with an MRAM-backed implementation for the selected Siproin `PM004MNIATR` or compatible SPI MRAM.
- Position input is still represented by provisional interrupt pins; replace it with an LS7366R-backed position service.
- Pin assignments are placeholders.
- Web write endpoints have no authentication yet.
- Network settings use ESP32 NVS for now; final settings should move to the MRAM configuration store.
- VFD parameter reads/writes send protocol commands and return pending read-back status; complete frame parsing and cache persistence are still TODO.
- `EventLog` is an in-memory scaffold; final recent logs should move to MRAM.
- RF receive, pairing, and remote registry support are not implemented yet, but RXM-418-LR compatibility is required.
- Home automation is not implemented yet; the preferred path is local REST first, optional MQTT/Home Assistant later.
- The motion constants are placeholders and must not be used on real hardware.
