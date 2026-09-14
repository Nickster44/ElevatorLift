#pragma once
#include <cstdint>
namespace lift {
// Input intent and release-to-STOP only. This class never authorizes or emits RUN.
class ManualStop {
 public:
  bool key = false, hold = false, up = false, down = false, safety = false, known = false;
  int requestedDirection = 0;
  const char* update(int keyLevel, int holdLevel, int upLevel, int downLevel, int safetyLevel,
                     uint32_t now) {
    const bool previousKey = key, previousHold = hold, previousUp = up, previousDown = down;
    const bool previousSafety = safety, previousKnown = known;
    const bool gap = sampled_ && now - sampledMs_ > 25;
    key = keyLevel == 0;
    hold = holdLevel == 0;
    up = upLevel == 0;
    down = downLevel == 0;
    safety = safetyLevel == 0;
    known = binary(keyLevel) && binary(holdLevel) && binary(upLevel) && binary(downLevel) &&
            binary(safetyLevel);
    requestedDirection = known && !gap && key && hold && safety && up != down ? (up ? 1 : -1) : 0;
    const bool first = !sampled_;
    sampled_ = true;
    sampledMs_ = now;
    if (first)
      return "manual_startup_stop";
    if (gap)
      return "manual_sample_gap";
    if (!known && previousKnown)
      return "manual_input_unknown";
    if (!safety && previousSafety)
      return "manual_safety_lost";
    if (!key && previousKey)
      return "manual_key_released";
    if (key && !previousKey)
      return "manual_takeover_stop";
    if ((key || previousKey) && previousHold && !hold)
      return "manual_hold_released";
    if ((key || previousKey) && (up != previousUp || down != previousDown))
      return up && down ? "manual_direction_conflict" : "manual_direction_changed";
    return nullptr;
  }

 private:
  static bool binary(int level) { return level == 0 || level == 1; }
  bool sampled_ = false;
  uint32_t sampledMs_ = 0;
};
}  // namespace lift
