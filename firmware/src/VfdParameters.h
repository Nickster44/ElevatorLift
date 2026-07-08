#pragma once

#include <Arduino.h>

enum class VfdAccessLevel : uint8_t {
  User,
  Installer,
  Advanced,
  Locked,
};

struct VfdParameterDefinition {
  uint8_t number;
  const char* name;
  const char* displayName;
  const char* units;
  uint16_t minValue;
  uint16_t maxValue;
  uint16_t defaultValue;
  uint16_t scaleDivisor;
  bool writable;
  VfdAccessLevel accessLevel;
};

namespace VfdParameters {

const VfdParameterDefinition* find(uint8_t number);
const VfdParameterDefinition* at(size_t index);
size_t count();
String accessLevelName(VfdAccessLevel accessLevel);
String definitionJson(const VfdParameterDefinition& definition);
String allDefinitionsJson();

}  // namespace VfdParameters

