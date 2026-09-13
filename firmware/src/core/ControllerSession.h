#pragma once
#include "Configuration.h"
#include "SafetyLedger.h"
#include "Supervisor.h"

namespace lift {
// Transaction coordinator used by host integration tests. Target motion binding
// is deliberately withheld until the hardware contract has an operational profile.
class ControllerSession {
 public:
  ControllerSession(ConfigurationStore& configurations, SafetyLedger& ledger)
      : configurations_(configurations), ledger_(ledger) {}
  Supervisor supervisor;
  Configuration configuration;
  bool programming = false;
  const char* lastResult = "";
  bool load() {
    bool loaded = configurations_.load(configuration);
    if (loaded)
      supervisor.config = configuration.motion;
    if (ledger_.load() && (ledger_.faultLatched || ledger_.unfinishedMotion))
      supervisor.fail("unclean_recovery");
    // No retained count can prove no motion occurred with the incremental counter off.
    supervisor.positionValid = false;
    return loaded;
  }
  const char* configure(const Configuration& next) {
    if (supervisor.inputs.hold)
      return "release_service_controls";
    Supervisor candidate = supervisor;
    if (auto error = candidate.configure(next.motion))
      return error;
    Configuration updated = next;
    updated.motion = candidate.config;
    if (!configurations_.save(updated)) {
      supervisor.fail("configuration_commit_failed");
      return supervisor.fault;
    }
    configuration = updated;
    supervisor = candidate;
    return nullptr;
  }
  const char* beginProgramming() {
    if (!supervisor.guarded() || supervisor.inputs.hold)
      return "programming_not_ready";
    programming = true;
    return nullptr;
  }
  const char* leaveProgramming(uint32_t now) {
    if (!programming)
      return "not_programming";
    if (auto error = supervisor.calibrate(now))
      return error;
    if (!prepare())
      return supervisor.fault;
    programming = false;
    return nullptr;
  }
  const char* home(uint32_t now) {
    if (auto error = supervisor.home(now))
      return error;
    return prepare() ? nullptr : supervisor.fault;
  }
  const char* request(const Batch& command, uint32_t now) {
    if (programming && !command.stop && !command.toggleLight)
      return "programming_active";
    const bool before = supervisor.runRequested();
    auto error = supervisor.command(command, now);
    if (!error && !before && supervisor.runRequested() && !prepare())
      return supervisor.fault;
    return error;
  }
  void submit(const Batch& command) {
    arbiter_.submit(command);
    pending_ = true;
  }
  void tick(const Inputs& inputs, uint32_t now) {
    const bool wasRunning = supervisor.runRequested();
    const bool hadCalibration = supervisor.config.calibration.valid;
    supervisor.tick(inputs, now);
    if (!wasRunning && supervisor.runRequested())
      prepare();
    if (pending_) {
      auto command = arbiter_.take();
      pending_ = false;
      const char* error = request(command, now);
      lastResult = error ? error : "accepted";
    }
    if (*supervisor.fault && !ledger_.faultLatched && !ledger_.latchFault())
      supervisor.fail("fault_commit_failed");
    if (ledger_.unfinishedMotion && !supervisor.runRequested() &&
        supervisor.state != State::Stopping && supervisor.stationary()) {
      if (!ledger_.confirmStop(true))
        supervisor.fail("stop_commit_failed");
    }
    if (!hadCalibration && supervisor.config.calibration.valid) {
      configuration.motion = supervisor.config;
      if (!configurations_.save(configuration)) {
        supervisor.config.calibration.valid = false;
        supervisor.fail("calibration_commit_failed");
      }
    }
  }
  const char* resetFault() {
    Supervisor candidate = supervisor;
    if (auto error = candidate.resetFault())
      return error;
    if (!ledger_.reset(true))
      return "fault_reset_commit_failed";
    supervisor = candidate;
    return nullptr;
  }
  bool mayReboot() const {
    return supervisor.guarded() && !pending_ && !programming && !ledger_.unfinishedMotion;
  }
  bool invalidateCalibration() {
    if (!supervisor.guarded() || supervisor.config.revision == 0xffffffff)
      return false;
    Configuration next = configuration;
    next.motion = supervisor.config;
    next.motion.calibration.valid = false;
    ++next.motion.revision;
    if (!configurations_.save(next)) {
      supervisor.fail("calibration_invalidation_commit_failed");
      return false;
    }
    configuration = next;
    supervisor.config = next.motion;
    return true;
  }

 private:
  ConfigurationStore& configurations_;
  SafetyLedger& ledger_;
  CommandArbiter arbiter_;
  bool pending_ = false;
  bool prepare() {
    if (!ledger_.prepareMotion()) {
      supervisor.fail("motion_intent_commit_failed");
      return false;
    }
    return true;
  }
};
}  // namespace lift
