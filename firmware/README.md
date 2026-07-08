# Firmware Starter

This is a PlatformIO starter firmware for an ESP32-S3 reference target. It is intended to prove the controller architecture before the final KiCad board is selected.

## Files

| File | Purpose |
| --- | --- |
| `platformio.ini` | PlatformIO build target |
| `include/PinMap.h` | Provisional ESP32-S3 pins |
| `include/LiftConfig.h` | Motion constants and default floor table |
| `include/Secrets.example.h` | Fallback AP credential template |
| `src/NetworkConfig.*` | Stored station network settings and AP fallback defaults |
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

## Current Limitations

- `PositionStore` uses ESP32 NVS as a placeholder; replace it with an MRAM-backed implementation once the MRAM part is selected.
- Position input is still represented by provisional interrupt pins until the quadrature counter IC is selected.
- Pin assignments are placeholders.
- Web write endpoints have no authentication yet.
- Network settings use ESP32 NVS for now; final settings should move to the MRAM configuration store.
- The motion constants are placeholders and must not be used on real hardware.
