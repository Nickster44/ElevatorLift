#include "NetworkConfig.h"

namespace {

constexpr const char* kNamespace = "network";
constexpr const char* kSettingsKey = "settings";

uint32_t crc32Simple(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint32_t mask = -(crc & 1U);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

uint32_t settingsCrc(NetworkSettings settings) {
  settings.crc = 0;
  return crc32Simple(reinterpret_cast<const uint8_t*>(&settings), sizeof(settings));
}

}  // namespace

bool NetworkConfig::begin() {
  return preferences_.begin(kNamespace, false);
}

bool NetworkConfig::load(NetworkSettings& settings) {
  if (!preferences_.isKey(kSettingsKey)) {
    return false;
  }

  const size_t read = preferences_.getBytes(kSettingsKey, &settings, sizeof(settings));
  if (read != sizeof(settings)) {
    return false;
  }

  return settings.crc == settingsCrc(settings);
}

bool NetworkConfig::save(const NetworkSettings& settings) {
  NetworkSettings copy = settings;
  copy.crc = settingsCrc(copy);
  return preferences_.putBytes(kSettingsKey, &copy, sizeof(copy)) == sizeof(copy);
}

bool NetworkConfig::clear() {
  return preferences_.remove(kSettingsKey);
}

