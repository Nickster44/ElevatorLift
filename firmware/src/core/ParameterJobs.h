#pragma once
#include "ControllerSession.h"
#include "Em01.h"
#include "ParameterCatalog.h"
namespace lift {
class ParameterJobs {
 public:
  ParameterJobs(Em01& protocol, ControllerSession& session, Journal& cache)
      : protocol_(protocol), session_(session), cache_(cache) {
    values.fill(-1);
  }
  std::array<int, 17> values;
  std::array<uint32_t, 17> sampledMs{};
  bool active = false;
  uint32_t id = 0;
  const char* result = "idle";
  const char* start(bool write, unsigned number, unsigned value, ParameterAccess access,
                    uint32_t now) {
    if (active || protocol_.busy())
      return "vfd_busy";
    if (number >= 17)
      return "unknown_parameter";
    if (!session_.supervisor.guarded() || session_.supervisor.inputs.hold || session_.programming)
      return "vfd_requires_stopped_service";
    auto& definition = ParameterDefinitions[number];
    if (write) {
      if (!definition.writable)
        return "parameter_write_unsupported";
      if (static_cast<unsigned>(access) < static_cast<unsigned>(definition.accessLevel))
        return "installer_access_required";
      if (value < definition.minValue || value > definition.maxValue)
        return "parameter_out_of_range";
      if (number == 12 &&
          (value < VfdTiming::MinWatchdogTenths || value > VfdTiming::MaxWatchdogTenths))
        return "serial_timeout_policy";
      if (number == 10 && value < session_.supervisor.config.speed)
        return "maximum_below_run_speed";
      if (number == 11 && value > session_.supervisor.config.homing)
        return "minimum_above_homing_speed";
      // Persist invalidity BEFORE a command can change drive stopping behavior.
      if (!session_.invalidateCalibration())
        return "calibration_invalidation_failed";
    }
    if (id == 0xffffffff)
      return "job_id_exhausted";
    if (!protocol_.request(write ? '4' : '5', value, number, now))
      return "vfd_session_unavailable";
    ++id;
    active = true;
    number_ = number;
    write_ = write;
    requested_ = value;
    result = "pending";
    return nullptr;
  }
  void tick(uint32_t now) {
    if (!active)
      return;
    if (session_.supervisor.runRequested() || !session_.supervisor.inputs.key ||
        !session_.supervisor.inputs.safety) {
      session_.supervisor.fail("parameter_service_interrupted");
      protocol_.preemptStop(now);
      active = false;
      result = "interrupted";
      return;
    }
    if (protocol_.result == Em01::Result::Pending)
      return;
    active = false;
    if (protocol_.result != Em01::Result::Verified) {
      result = "transaction_failed";
      session_.supervisor.fail("parameter_transaction_failed");
      return;
    }
    values[number_] = protocol_.parameters[number_];
    sampledMs[number_] = now;
    if (write_ && (number_ == 4 || number_ == 10)) {
      auto configuration = session_.configuration;
      if (number_ == 4)
        configuration.motion.decel = requested_;
      else
        configuration.motion.maxFrequency = requested_;
      if (auto error = session_.configure(configuration)) {
        result = error;
        session_.supervisor.fail("parameter_configuration_commit_failed");
        return;
      }
    }
    uint8_t data[80] = {};
    put32(data, 1);
    for (unsigned n = 0; n < 17; ++n)
      put32(data + 4 + n * 4, uint32_t(values[n]));
    if (!cache_.append(data, sizeof data)) {
      result = "cache_commit_failed";
      session_.supervisor.fail("parameter_cache_commit_failed");
      return;
    }
    result = "verified";
  }
  bool loadCache() {
    uint8_t data[80];
    size_t n = sizeof data;
    if (!cache_.latest(data, n) || n != sizeof data || get32(data) != 1)
      return false;
    for (unsigned i = 0; i < 17; ++i) {
      uint32_t value = get32(data + 4 + i * 4);
      if (value != 0xffffffff && value > 9999)
        return false;
    }
    for (unsigned i = 0; i < 17; ++i) {
      uint32_t value = get32(data + 4 + i * 4);
      values[i] = value == 0xffffffff ? -1 : int(value);
      sampledMs[i] = 0;
    }
    return true;  // Restored values are stale; no restored communication health.
  }

 private:
  Em01& protocol_;
  ControllerSession& session_;
  Journal& cache_;
  unsigned number_ = 0, requested_ = 0;
  bool write_ = false;
};
}  // namespace lift
