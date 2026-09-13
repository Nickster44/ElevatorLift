#pragma once
#include <map>
#include <string>

#include "Supervisor.h"
namespace lift {
class HttpRequest {
 public:
  enum class Result { More, Complete, Invalid, TooLarge };
  std::string method, path, query, body;
  std::map<std::string, std::string> headers;
  Result feed(char c);
  static bool form(const std::string& value, std::map<std::string, std::string>& out);

 private:
  std::string raw_;
  size_t end_ = 0, bodySize_ = 0;
  bool parsed_ = false;
};
}  // namespace lift
