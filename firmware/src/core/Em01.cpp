#include "Em01.h"

#include <cstdio>
namespace lift {
std::string Em01::frame(const std::string& payload) {
  uint8_t sum = 0;
  for (unsigned char b : payload)
    sum += b;
  return payload + char('0' + (sum >> 4)) + char('0' + (sum & 15));
}
bool Em01::valid(const std::string& f) {
  if (f.size() < 5 || f.size() > 32 || f.front() != '(' || f[f.size() - 3] != ')')
    return false;
  for (size_t i = 1; i < f.size() - 3; ++i)
    if (f[i] < '0' || f[i] > '9')
      return false;
  return frame(f.substr(0, f.size() - 2)) == f;
}
bool Em01::request(char op, unsigned value, unsigned parameter, uint32_t now) {
  if (busy() || (poisoned_ && op != '3') || op < '0' || op > '5' || value > 9999 || parameter > 16)
    return false;
  char p[20];
  if (op == '1' || op == '2')
    std::snprintf(p, sizeof p, "(%c%04u)", op, value);
  else if (op == '4')
    std::snprintf(p, sizeof p, "(4%02u%04u)", parameter, value);
  else if (op == '5')
    std::snprintf(p, sizeof p, "(5%02u)", parameter);
  else
    std::snprintf(p, sizeof p, "(%c)", op);
  op_ = op;
  value_ = value;
  parameter_ = parameter;
  attempts_ = 0;
  sent_ = false;
  readback_ = false;
  startMs_ = now;
  buffer_.clear();
  outgoing_ = frame(p);
  result = Result::Pending;
  if (op == '3')
    stopAcknowledged = false;
  return true;
}
std::string Em01::transmit(uint32_t now) {
  if (!busy())
    return {};
  if (sent_ && now - sentMs_ < 150)
    return {};
  if (attempts_ >= 3) {
    result = Result::Timeout;
    poisoned_ = true;
    telemetry.valid = false;
    return {};
  }
  ++attempts_;
  sent_ = true;
  sentMs_ = now;
  buffer_.clear();
  return outgoing_;
}
void Em01::receive(char b, uint32_t now) {
  if (!busy() || !sent_ || now - sentMs_ >= 150)
    return;
  if (!buffer_.empty() && now - lastByteMs_ > 30)
    buffer_.clear();
  lastByteMs_ = now;
  if (b == '(') {
    buffer_ = "(";
    return;
  }
  if (buffer_.empty())
    return;
  buffer_ += b;
  if (buffer_.size() > 32) {
    buffer_.clear();
    return;
  }
  size_t end = buffer_.find(')');
  if (end != std::string::npos && buffer_.size() == end + 3) {
    auto f = buffer_;
    buffer_.clear();
    if (valid(f))
      accept(f, now);
  }
}
void Em01::accept(const std::string& f, uint32_t now) {
  auto number = [&](size_t offset, size_t length) {
    unsigned n = 0;
    for (size_t i = 0; i < length; ++i)
      n = n * 10 + unsigned(f[offset + i] - '0');
    return n;
  };
  if (f[1] != op_)
    return;
  if (op_ == '0' && f.size() == 19) {
    unsigned status = number(15, 1);
    if (status > 8)
      return;
    telemetry = {true, now, number(2, 3), number(5, 4), number(9, 3), number(12, 3), status};
    result = Result::Verified;
  } else if (op_ == '5' && f.size() == 9) {
    unsigned value = number(2, 4);
    if (readback_ && value != value_) {
      result = Result::Mismatch;
      return;
    }
    parameters[parameter_] = static_cast<int>(value);
    parameterSampleMs[parameter_] = now;
    result = Result::Verified;
  } else if ((op_ == '1' || op_ == '2' || op_ == '3' || op_ == '4') && f.size() == 5) {
    if (op_ == '3')
      stopAcknowledged = true;
    if (op_ == '4') {
      // EM01 does not echo the parameter number. Only one transaction may exist.
      char p[10];
      std::snprintf(p, sizeof p, "(5%02u)", parameter_);
      outgoing_ = frame(p);
      op_ = '5';
      readback_ = true;
      sent_ = false;
      attempts_ = 0;
      startMs_ = now;
    } else
      result = Result::Acknowledged;
  }
}
}  // namespace lift
