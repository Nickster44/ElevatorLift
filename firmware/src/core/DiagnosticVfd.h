#pragma once
#include "Em01.h"
namespace lift {
// Deliberately no RUN or WRITE entry point. Safety state is not a communications gate.
class DiagnosticVfd {
 public:
  void stop(uint32_t now) {
    protocol_.telemetry.valid = false;
    requestStop(now);
  }
  void receive(char b, uint32_t now) {
    protocol_.receive(b, now);
    if (!protocol_.busy())
      stopPending_ = false;
  }
  std::string transmit(uint32_t now) {
    if (!started_) {
      started_ = true;
      stop(now);
    }
    if (!protocol_.busy()) {
      if (protocol_.result == Em01::Result::Timeout)
        failed = true;
      // Refresh STOP even after failure, but never clear the latched failed session.
      // Periodic refresh does not invalidate fresh monitor evidence; a new STOP intent does.
      if (now - lastStopTx_ >= VfdTiming::StopRefreshMs) {
        requestStop(now);
      } else if (!failed && now - scheduled_ >= VfdTiming::TrafficSlotMs) {
        protocol_.request(readNext_ ? '5' : '0', 0, parameter_, now);
        if (readNext_) {
          parameter_ = (parameter_ + 1) % 17;
          if (parameter_ == 13)
            ++parameter_;  // RELE readback remains unresolved.
        }
        readNext_ = !readNext_;
      }
    }
    auto frame = protocol_.transmit(now);
    if (!frame.empty()) {
      scheduled_ = now;
      if (frame[1] == '3') {
        stopTransmitted = true;
        lastStopTx_ = now;
      }
    }
    if (protocol_.result == Em01::Result::Timeout)
      failed = true;
    return frame;
  }
  bool healthy(uint32_t now) const { return !failed && protocol_.healthy(now); }
  bool monitorStopped(uint32_t now) const {
    return healthy(now) && protocol_.confirmedStopped(now);
  }
  bool acknowledged() const { return protocol_.stopAcknowledged; }
  const char* watchdogState(uint32_t now) const {
    if (protocol_.parameters[12] < 0 ||
        now - protocol_.parameterSampleMs[12] > VfdTiming::WatchdogReadFreshMs)
      return "unknown";
    if (!protocol_.parameters[12])
      return "disabled";
    if (protocol_.parameters[12] < int(VfdTiming::MinWatchdogTenths))
      return "too-short";
    if (protocol_.parameters[12] > int(VfdTiming::MaxWatchdogTenths))
      return "outside-policy";
    return "within-software-policy";
  }
  const Em01& protocol() const { return protocol_; }
  bool stopTransmitted = false, failed = false;

 private:
  void requestStop(uint32_t now) {
    if (stopPending_ && protocol_.busy())
      return;
    protocol_.preemptStop(now);
    stopPending_ = true;
    stopTransmitted = false;
  }
  Em01 protocol_;
  bool started_ = false, readNext_ = false, stopPending_ = false;
  unsigned parameter_ = 12;  // Read the drive watchdog first, without changing it.
  uint32_t scheduled_ = 0, lastStopTx_ = 0;
};
}  // namespace lift
