#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace lift {
enum class State { Unknown, Idle, Moving, Homing, Service, Calibrating, Stopping, Fault };
enum class Source : uint8_t { Web, Automation, Rf, Local };
struct Inputs {
  bool hardwareReady = false, safety = false, limitsKnown = false, upper = false, lower = false;
  bool key = false, hold = false, up = false, down = false, home = false;
  bool encoderHealthy = false, communicationHealthy = false, storageHealthy = false;
  bool stopped = false;  // Fresh monitor status=0 AND frequency=0, never STOP ACK.
  int64_t position = 0;
  uint16_t frequency = 0;
  uint32_t sampleMs = 0;
};
struct Calibration {
  bool valid = false;
  uint32_t distance = 0, revision = 0;
  int direction = 0;
  int64_t start = 0, stop = 0, final = 0;
};
struct Config {
  std::array<int64_t, 3> floors{{0, 0, 0}};
  int64_t home = 0;
  bool floorsValid = false;
  uint16_t speed = 450, jog = 100, homing = 100, decel = 10, maxFrequency = 1200;
  uint32_t scale = 4, revision = 1, tolerance = 100;
  uint32_t progressTimeout = 1000, maxRunMs = 60000, stopTimeout = 5000;
  uint32_t calibrationCruiseMs = 3000, calibrationMargin = 5000;
  Calibration calibration;
};
struct Batch {
  bool stop = false;
  unsigned floorMask = 0;
  Source source = Source::Web;
  bool toggleLight = false;
};
class Supervisor {
 public:
  State state = State::Unknown;
  Config config;
  Inputs inputs;
  const char* fault = "";
  bool positionValid = false, light = false;
  int direction = 0, targetFloor = 0;
  int64_t target = 0;
  bool stopRequested = true;
  void setLight(bool on) { light = on; }
  bool referencePending = false;
  int64_t referencePosition = 0;
  void confirmReference(bool applied) {
    if (!referencePending)
      return;
    referencePending = false;
    positionValid = applied;
    if (applied) {
      previousPosition_ = referencePosition;
      inputs.position = referencePosition;
      state = State::Idle;
    } else
      fail("reference_apply_failed");
  }
  void tick(const Inputs& next, uint32_t now);
  const char* command(const Batch& batch, uint32_t now);
  const char* home(uint32_t now);
  const char* calibrate(uint32_t now);
  const char* configure(const Config& next);
  const char* resetFault();
  void fail(const char* reason);
  void stop(uint32_t now);
  bool guarded() const;
  bool permitted() const;
  bool stationary() const { return stable_ && inputs.stopped && inputs.communicationHealthy; }
  int currentFloor() const;
  bool runRequested() const;
  uint16_t runSpeed() const;

 private:
  uint32_t started_ = 0, progress_ = 0, stableSince_ = 0, stopping_ = 0, cruise_ = 0;
  bool sampled_ = false, stable_ = false, homeEdge_ = false, previousHome_ = false,
       cruiseSeen_ = false;
  bool serviceArmed_ = false;
  int64_t previousPosition_ = 0, homeEdgePosition_ = 0;
  enum class Completion { None, Landing, Home, Calibration } completion_ = Completion::None;
  const char* readiness() const;
  void start(State s, int dir, uint32_t now);
};
class CommandArbiter {
 public:
  void submit(const Batch& request) {
    pending_.stop |= request.stop;
    pending_.floorMask |= request.floorMask;
    pending_.source = request.source;
    pending_.toggleLight |= request.toggleLight;
  }
  Batch take() {
    Batch b = pending_;
    pending_ = {};
    return b;
  }

 private:
  Batch pending_;
};
const char* stateName(State state);
bool parseUnsigned(const char* text, uint32_t max, uint32_t& out);
}  // namespace lift
