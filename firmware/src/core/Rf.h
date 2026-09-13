#pragma once
#include <array>

#include "Supervisor.h"
namespace lift {
class Rf {
 public:
  struct Observation {
    bool known = false;
    uint8_t slot = 0;
    uint32_t epoch = 0, atMs = 0;
  };
  Observation last;
  uint32_t epoch = 1;
  bool learnOutput = false;
  enum class LearnState { Idle, Pulse, AwaitIndicator, Learning, Unconfirmed };
  LearnState learning = LearnState::Idle;
  enum class EraseState { Idle, Holding, AwaitHigh, ConfirmHigh, Confirmed, Unconfirmed };
  EraseState erasing = EraseState::Idle;
  std::array<uint32_t, 256> associated{};
  void capture(uint8_t slot, uint32_t now) { last = {true, slot, epoch, now}; }
  void invalidateAssociations() {
    if (epoch != 0xffffffff)
      ++epoch;
    associated.fill(0);
    last.known = false;
  }
  bool associate(uint8_t slot) {
    if (!last.known || last.slot != slot || last.epoch != epoch)
      return false;
    associated[slot] = epoch;
    return true;
  }
  bool beginLearn(bool guarded, uint32_t now) {
    if (!guarded || learning != LearnState::Idle || erasing != EraseState::Idle ||
        epoch == 0xffffffff)
      return false;
    invalidateAssociations();
    learning = LearnState::Pulse;
    learnOutput = true;
    started_ = now;
    return true;
  }
  bool beginErase(bool guarded, bool explicitConfirmation, uint32_t now) {
    if (!guarded || !explicitConfirmation || learning != LearnState::Idle ||
        erasing != EraseState::Idle || epoch == 0xffffffff)
      return false;
    invalidateAssociations();
    erasing = EraseState::Holding;
    learnOutput = true;
    started_ = now;
    indicatorHigh_ = false;
    return true;
  }
  void tickErase(bool indicator, bool guarded, uint32_t now) {
    if (erasing == EraseState::Idle || erasing == EraseState::Confirmed ||
        erasing == EraseState::Unconfirmed)
      return;
    auto abort = [&]() {
      learnOutput = false;
      erasing = EraseState::Unconfirmed;
    };
    if (!guarded) {
      abort();
      return;
    }
    if (erasing == EraseState::Holding) {
      indicatorHigh_ |= indicator;
      if (now - started_ >= 10000 && !indicator && indicatorHigh_) {
        learnOutput = false;
        erasing = EraseState::AwaitHigh;
        phase_ = now;
      } else if (now - started_ > 10500)
        abort();
    } else if (erasing == EraseState::AwaitHigh) {
      if (indicator) {
        erasing = EraseState::ConfirmHigh;
        phase_ = now;
      } else if (now - phase_ > 500)
        abort();
    } else if (erasing == EraseState::ConfirmHigh) {
      if (!indicator) {
        erasing = now - phase_ >= 1900 && now - phase_ <= 2500 ? EraseState::Confirmed
                                                               : EraseState::Unconfirmed;
      } else if (now - phase_ > 2500)
        abort();
    }
  }
  void tickLearn(bool indicator, bool guarded, uint32_t now) {
    if (learning == LearnState::Idle)
      return;
    if (!guarded) {
      learnOutput = false;
      learning = LearnState::Unconfirmed;
      return;
    }
    if (learning == LearnState::Pulse && now - started_ >= 50) {
      learnOutput = false;
      learning = LearnState::AwaitIndicator;
    }
    if (indicator)
      learning = LearnState::Learning;
    if (now - started_ >= 18000) {
      learnOutput = false;
      learning = indicator ? LearnState::Unconfirmed : LearnState::Idle;
    }
  }
  Batch buttons(unsigned mask, uint32_t now, bool mappingVerified) {
    Batch b;
    b.source = Source::Rf;
    if (!mask) {
      armed_ = true;
      return b;
    }
    if (!armed_)
      return b;
    armed_ = false;
    if (!mappingVerified)
      return b;
    if (mask & 8) {
      b.stop = true;
      return b;
    }
    if (!last.known || now - last.atMs > 200 || last.epoch != epoch ||
        associated[last.slot] != epoch)
      return b;
    b.floorMask = mask & 7;
    b.toggleLight = mask & 16;
    return b;
  }

 private:
  bool armed_ = false;
  uint32_t started_ = 0;
  uint32_t phase_ = 0;
  bool indicatorHigh_ = false;
};
}  // namespace lift
