#include "PositionStore.h"

namespace {

constexpr const char* kNamespace = "lift";
constexpr const char* kStateKey = "state";
constexpr uint32_t kMagic = 0x4C465431;

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

uint32_t stateCrc(StoredLiftState state) {
  state.crc = 0;
  return crc32Simple(reinterpret_cast<const uint8_t*>(&state), sizeof(state));
}

}  // namespace

bool PositionStore::begin() {
  return preferences_.begin(kNamespace, false);
}

bool PositionStore::load(StoredLiftState& state) {
  if (!preferences_.isKey(kStateKey)) {
    return false;
  }

  const size_t read = preferences_.getBytes(kStateKey, &state, sizeof(state));
  if (read != sizeof(state)) {
    return false;
  }
  if (state.magic != kMagic || state.version != 1) {
    return false;
  }

  return state.crc == stateCrc(state);
}

bool PositionStore::save(const StoredLiftState& state) {
  StoredLiftState copy = state;
  copy.magic = kMagic;
  copy.version = 1;
  copy.crc = stateCrc(copy);
  return preferences_.putBytes(kStateKey, &copy, sizeof(copy)) == sizeof(copy);
}

