#pragma once

#include <Arduino.h>

enum class VfdDirection : uint8_t {
  Forward = 1,
  Reverse = 2,
};

namespace VfdProtocol {

String run(VfdDirection direction, uint16_t tenthsHz);
String stop();
String monitor();
String setParameter(uint8_t parameter, uint16_t value);
String getParameter(uint8_t parameter);

bool isStopAck(const String& frame);
bool isRunForwardAck(const String& frame);
bool isRunReverseAck(const String& frame);

uint8_t checksumByte(const String& framedPayload);
String appendChecksum(const String& framedPayload);

}  // namespace VfdProtocol

