#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct StoredLiftState {
  uint32_t magic = 0x4C465431;  // LFT1
  uint16_t version = 1;
  uint16_t reserved = 0;
  uint32_t bootCount = 0;
  int64_t currentPosition = 0;
  uint32_t crc = 0;
};

class PositionStore {
 public:
  bool begin();
  bool load(StoredLiftState& state);
  bool save(const StoredLiftState& state);

 private:
  Preferences preferences_;
};

