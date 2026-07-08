#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct LiftSettingsData {
  uint32_t magic = 0x4C534531;  // LSE1
  uint16_t version = 1;
  uint16_t normalRunTenthsHz = 450;
  uint16_t serviceJogTenthsHz = 250;
  uint16_t homingTenthsHz = 200;
  uint16_t stopOffsetCounts = 3500;
  uint32_t homingTimeoutMs = 30000;
  uint32_t logRetentionRecords = 2048;
  uint32_t crc = 0;
};

class LiftSettingsStore {
 public:
  bool begin();
  bool load(LiftSettingsData& settings);
  bool save(const LiftSettingsData& settings);
  LiftSettingsData defaults() const;

 private:
  Preferences preferences_;
};

