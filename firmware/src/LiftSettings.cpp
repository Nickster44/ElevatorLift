#include "LiftSettings.h"

namespace {

constexpr const char* kNamespace = "settings";
constexpr const char* kSettingsKey = "lift";
constexpr uint32_t kMagic = 0x4C534531;

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

uint32_t settingsCrc(LiftSettingsData settings) {
  settings.crc = 0;
  return crc32Simple(reinterpret_cast<const uint8_t*>(&settings), sizeof(settings));
}

}  // namespace

bool LiftSettingsStore::begin() {
  return preferences_.begin(kNamespace, false);
}

LiftSettingsData LiftSettingsStore::defaults() const {
  return LiftSettingsData{};
}

bool LiftSettingsStore::load(LiftSettingsData& settings) {
  if (!preferences_.isKey(kSettingsKey)) {
    return false;
  }

  const size_t read = preferences_.getBytes(kSettingsKey, &settings, sizeof(settings));
  if (read != sizeof(settings)) {
    return false;
  }
  if (settings.magic != kMagic || settings.version != 1) {
    return false;
  }

  return settings.crc == settingsCrc(settings);
}

bool LiftSettingsStore::save(const LiftSettingsData& settings) {
  LiftSettingsData copy = settings;
  copy.magic = kMagic;
  copy.version = 1;
  copy.crc = settingsCrc(copy);
  return preferences_.putBytes(kSettingsKey, &copy, sizeof(copy)) == sizeof(copy);
}

