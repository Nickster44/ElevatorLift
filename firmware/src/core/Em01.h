#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "VfdTiming.h"
namespace lift {
struct Telemetry {
  bool valid = false;
  uint32_t sampledMs = 0;
  unsigned busVolts = 0, frequency = 0, temperature = 0, current = 0, status = 0;
};
class Em01 {
 public:
  enum class Result { Idle, Pending, Acknowledged, Verified, Timeout, Mismatch };
  Telemetry telemetry;
  Result result = Result::Idle;
  bool stopAcknowledged = false;
  std::array<int, 17> parameters;
  std::array<uint32_t, 17> parameterSampleMs{};
  Em01() { parameters.fill(-1); }
  bool request(char operation, unsigned value, unsigned parameter, uint32_t now);
  void preemptStop(uint32_t now) {
    result = Result::Idle;
    request('3', 0, 0, now);
  }
  std::string transmit(uint32_t now);
  void receive(char byte, uint32_t now);
  bool healthy(uint32_t now) const {
    return telemetry.valid && now - telemetry.sampledMs <= VfdTiming::MonitorFreshMs && !poisoned_;
  }
  bool confirmedStopped(uint32_t now) const {
    return healthy(now) && telemetry.status == 0 && telemetry.frequency == 0;
  }
  bool busy() const { return result == Result::Pending; }
  static std::string frame(const std::string& payload);
  static bool valid(const std::string& frame);

 private:
  char op_ = 0;
  unsigned parameter_ = 0, value_ = 0, attempts_ = 0;
  bool sent_ = false, poisoned_ = false, readback_ = false;
  uint32_t sentMs_ = 0, startMs_ = 0, lastByteMs_ = 0;
  std::string outgoing_, buffer_;
  void accept(const std::string& f, uint32_t now);
};
}  // namespace lift
