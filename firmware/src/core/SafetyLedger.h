#pragma once
#include <cstring>

#include "Devices.h"
namespace lift {
// Durable motion intent is committed BEFORE the first RUN. A reset cannot turn
// an unfinished motion/fault transaction into a clean stopped recovery record.
class SafetyLedger {
 public:
  explicit SafetyLedger(Journal& records) : records_(records) {}
  bool faultLatched = false, unfinishedMotion = false;
  bool load() {
    uint8_t data[16];
    size_t n = sizeof data;
    if (!records_.latest(data, n))
      return false;
    if (n != sizeof data || get32(data) != 1 || data[4] > 1 || data[5] > 1)
      return false;
    faultLatched = data[4];
    unfinishedMotion = data[5];
    return true;
  }
  bool prepareMotion() {
    if (faultLatched || unfinishedMotion)
      return false;
    unfinishedMotion = true;
    return save();
  }
  bool latchFault() {
    faultLatched = true;
    return save();
  }
  bool confirmStop(bool confirmed) {
    if (!confirmed)
      return false;
    unfinishedMotion = false;
    return save();
  }
  bool reset(bool guarded) {
    if (!guarded || unfinishedMotion)
      return false;
    faultLatched = false;
    if (!save()) {
      faultLatched = true;
      return false;
    }
    return true;
  }

 private:
  Journal& records_;
  bool save() {
    uint8_t data[16] = {};
    put32(data, 1);
    data[4] = faultLatched;
    data[5] = unfinishedMotion;
    return records_.append(data, sizeof data);
  }
};
}  // namespace lift
