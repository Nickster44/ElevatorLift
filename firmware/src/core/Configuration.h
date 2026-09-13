#pragma once
#include "Devices.h"
#include "Supervisor.h"
namespace lift {
struct Configuration {
  Config motion;
  std::array<std::array<char, 32>, 3> names{};
  uint32_t rfEpoch = 1;
};
class ConfigurationStore {
 public:
  explicit ConfigurationStore(Journal& journal) : journal_(journal) {}
  bool save(const Configuration& config);
  bool load(Configuration& config);

 private:
  Journal& journal_;
};
}  // namespace lift
