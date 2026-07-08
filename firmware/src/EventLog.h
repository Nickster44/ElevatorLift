#pragma once

#include <Arduino.h>

enum class EventCode : uint16_t {
  Boot = 1,
  MotionAccepted = 2,
  MotionRejected = 3,
  MotionStopped = 4,
  Fault = 5,
  SettingsChanged = 6,
  VfdParameterWrite = 7,
  VfdParameterRead = 8,
  NetworkChanged = 9,
};

struct EventRecord {
  uint32_t sequence = 0;
  uint32_t millisAtEvent = 0;
  EventCode code = EventCode::Boot;
  int32_t valueA = 0;
  int32_t valueB = 0;
};

class EventLog {
 public:
  void begin();
  void append(EventCode code, int32_t valueA = 0, int32_t valueB = 0);
  String jsonRecent() const;

 private:
  static constexpr size_t kCapacity = 32;
  EventRecord records_[kCapacity] = {};
  uint32_t nextSequence_ = 1;
  size_t writeIndex_ = 0;
  size_t count_ = 0;
};

