#pragma once

#include <Arduino.h>

#include "core/ParameterCatalog.h"

using VfdAccessLevel = lift::ParameterAccess;
using VfdParameterDefinition = lift::ParameterDefinition;

namespace VfdParameters {

const VfdParameterDefinition* find(uint8_t number);
const VfdParameterDefinition* at(size_t index);
size_t count();
String accessLevelName(VfdAccessLevel accessLevel);
String definitionJson(const VfdParameterDefinition& definition);
String allDefinitionsJson();

}  // namespace VfdParameters
