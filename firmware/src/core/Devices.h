#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace lift {
class SpiBus {
 public:
  virtual ~SpiBus() = default;
  // Each call owns the bus and one CS for exactly one instruction. No ISR access.
  virtual bool transfer(int cs, const uint8_t* tx, uint8_t* rx, size_t length) = 0;
};
class ByteStore {
 public:
  virtual ~ByteStore() = default;
  virtual bool read(uint32_t address, uint8_t* data, size_t length) = 0;
  virtual bool write(uint32_t address, const uint8_t* data, size_t length) = 0;
};
class Counter {
 public:
  Counter(SpiBus& bus, int cs) : bus_(bus), cs_(cs) {}
  bool begin();
  bool sample(int64_t& position);
  void establishOrigin(int64_t value) { position_ = value; }

 private:
  SpiBus& bus_;
  int cs_;
  bool first_ = true;
  uint32_t previous_ = 0;
  int64_t position_ = 0;
  bool reg(uint8_t command, uint8_t& value);
};
class Mram : public ByteStore {
 public:
  static constexpr uint32_t Capacity = 524288;
  Mram(SpiBus& bus, int cs) : bus_(bus), cs_(cs) {}
  bool begin();
  bool read(uint32_t address, uint8_t* data, size_t length) override;
  bool write(uint32_t address, const uint8_t* data, size_t length) override;

 private:
  SpiBus& bus_;
  int cs_;
  bool ready_ = false;
  bool access(bool write, uint32_t address, uint8_t* data, size_t length);
};
class I2cBus {
 public:
  virtual ~I2cBus() = default;
  virtual bool read(uint8_t device, uint8_t reg, uint8_t* out, size_t n) = 0;
};
class Rtc {
 public:
  explicit Rtc(I2cBus& bus) : bus_(bus) {}
  bool readUnix(uint32_t& value);

 private:
  I2cBus& bus_;
};
uint32_t crc32(const uint8_t* data, size_t n);
void put32(uint8_t* data, uint32_t value);
uint32_t get32(const uint8_t* data);
// Explicit little-endian wire records, not compiler-dependent struct blobs.
class Journal {
 public:
  static constexpr size_t SlotSize = 576, PayloadMax = 544;
  Journal(ByteStore& store, uint32_t base, uint16_t slots)
      : store_(store), base_(base), slots_(slots) {}
  bool recover();
  bool append(const uint8_t* payload, size_t n);
  bool latest(uint8_t* payload, size_t& n);
  bool readSequence(uint32_t sequence, uint8_t* payload, size_t& n);
  uint32_t sequence() const { return sequence_; }

 private:
  ByteStore& store_;
  uint32_t base_;
  uint16_t slots_;
  uint32_t sequence_ = 0;
  bool recovered_ = false;
  bool decode(uint16_t slot, std::array<uint8_t, SlotSize>& data);
};
}  // namespace lift
