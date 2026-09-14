#pragma once
#include <cstdint>
namespace lift {
// Electrical stability is not evidence of field-wire continuity or NO/NC semantics.
class ActiveLowInput {
 public:
  bool known = false, active = false;
  void sample(int level, uint32_t now) {
    if (level != 0 && level != 1) {
      reset();
      return;
    }
    if (!seen_ || now - last_ > 25 || level != level_) {
      known = false;
      since_ = now;
    }
    seen_ = true;
    last_ = now;
    level_ = level;
    active = level == 0;
    if (now - since_ >= 100)
      known = true;
  }
  bool fresh(uint32_t now) const { return known && now - last_ <= 25; }
  void reset() { known = active = seen_ = false; }

 private:
  bool seen_ = false;
  int level_ = 1;
  uint32_t since_ = 0, last_ = 0;
};
struct FieldInputs {
  ActiveLowInput upper, lower, key;
  bool readingsKnown(uint32_t now) const {
    return upper.fresh(now) && lower.fresh(now) && key.fresh(now);
  }
  bool qualified(uint32_t now, bool continuityVerified) const {
    return continuityVerified && readingsKnown(now) && !(upper.active && lower.active);
  }
};
}  // namespace lift
