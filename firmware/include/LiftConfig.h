#pragma once

#include <Arduino.h>

struct FloorTarget {
  uint8_t floor;
  int64_t positionCounts;
  uint32_t stopOffsetCounts;
};

namespace LiftConfig {

constexpr const char* Hostname = "lift";
constexpr uint32_t VfdBaud = 9600;
constexpr uint32_t VfdCommandRefreshMs = 100;
constexpr uint32_t StopRetryMs = 100;
constexpr uint32_t StopAckTimeoutMs = 2000;
constexpr uint32_t PositionPersistMs = 250;

constexpr uint16_t CruiseTenthsHz = 450;  // 45.0 Hz placeholder.
constexpr uint16_t ServiceJogTenthsHz = 250;

constexpr FloorTarget DefaultFloors[] = {
    {1, 0, 3500},
    {2, 20000, 3500},
    {3, 40000, 3500},
    {4, 60000, 3500},
};

constexpr size_t FloorCount = sizeof(DefaultFloors) / sizeof(DefaultFloors[0]);

}  // namespace LiftConfig
