#include "Supervisor.h"

#include <algorithm>

namespace lift {
const char* stateName(State s) {
  switch (s) {
    case State::Unknown:
      return "unknown-position";
    case State::Idle:
      return "idle";
    case State::Moving:
      return "moving";
    case State::Homing:
      return "homing";
    case State::Service:
      return "service";
    case State::Calibrating:
      return "calibrating";
    case State::Stopping:
      return "stopping";
    case State::Fault:
      return "fault";
  }
  return "fault";
}
bool parseUnsigned(const char* s, uint32_t max, uint32_t& out) {
  if (!s || !*s)
    return false;
  uint32_t n = 0;
  for (; *s; ++s) {
    if (*s < '0' || *s > '9')
      return false;
    unsigned d = *s - '0';
    if (d > max || n > (max - d) / 10)
      return false;
    n = n * 10 + d;
  }
  out = n;
  return true;
}
const char* Supervisor::readiness() const {
  if (!inputs.hardwareReady)
    return "hardware_unresolved";
  if (*fault)
    return fault;
  if (!inputs.safety)
    return "safety_open";
  if (!inputs.limitsKnown)
    return "limits_unknown";
  if (!inputs.keyKnown)
    return "service_key_unknown";
  if (inputs.upper && inputs.lower)
    return "limit_conflict";
  if (!inputs.encoderHealthy)
    return "encoder_unavailable";
  if (!inputs.communicationHealthy)
    return "vfd_unavailable";
  if (!inputs.storageHealthy)
    return "storage_unavailable";
  return nullptr;
}
bool Supervisor::guarded() const {
  return !readiness() && stationary() && !runRequested() && state != State::Stopping && inputs.key;
}
bool Supervisor::permitted() const {
  return !readiness() && state == State::Idle && positionValid && stationary() &&
         config.floorsValid && config.calibration.valid &&
         config.calibration.revision == config.revision && !inputs.key && !referencePending;
}
bool Supervisor::runRequested() const {
  return state == State::Moving || state == State::Homing || state == State::Service ||
         state == State::Calibrating;
}
uint16_t Supervisor::runSpeed() const {
  return state == State::Service  ? config.jog
         : state == State::Homing ? config.homing
                                  : config.speed;
}
void Supervisor::fail(const char* reason) {
  if (!*fault)
    fault = reason;
  state = State::Fault;
  stopRequested = true;
  direction = 0;
  positionValid = false;
  referencePending = false;
  completion_ = Completion::None;
}
void Supervisor::stop(uint32_t now) {
  stopRequested = true;
  if (state == State::Fault || state == State::Stopping)
    return;
  if (runRequested()) {
    state = State::Stopping;
    stopping_ = now;
    stable_ = false;
    stableSince_ = now;
  }
}
void Supervisor::start(State s, int dir, uint32_t now) {
  state = s;
  direction = dir;
  started_ = progress_ = now;
  stopRequested = false;
  stable_ = false;
  serviceArmed_ = false;
}
const char* Supervisor::command(const Batch& b, uint32_t now) {
  if (b.stop) {
    completion_ = Completion::None;
    stop(now);
    return nullptr;
  }
  if (b.toggleLight) {
    if (b.floorMask)
      return "conflicting_commands";
    setLight(!light);
    return nullptr;
  }
  if (!b.floorMask || (b.floorMask & ~7u) || (b.floorMask & (b.floorMask - 1)))
    return "conflicting_or_invalid_floor";
  if (auto r = readiness())
    return r;
  if (!permitted())
    return "motion_not_ready";
  int f = b.floorMask == 1 ? 1 : b.floorMask == 2 ? 2 : 3;
  const int64_t t = config.floors[f - 1];
  const int d = t > inputs.position ? 1 : -1;
  if ((d > 0 && inputs.upper) || (d < 0 && inputs.lower))
    return "directional_limit";
  if (std::llabs(t - inputs.position) <= config.tolerance)
    return nullptr;
  if (std::llabs(t - inputs.position) <= config.calibration.distance)
    return "insufficient_stopping_distance";
  target = t;
  targetFloor = f;
  completion_ = Completion::Landing;
  start(State::Moving, d, now);
  return nullptr;
}
const char* Supervisor::home(uint32_t now) {
  if (!guarded() || !inputs.hold || !config.floorsValid || inputs.upper || inputs.home)
    return "homing_requires_local_service_and_clear_home";
  homeEdge_ = false;
  previousHome_ = inputs.home;
  completion_ = Completion::Home;
  start(State::Homing, 1, now);
  return nullptr;
}
const char* Supervisor::calibrate(uint32_t now) {
  if (!guarded() || !inputs.hold || !positionValid || !config.floorsValid)
    return "calibration_not_ready";
  const int64_t up = config.floors[2] - inputs.position, down = inputs.position - config.floors[0];
  int d = up > down ? 1 : -1;
  if (std::max(up, down) <= config.calibrationMargin + config.tolerance ||
      (d > 0 ? inputs.upper : inputs.lower))
    return "insufficient_calibration_travel";
  config.calibration = {};
  config.calibration.direction = d;
  config.calibration.start = inputs.position;
  target = d > 0 ? config.floors[2] : config.floors[0];
  cruiseSeen_ = false;
  completion_ = Completion::Calibration;
  start(State::Calibrating, d, now);
  return nullptr;
}
const char* Supervisor::configure(const Config& next) {
  if (!guarded())
    return "settings_require_confirmed_stop_and_key";
  if (!next.speed || next.speed > 2000 || !next.jog || next.jog > next.speed || !next.homing ||
      next.homing > next.jog || !next.scale || !next.decel || next.decel > 599 ||
      next.maxFrequency < next.speed || next.maxFrequency > 2000 || !next.tolerance ||
      next.tolerance > 1000)
    return "invalid_settings";
  if (next.floorsValid && (!(next.floors[0] < next.floors[1] && next.floors[1] < next.floors[2]) ||
                           next.floors[0] < -1000000000LL || next.floors[2] > 1000000000LL ||
                           next.home < next.floors[1] || next.home > next.floors[2]))
    return "invalid_floor_coordinates";
  if (config.revision == 0xffffffff)
    return "configuration_revision_exhausted";
  const bool referenceChanged = next.scale != config.scale || next.home != config.home;
  const bool calibrationChanged = referenceChanged || next.floors != config.floors ||
                                  next.speed != config.speed || next.decel != config.decel ||
                                  next.maxFrequency != config.maxFrequency;
  Config updated = next;
  updated.revision = config.revision + 1;
  updated.calibration = config.calibration;
  if (calibrationChanged)
    updated.calibration.valid = false;
  else
    updated.calibration.revision = updated.revision;
  config = updated;
  if (referenceChanged) {
    positionValid = false;
    state = State::Unknown;
  }
  return nullptr;
}
const char* Supervisor::resetFault() {
  // Fault reset is deliberately independent of STOP and never restores position validity.
  if (!inputs.hardwareReady || !inputs.keyKnown || !inputs.key || inputs.hold || !inputs.safety ||
      !inputs.limitsKnown || inputs.upper || inputs.lower || !inputs.encoderHealthy ||
      !inputs.communicationHealthy || !inputs.storageHealthy || !stationary())
    return "fault_reset_not_ready";
  fault = "";
  state = State::Unknown;
  positionValid = false;
  return nullptr;
}
int Supervisor::currentFloor() const {
  if (!positionValid || !config.floorsValid || !stationary())
    return 0;
  for (int i = 0; i < 3; ++i)
    if (std::llabs(config.floors[i] - inputs.position) <= config.tolerance)
      return i + 1;
  return 0;
}
void Supervisor::tick(const Inputs& n, uint32_t now) {
  inputs = n;
  if (n.position < -1000000000000LL || n.position > 1000000000000LL) {
    fail("position_out_of_range");
    return;
  }
  if (!n.hold && !n.up && !n.down)
    serviceArmed_ = true;
  if (now - n.sampleMs > 100) {
    fail("stale_input_snapshot");
    return;
  }
  const int64_t delta = sampled_ ? n.position - previousPosition_ : 0;
  if (!sampled_ || delta != 0 || !n.stopped) {
    stableSince_ = now;
    stable_ = false;
  } else if (now - stableSince_ >= 300)
    stable_ = true;
  previousPosition_ = n.position;
  sampled_ = true;
  if (!n.hardwareReady) {
    stopRequested = true;
    if (runRequested())
      fail("hardware_unresolved");
    return;
  }
  if (!n.safety) {
    fail("safety_open");
    return;
  }
  if (!n.limitsKnown || !n.keyKnown || (n.upper && n.lower)) {
    fail("limit_conflict_or_unknown");
    return;
  }
  if (!n.encoderHealthy) {
    fail("encoder_unavailable");
    return;
  }
  if (!n.storageHealthy) {
    fail("storage_unavailable");
    return;
  }
  if (!n.communicationHealthy) {
    fail("communication_lost");
    return;
  }
  if (state == State::Fault)
    return;
  if (runRequested()) {
    if ((direction > 0 && n.upper) || (direction < 0 && n.lower)) {
      fail("directional_limit");
      return;
    }
    if (delta * direction < 0) {
      fail("wrong_direction");
      return;
    }
    if (delta * direction > 0)
      progress_ = now;
    if (now - progress_ > config.progressTimeout) {
      fail("encoder_no_progress");
      return;
    }
    if (now - started_ > config.maxRunMs) {
      fail("travel_timeout");
      return;
    }
    if (state != State::Moving && (!n.key || !n.hold || (n.up && n.down))) {
      completion_ = Completion::None;
      stop(now);
      return;
    }
    if (state == State::Moving && n.key) {
      completion_ = Completion::None;
      stop(now);
      return;
    }
    if (state == State::Service && (now - started_ > 5000 || (direction > 0 ? !n.up : !n.down))) {
      stop(now);
      return;
    }
    if (state == State::Homing && !previousHome_ && n.home) {
      homeEdge_ = true;
      homeEdgePosition_ = n.position;
      stop(now);
    }
    previousHome_ = n.home;
    if (state == State::Moving || state == State::Calibrating) {
      const int64_t remaining = (target - n.position) * direction;
      if (remaining < -static_cast<int64_t>(config.tolerance)) {
        fail("overshoot");
        return;
      }
      if (state == State::Moving && remaining <= config.calibration.distance)
        stop(now);
      else if (state == State::Calibrating) {
        if (remaining <= config.calibrationMargin) {
          fail("calibration_travel_exhausted");
          return;
        }
        if (n.frequency == config.speed) {
          if (!cruiseSeen_) {
            cruiseSeen_ = true;
            cruise_ = now;
          }
        } else
          cruiseSeen_ = false;
        if (cruiseSeen_ && now - cruise_ >= config.calibrationCruiseMs) {
          config.calibration.stop = n.position;
          stop(now);
        }
      }
    }
  } else if (state == State::Stopping) {
    if ((direction > 0 && n.upper) || (direction < 0 && n.lower)) {
      fail("directional_limit");
      return;
    }
    if (completion_ == Completion::Landing &&
        (target - n.position) * direction < -static_cast<int64_t>(config.tolerance)) {
      fail("overshoot");
      return;
    }
    if (now - stopping_ > config.stopTimeout) {
      fail("stop_unconfirmed");
      return;
    }
    if (stationary()) {
      if (completion_ == Completion::Landing &&
          std::llabs(n.position - target) > config.tolerance) {
        fail("landing_outside_tolerance");
        return;
      }
      if (completion_ == Completion::Home && homeEdge_) {
        // Caller applies this offset to the counter origin, preserving post-edge coast.
        referencePosition = config.home + (n.position - homeEdgePosition_);
        referencePending = true;
      }
      if (completion_ == Completion::Calibration) {
        int64_t distance = (n.position - config.calibration.stop) * direction;
        if (distance <= 0 || distance > config.calibrationMargin) {
          fail("invalid_calibration_distance");
          return;
        }
        config.calibration.distance = static_cast<uint32_t>(distance);
        config.calibration.final = n.position;
        config.calibration.revision = config.revision;
        config.calibration.valid = true;
      }
      completion_ = Completion::None;
      direction = 0;
      targetFloor = 0;
      state = positionValid ? State::Idle : State::Unknown;
    }
  } else {
    if (delta != 0) {
      fail("unexpected_idle_movement");
      return;
    }
    if (!n.stopped) {
      fail("unexpected_drive_running");
      return;
    }
    if (serviceArmed_ && n.key && n.hold && (n.up != n.down) && stationary() && !referencePending) {
      int d = n.up ? 1 : -1;
      if (!(d > 0 ? n.upper : n.lower)) {
        completion_ = Completion::None;
        start(State::Service, d, now);
      }
    }
  }
}
}  // namespace lift
