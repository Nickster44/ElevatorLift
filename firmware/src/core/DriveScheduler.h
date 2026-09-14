#pragma once
#include "Em01.h"
#include "Supervisor.h"
namespace lift {
// Operational scheduler remains isolated from the inhibited target.
class DriveScheduler {
 public:
  Em01 protocol;
  void tick(Supervisor& supervisor, uint32_t now) {
    if (protocol.result == Em01::Result::Timeout || protocol.result == Em01::Result::Mismatch)
      supervisor.fail("vfd_transaction_failed");
    if (protocol.telemetry.valid && protocol.telemetry.status >= 3)
      supervisor.fail("vfd_alarm");
    if (!supervisor.inputs.hardwareReady)
      return;
    // STOP preempts any pending operation; protocol failure remains latched.
    if (supervisor.stopRequested && !stopping_) {
      stopping_ = true;
      protocol.preemptStop(now);
      monitorNext_ = true;
    }
    if (!protocol.busy() && now - lastRequest_ >= VfdTiming::TrafficSlotMs) {
      if (monitorNext_)
        protocol.request('0', 0, 0, now);
      else
        protocol.request(supervisor.runRequested() ? (supervisor.direction > 0 ? '1' : '2') : '3',
                         supervisor.runSpeed(), 0, now);
      monitorNext_ = !monitorNext_;
      lastRequest_ = now;
    }
    if (supervisor.runRequested())
      stopping_ = false;
  }

 private:
  bool monitorNext_ = false, stopping_ = false;
  uint32_t lastRequest_ = 0;
};
}  // namespace lift
