#include "VfdParameters.h"

namespace {

constexpr VfdParameterDefinition kDefinitions[] = {
    {0, "OVERV", "Overvoltage alarm", "Vdc", 100, 400, 390, 1, true,
     VfdAccessLevel::Advanced},
    {1, "UNDER", "Undervoltage alarm", "Vdc", 100, 400, 200, 1, true,
     VfdAccessLevel::Advanced},
    {2, "TEMPALL", "Temperature alarm", "deg C", 20, 100, 80, 1, true,
     VfdAccessLevel::Installer},
    {3, "ACC", "Acceleration ramp", "s", 1, 599, 50, 10, true,
     VfdAccessLevel::Installer},
    {4, "DEC", "Deceleration ramp", "s", 1, 599, 50, 10, true,
     VfdAccessLevel::Installer},
    {5, "BOOST", "Voltage boost", "raw", 0, 90, 8, 1, true,
     VfdAccessLevel::Advanced},
    {6, "IN", "Nominal current", "A", 5, 100, 36, 10, true,
     VfdAccessLevel::Installer},
    {7, "TSOVRA", "Overload time", "s", 0, 60, 0, 1, true,
     VfdAccessLevel::Installer},
    {8, "PSOVRA", "Overload percent", "%", 100, 150, 150, 1, true,
     VfdAccessLevel::Installer},
    {9, "VMAX", "Rated-voltage frequency", "Hz", 250, 2000, 500, 10, true,
     VfdAccessLevel::Advanced},
    {10, "FMAX", "Maximum frequency", "Hz", 0, 2000, 1000, 10, true,
     VfdAccessLevel::Advanced},
    {11, "FMIN", "Minimum frequency", "Hz", 0, 2000, 0, 10, true,
     VfdAccessLevel::Advanced},
    {12, "TIME", "Serial timeout", "s", 0, 599, 0, 10, true,
     VfdAccessLevel::Installer},
    {13, "RELE", "Expansion relay", "bool", 0, 1, 0, 1, true,
     VfdAccessLevel::Advanced},
    {14, "POT1", "Expansion analog input 1", "raw", 0, 255, 0, 1, false,
     VfdAccessLevel::Advanced},
    {15, "POT2", "Expansion analog input 2", "raw", 0, 255, 0, 1, false,
     VfdAccessLevel::Advanced},
    {16, "DAC", "Expansion analog output", "raw", 0, 255, 0, 1, false,
     VfdAccessLevel::Advanced},
};

}  // namespace

namespace VfdParameters {

const VfdParameterDefinition* find(uint8_t number) {
  for (const auto& definition : kDefinitions) {
    if (definition.number == number) {
      return &definition;
    }
  }
  return nullptr;
}

const VfdParameterDefinition* at(size_t index) {
  if (index >= count()) {
    return nullptr;
  }
  return &kDefinitions[index];
}

size_t count() {
  return sizeof(kDefinitions) / sizeof(kDefinitions[0]);
}

String accessLevelName(VfdAccessLevel accessLevel) {
  switch (accessLevel) {
    case VfdAccessLevel::User:
      return "user";
    case VfdAccessLevel::Installer:
      return "installer";
    case VfdAccessLevel::Advanced:
      return "advanced";
    case VfdAccessLevel::Locked:
      return "locked";
  }
  return "unknown";
}

String definitionJson(const VfdParameterDefinition& definition) {
  String json = "{";
  json += "\"number\":";
  json += definition.number;
  json += ",\"name\":\"";
  json += definition.name;
  json += "\",\"displayName\":\"";
  json += definition.displayName;
  json += "\",\"units\":\"";
  json += definition.units;
  json += "\",\"min\":";
  json += definition.minValue;
  json += ",\"max\":";
  json += definition.maxValue;
  json += ",\"default\":";
  json += definition.defaultValue;
  json += ",\"scaleDivisor\":";
  json += definition.scaleDivisor;
  json += ",\"writable\":";
  json += definition.writable ? "true" : "false";
  json += ",\"access\":\"";
  json += accessLevelName(definition.accessLevel);
  json += "\"}";
  return json;
}

String allDefinitionsJson() {
  String json = "[";
  for (size_t i = 0; i < count(); ++i) {
    if (i > 0) {
      json += ",";
    }
    json += definitionJson(kDefinitions[i]);
  }
  json += "]";
  return json;
}

}  // namespace VfdParameters

