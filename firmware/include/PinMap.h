#pragma once

#include <Arduino.h>

namespace Pins {

// Provisional ESP32-S3 pinout. Replace during schematic capture.
constexpr uint8_t ButtonTop = 4;
constexpr uint8_t ButtonRight = 5;
constexpr uint8_t ButtonBottom = 6;
constexpr uint8_t ButtonLeft = 7;
constexpr uint8_t ButtonCenter = 8;

constexpr uint8_t SafetyLoop = 9;   // Low means closed/healthy, matching old design.
constexpr uint8_t HomeSwitch = 10;  // Active low placeholder.
constexpr uint8_t UpperLimit = 11;  // Active low placeholder.
constexpr uint8_t LowerLimit = 12;  // Active low placeholder.

constexpr uint8_t VfdRx = 17;
constexpr uint8_t VfdTx = 18;

// Temporary pulse inputs until the external quadrature counter IC is selected.
constexpr uint8_t PositionUpPulse = 35;
constexpr uint8_t PositionDownPulse = 36;

constexpr uint8_t StatusLed = 48;

}  // namespace Pins

