#include "VfdProtocol.h"

namespace {

char checksumNibble(uint8_t value) {
  return static_cast<char>('0' + (value & 0x0F));
}

String fixedWidth(uint16_t value, uint8_t width) {
  char buffer[8] = {};
  snprintf(buffer, sizeof(buffer), "%0*u", width, value);
  return String(buffer);
}

}  // namespace

namespace VfdProtocol {

uint8_t checksumByte(const String& framedPayload) {
  uint16_t sum = 0;
  for (size_t i = 0; i < framedPayload.length(); ++i) {
    sum += static_cast<uint8_t>(framedPayload[i]);
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

String appendChecksum(const String& framedPayload) {
  const uint8_t checksum = checksumByte(framedPayload);
  String out = framedPayload;
  out += checksumNibble(checksum >> 4);
  out += checksumNibble(checksum);
  return out;
}

String run(VfdDirection direction, uint16_t tenthsHz) {
  if (tenthsHz > 9999) {
    tenthsHz = 9999;
  }

  String payload = "(";
  payload += direction == VfdDirection::Forward ? '1' : '2';
  payload += fixedWidth(tenthsHz, 4);
  payload += ")";
  return appendChecksum(payload);
}

String stop() {
  return appendChecksum("(3)");
}

String monitor() {
  return appendChecksum("(0)");
}

String setParameter(uint8_t parameter, uint16_t value) {
  if (parameter > 99) {
    parameter = 99;
  }
  if (value > 9999) {
    value = 9999;
  }

  String payload = "(4";
  payload += fixedWidth(parameter, 2);
  payload += fixedWidth(value, 4);
  payload += ")";
  return appendChecksum(payload);
}

String getParameter(uint8_t parameter) {
  if (parameter > 99) {
    parameter = 99;
  }

  String payload = "(5";
  payload += fixedWidth(parameter, 2);
  payload += ")";
  return appendChecksum(payload);
}

bool isStopAck(const String& frame) {
  return frame.indexOf("(3)84") >= 0;
}

bool isRunForwardAck(const String& frame) {
  return frame.indexOf("(1)82") >= 0;
}

bool isRunReverseAck(const String& frame) {
  return frame.indexOf("(2)83") >= 0;
}

}  // namespace VfdProtocol
