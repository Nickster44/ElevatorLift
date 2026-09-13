#include "Devices.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
namespace lift {
bool Counter::reg(uint8_t command, uint8_t& value) {
  uint8_t tx[2] = {command, value}, rx[2] = {};
  if (!bus_.transfer(cs_, tx, rx, 2))
    return false;
  if ((command & 0xc0) == 0x40)
    value = rx[1];
  return true;
}
bool Counter::begin() {
  uint8_t v = 3;
  if (!reg(0x88, v))
    return false;
  v = 0;
  if (!reg(0x90, v))
    return false;
  v = 0;
  if (!reg(0x48, v) || v != 3)
    return false;
  if (!reg(0x50, v) || v != 0)
    return false;
  uint8_t clear = 0x30, rx = 0;
  first_ = true;
  return bus_.transfer(cs_, &clear, &rx, 1);  // Clear PLS once; later power loss is a fault.
}
bool Counter::sample(int64_t& position) {
  uint8_t v = 0;
  if (!reg(0x48, v) || v != 3 || !reg(0x50, v) || v != 0 || !reg(0x70, v) || (v & 4) || !(v & 8))
    return false;
  uint8_t tx[5] = {0x60, 0, 0, 0, 0}, rx[5] = {};
  if (!bus_.transfer(cs_, tx, rx, 5))
    return false;
  uint32_t raw = (uint32_t(rx[1]) << 24) | (uint32_t(rx[2]) << 16) | (uint32_t(rx[3]) << 8) | rx[4];
  if (first_) {
    previous_ = raw;
    first_ = false;
  }
  uint32_t unsignedDelta = raw - previous_;
  int64_t delta =
      unsignedDelta <= 0x7fffffff ? unsignedDelta : int64_t(unsignedDelta) - 0x100000000LL;
  // Provisional plausibility bound per sample, to be field-derived before deployment.
  if (std::llabs(delta) > 1000000)
    return false;
  position_ += delta;
  previous_ = raw;
  position = position_;
  return true;
}
bool Mram::begin() {
  ready_ = false;
  // Do not reset/change modes silently. Unknown latency/protection is a hard failure.
  uint8_t tx[20] = {0x9f, 0, 0, 0}, rx[20] = {};
  if (!bus_.transfer(cs_, tx, rx, 20) || rx[4] != 0x29 || rx[5] != 0x55)
    return false;
  for (uint8_t r = 0; r < 3; ++r) {
    uint8_t t[5] = {0xb5, 0, 0, r, 0}, o[5] = {};
    if (!bus_.transfer(cs_, t, o, 5))
      return false;
    if ((r == 0 && (o[4] & 0x8c)) || (r == 1 && (o[4] & 0x18)) || (r == 2 && (o[4] & 0x60)))
      return false;
  }
  ready_ = true;
  return true;
}
bool Mram::access(bool writing, uint32_t address, uint8_t* data, size_t n) {
  if (!ready_ || !n || n > 64 || (address & 1) || (n & 1) || address >= Capacity ||
      n > Capacity - address)
    return false;
  if (writing) {
    uint8_t enable = 0x06, rx = 0;
    if (!bus_.transfer(cs_, &enable, &rx, 1))
      return false;
  }
  uint32_t word = address / 2;
  uint8_t tx[68] = {}, rx[68] = {};
  tx[0] = writing ? 0x02 : 0x03;
  tx[1] = word >> 16;
  tx[2] = word >> 8;
  tx[3] = word;
  if (writing)
    std::memcpy(tx + 4, data, n);
  bool ok = bus_.transfer(cs_, tx, rx, n + 4);
  if (writing) {
    uint8_t disable = 0x04, out = 0;
    ok = bus_.transfer(cs_, &disable, &out, 1) && ok;
  } else if (ok)
    std::memcpy(data, rx + 4, n);
  return ok;
}
bool Mram::read(uint32_t a, uint8_t* data, size_t n) {
  if ((a & 1) || (n & 1) || a >= Capacity || n > Capacity - a)
    return false;
  while (n) {
    size_t chunk = std::min(n, size_t(64));
    if (!access(false, a, data, chunk))
      return false;
    a += chunk;
    data += chunk;
    n -= chunk;
  }
  return true;
}
bool Mram::write(uint32_t a, const uint8_t* data, size_t n) {
  if ((a & 1) || (n & 1) || a >= Capacity || n > Capacity - a)
    return false;
  while (n) {
    size_t chunk = std::min(n, size_t(64));
    uint8_t copy[64], check[64];
    std::memcpy(copy, data, chunk);
    if (!access(true, a, copy, chunk) || !access(false, a, check, chunk) ||
        std::memcmp(copy, check, chunk))
      return false;
    a += chunk;
    data += chunk;
    n -= chunk;
  }
  return true;
}
bool Rtc::readUnix(uint32_t& value) {
  uint8_t status = 0, a[4], b[4];
  if (!bus_.read(0x52, 0x0e, &status, 1) || (status & 1))
    return false;
  if (!bus_.read(0x52, 0x1b, a, 4) || !bus_.read(0x52, 0x1b, b, 4))
    return false;
  if (std::memcmp(a, b, 4) || !bus_.read(0x52, 0x0e, &status, 1) || (status & 1))
    return false;
  uint32_t unixTime = get32(a);
  if (!unixTime || unixTime == 0xffffffff)
    return false;
  value = unixTime;
  return true;
}
void put32(uint8_t* p, uint32_t v) {
  for (unsigned i = 0; i < 4; ++i)
    p[i] = v >> (8 * i);
}
uint32_t get32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
uint32_t crc32(const uint8_t* p, size_t n) {
  uint32_t c = ~0u;
  while (n--) {
    c ^= *p++;
    for (int i = 0; i < 8; ++i)
      c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
  }
  return ~c;
}
bool Journal::decode(uint16_t slot, std::array<uint8_t, SlotSize>& b) {
  return store_.read(base_ + slot * SlotSize, b.data(), b.size()) &&
         get32(b.data()) == 0x4c494654 && get32(b.data() + 4) == 1 &&
         get32(b.data() + 12) <= PayloadMax && get32(b.data() + 8) > 0 &&
         get32(b.data() + 572) == 0xc01117ed && get32(b.data() + 568) == crc32(b.data(), 568);
}
bool Journal::recover() {
  if (slots_ < 2 || base_ > Mram::Capacity || uint32_t(slots_) * SlotSize > Mram::Capacity - base_)
    return false;
  sequence_ = 0;
  std::array<uint8_t, SlotSize> b{};
  for (uint16_t i = 0; i < slots_; ++i)
    if (decode(i, b))
      sequence_ = std::max(sequence_, get32(b.data() + 8));
  recovered_ = true;
  return true;
}
bool Journal::append(const uint8_t* p, size_t n) {
  if (!recovered_ || n > PayloadMax || sequence_ == 0xffffffff)
    return false;
  std::array<uint8_t, SlotSize> b{};
  uint32_t seq = sequence_ + 1, address = base_ + (seq % slots_) * SlotSize;
  put32(b.data(), 0x4c494654);
  put32(b.data() + 4, 1);
  put32(b.data() + 8, seq);
  put32(b.data() + 12, n);
  std::memcpy(b.data() + 16, p, n);
  put32(b.data() + 568, crc32(b.data(), 568));
  uint8_t zero[4] = {};
  // Invalidate destination first, commit last; the previous record remains intact.
  if (!store_.write(address + 572, zero, 4) || !store_.write(address, b.data(), 572))
    return false;
  put32(b.data() + 572, 0xc01117ed);
  if (!store_.write(address + 572, b.data() + 572, 4))
    return false;
  std::array<uint8_t, SlotSize> check{};
  if (!decode(seq % slots_, check) || check != b)
    return false;
  sequence_ = seq;
  return true;
}
bool Journal::readSequence(uint32_t seq, uint8_t* p, size_t& n) {
  std::array<uint8_t, SlotSize> b{};
  if (!recovered_ || !seq || !decode(seq % slots_, b) || get32(b.data() + 8) != seq ||
      get32(b.data() + 12) > n)
    return false;
  n = get32(b.data() + 12);
  std::memcpy(p, b.data() + 16, n);
  return true;
}
bool Journal::latest(uint8_t* p, size_t& n) { return readSequence(sequence_, p, n); }
}  // namespace lift
