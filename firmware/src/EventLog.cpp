#include "EventLog.h"

void EventLog::begin() {
  nextSequence_ = 1;
  writeIndex_ = 0;
  count_ = 0;
}

void EventLog::append(EventCode code, int32_t valueA, int32_t valueB) {
  EventRecord& record = records_[writeIndex_];
  record.sequence = nextSequence_++;
  record.millisAtEvent = millis();
  record.code = code;
  record.valueA = valueA;
  record.valueB = valueB;

  writeIndex_ = (writeIndex_ + 1) % kCapacity;
  if (count_ < kCapacity) {
    ++count_;
  }
}

String EventLog::jsonRecent() const {
  String json = "[";
  for (size_t i = 0; i < count_; ++i) {
    const size_t index = (writeIndex_ + kCapacity - count_ + i) % kCapacity;
    const EventRecord& record = records_[index];
    if (i > 0) {
      json += ",";
    }
    json += "{\"seq\":";
    json += record.sequence;
    json += ",\"ms\":";
    json += record.millisAtEvent;
    json += ",\"code\":";
    json += static_cast<uint16_t>(record.code);
    json += ",\"a\":";
    json += record.valueA;
    json += ",\"b\":";
    json += record.valueB;
    json += "}";
  }
  json += "]";
  return json;
}

