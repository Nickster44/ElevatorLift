#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct NetworkSettings {
  char staSsid[33] = {};
  char staPassword[65] = {};
  char apSsid[33] = {};
  char apPassword[65] = {};
  uint32_t crc = 0;
};

class NetworkConfig {
 public:
  bool begin();
  bool load(NetworkSettings& settings);
  bool save(const NetworkSettings& settings);
  bool clear();

 private:
  Preferences preferences_;
};

