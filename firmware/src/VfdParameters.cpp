#include "VfdParameters.h"

namespace {

constexpr auto& kDefinitions = lift::ParameterDefinitions;

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

size_t count() { return sizeof(kDefinitions) / sizeof(kDefinitions[0]); }

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
