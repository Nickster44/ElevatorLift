#include "NetworkConfig.h"

#include <cstring>

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

bool validStrings(const NetworkSettings& s) {
  return std::memchr(s.staSsid, 0, sizeof s.staSsid) &&
         std::memchr(s.staPassword, 0, sizeof s.staPassword) &&
         std::memchr(s.apSsid, 0, sizeof s.apSsid) &&
         std::memchr(s.apPassword, 0, sizeof s.apPassword) && std::strlen(s.apSsid) > 0 &&
         std::strlen(s.apPassword) >= 8;
}

}  // namespace

bool NetworkConfig::begin() { return preferences_.begin(kNamespace, false); }

bool NetworkConfig::load(NetworkSettings& settings) {
  if (!preferences_.isKey(kSettingsKey)) {
    return false;
  }

  NetworkSettings candidate;
  const size_t read = preferences_.getBytes(kSettingsKey, &candidate, sizeof(candidate));
  if (read != sizeof(candidate)) {
    return false;
  }

  if (candidate.crc != settingsCrc(candidate) || !validStrings(candidate))
    return false;
  settings = candidate;
  return true;
}

bool NetworkConfig::save(const NetworkSettings& settings) {
  if (!validStrings(settings))
    return false;
  NetworkSettings copy = settings;
  copy.crc = settingsCrc(copy);
  return preferences_.putBytes(kSettingsKey, &copy, sizeof(copy)) == sizeof(copy);
}

bool NetworkConfig::clear() { return preferences_.remove(kSettingsKey); }
