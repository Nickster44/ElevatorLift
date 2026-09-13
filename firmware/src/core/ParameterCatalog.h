#pragma once
#include <cstdint>
namespace lift {
enum class ParameterAccess : uint8_t { User, Installer, Advanced, Locked };
struct ParameterDefinition {
  uint8_t number;
  const char* name;
  const char* displayName;
  const char* units;
  uint16_t minValue, maxValue, defaultValue, scaleDivisor;
  bool writable;
  ParameterAccess accessLevel;
};
inline constexpr ParameterDefinition ParameterDefinitions[] = {
    {0, "OVERV", "Overvoltage alarm", "Vdc", 100, 400, 390, 1, true, ParameterAccess::Advanced},
    {1, "UNDER", "Undervoltage alarm", "Vdc", 100, 400, 200, 1, true, ParameterAccess::Advanced},
    {2, "TEMPALL", "Temperature alarm", "deg C", 20, 100, 80, 1, true, ParameterAccess::Installer},
    {3, "ACC", "Acceleration ramp", "s", 1, 599, 50, 10, true, ParameterAccess::Installer},
    {4, "DEC", "Deceleration ramp", "s", 1, 599, 50, 10, true, ParameterAccess::Installer},
    {5, "BOOST", "Voltage boost", "raw", 0, 90, 8, 1, true, ParameterAccess::Advanced},
    {6, "IN", "Nominal current", "A", 5, 100, 36, 10, true, ParameterAccess::Installer},
    {7, "TSOVRA", "Overload time", "s", 0, 60, 0, 1, true, ParameterAccess::Installer},
    {8, "PSOVRA", "Overload percent", "%", 100, 150, 150, 1, true, ParameterAccess::Installer},
    {9, "VMAX", "Rated-voltage frequency", "Hz", 250, 2000, 500, 10, true,
     ParameterAccess::Advanced},
    {10, "FMAX", "Maximum frequency", "Hz", 0, 2000, 1000, 10, true, ParameterAccess::Advanced},
    {11, "FMIN", "Minimum frequency", "Hz", 0, 2000, 0, 10, true, ParameterAccess::Advanced},
    {12, "TIME", "Serial timeout", "s", 0, 599, 0, 10, true, ParameterAccess::Installer},
    {13, "RELE", "Expansion relay / input (readback unresolved)", "raw", 0, 1, 0, 1, false,
     ParameterAccess::Locked},
    {14, "POT1", "Expansion analog input 1", "raw", 0, 255, 0, 1, false, ParameterAccess::Advanced},
    {15, "POT2", "Expansion analog input 2", "raw", 0, 255, 0, 1, false, ParameterAccess::Advanced},
    {16, "DAC", "Expansion analog output", "raw", 0, 255, 0, 1, false, ParameterAccess::Advanced}};
}  // namespace lift
