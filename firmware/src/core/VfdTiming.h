#pragma once
#include <cstdint>
namespace lift::VfdTiming {
constexpr uint32_t ReplyTimeoutMs = 150;
constexpr unsigned Attempts = 3;
constexpr uint32_t MonitorFreshMs = 500;
constexpr uint32_t TrafficSlotMs = 100;
constexpr uint32_t StopRefreshMs = 300;
// Keep the existing 1-second maximum; remove the too-short 0.2-0.9 second options.
// This development policy still needs worst-case scheduling/drive qualification.
constexpr unsigned MinWatchdogTenths = 10, MaxWatchdogTenths = 10;
constexpr uint32_t WatchdogReadFreshMs = 10000;
static_assert(StopRefreshMs + ReplyTimeoutMs * Attempts < MinWatchdogTenths * 100);
}  // namespace lift::VfdTiming
