#include "Configuration.h"

#include <cstring>
namespace lift {
namespace {
void put64(uint8_t* b, int64_t v) {
  put32(b, uint64_t(v));
  put32(b + 4, uint64_t(v) >> 32);
}
int64_t get64(const uint8_t* b) {
  uint64_t v = uint64_t(get32(b)) | (uint64_t(get32(b + 4)) << 32);
  int64_t s;
  std::memcpy(&s, &v, 8);
  return s;
}
bool sane(const Configuration& c) {
  const auto& m = c.motion;
  if (!c.rfEpoch || !m.revision || !m.speed || m.speed > 2000 || !m.jog || m.jog > m.speed ||
      !m.homing || m.homing > m.jog || !m.scale || !m.decel || m.decel > 599 ||
      m.maxFrequency < m.speed || m.maxFrequency > 2000)
    return false;
  if (m.floorsValid &&
      (!(m.floors[0] < m.floors[1] && m.floors[1] < m.floors[2]) || m.floors[0] < -1000000000LL ||
       m.floors[2] > 1000000000LL || m.home < m.floors[1] || m.home > m.floors[2]))
    return false;
  if (m.calibration.valid &&
      (!m.floorsValid || !m.calibration.distance || m.calibration.distance > m.calibrationMargin ||
       m.calibration.revision != m.revision ||
       (m.calibration.direction != 1 && m.calibration.direction != -1)))
    return false;
  for (const auto& name : c.names)
    if (!std::memchr(name.data(), 0, name.size()))
      return false;
  return true;
}
}  // namespace
bool ConfigurationStore::save(const Configuration& c) {
  if (!sane(c))
    return false;
  uint8_t b[256] = {};
  const auto& m = c.motion;
  put32(b, 1);
  put32(b + 4, c.rfEpoch);
  put32(b + 8, m.revision);
  put32(b + 12, m.speed);
  put32(b + 16, m.jog);
  put32(b + 20, m.homing);
  put32(b + 24, m.decel);
  put32(b + 28, m.maxFrequency);
  put32(b + 32, m.scale);
  b[36] = m.floorsValid;
  for (int i = 0; i < 3; ++i) {
    put64(b + 40 + i * 8, m.floors[i]);
    std::memcpy(b + 64 + i * 32, c.names[i].data(), 32);
  }
  put64(b + 160, m.home);
  b[168] = m.calibration.valid;
  b[169] = m.calibration.direction == 1 ? 1 : 2;
  put32(b + 172, m.calibration.distance);
  put32(b + 176, m.calibration.revision);
  put64(b + 180, m.calibration.start);
  put64(b + 188, m.calibration.stop);
  put64(b + 196, m.calibration.final);
  put32(b + 204, m.tolerance);
  return journal_.append(b, sizeof b);
}
bool ConfigurationStore::load(Configuration& c) {
  uint8_t b[256];
  size_t n = sizeof b;
  if (!journal_.latest(b, n) || n != sizeof b || get32(b) != 1 || b[36] > 1 || b[168] > 1)
    return false;
  Configuration next;
  auto& m = next.motion;
  next.rfEpoch = get32(b + 4);
  m.revision = get32(b + 8);
  for (unsigned offset : {12u, 16u, 20u, 24u, 28u})
    if (get32(b + offset) > 65535)
      return false;
  m.speed = get32(b + 12);
  m.jog = get32(b + 16);
  m.homing = get32(b + 20);
  m.decel = get32(b + 24);
  m.maxFrequency = get32(b + 28);
  m.scale = get32(b + 32);
  m.floorsValid = b[36];
  for (int i = 0; i < 3; ++i) {
    m.floors[i] = get64(b + 40 + i * 8);
    std::memcpy(next.names[i].data(), b + 64 + i * 32, 32);
  }
  m.home = get64(b + 160);
  m.calibration.valid = b[168];
  m.calibration.direction = b[169] == 1 ? 1 : -1;
  m.calibration.distance = get32(b + 172);
  m.calibration.revision = get32(b + 176);
  m.calibration.start = get64(b + 180);
  m.calibration.stop = get64(b + 188);
  m.calibration.final = get64(b + 196);
  m.tolerance = get32(b + 204);
  if (!m.tolerance || m.tolerance > 1000 || !sane(next))
    return false;
  c = next;
  return true;
}
}  // namespace lift
