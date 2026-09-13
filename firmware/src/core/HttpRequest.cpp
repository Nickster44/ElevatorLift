#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
namespace lift {
HttpRequest::Result HttpRequest::feed(char c) {
  raw_ += c;
  if (raw_.size() > 2560)
    return Result::TooLarge;
  if (!parsed_) {
    if (raw_.size() > 2048)
      return Result::TooLarge;
    auto end = raw_.find("\r\n\r\n");
    if (end == std::string::npos)
      return Result::More;
    end_ = end + 4;
    auto lineEnd = raw_.find("\r\n");
    auto line = raw_.substr(0, lineEnd);
    auto first = line.find(' '), second = line.find(' ', first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        line.substr(second + 1) != "HTTP/1.1")
      return Result::Invalid;
    method = line.substr(0, first);
    path = line.substr(first + 1, second - first - 1);
    if ((method != "GET" && method != "POST") || path.empty() || path[0] != '/' ||
        path.size() > 256)
      return Result::Invalid;
    auto q = path.find('?');
    if (q != std::string::npos) {
      query = path.substr(q + 1);
      path.resize(q);
    }
    for (size_t pos = lineEnd + 2; pos < end;) {
      auto next = raw_.find("\r\n", pos);
      auto colon = raw_.find(':', pos);
      if (colon == std::string::npos || colon >= next)
        return Result::Invalid;
      auto key = raw_.substr(pos, colon - pos);
      std::transform(key.begin(), key.end(), key.begin(),
                     [](unsigned char v) { return std::tolower(v); });
      auto v = raw_.substr(colon + 1, next - colon - 1);
      while (!v.empty() && v.front() == ' ')
        v.erase(0, 1);
      if (headers.count(key))
        return Result::Invalid;
      headers[key] = v;
      pos = next + 2;
    }
    if (headers.count("transfer-encoding"))
      return Result::Invalid;
    uint32_t length = 0;
    if (headers.count("content-length") &&
        !parseUnsigned(headers["content-length"].c_str(), 512, length))
      return Result::TooLarge;
    bodySize_ = length;
    if (method == "GET" && length)
      return Result::Invalid;
    if (length && headers["content-type"] != "application/x-www-form-urlencoded")
      return Result::Invalid;
    parsed_ = true;
  }
  if (raw_.size() == end_ + bodySize_) {
    body = raw_.substr(end_);
    return Result::Complete;
  }
  return Result::More;
}
bool HttpRequest::form(const std::string& s, std::map<std::string, std::string>& out) {
  auto decode = [](const std::string& in, std::string& value) {
    for (size_t i = 0; i < in.size(); ++i) {
      unsigned char c = in[i];
      if (c == '+')
        c = ' ';
      else if (c == '%') {
        if (i + 2 >= in.size())
          return false;
        auto hex = [](char h) -> int {
          if (h >= '0' && h <= '9')
            return h - '0';
          if (h >= 'A' && h <= 'F')
            return h - 'A' + 10;
          if (h >= 'a' && h <= 'f')
            return h - 'a' + 10;
          return -1;
        };
        int a = hex(in[++i]), b = hex(in[++i]);
        if (a < 0 || b < 0)
          return false;
        c = a * 16 + b;
      }
      if (c < 32 || c == 127)
        return false;
      value += c;
    }
    return true;
  };
  size_t pos = 0;
  while (pos < s.size()) {
    auto next = s.find('&', pos);
    if (next == std::string::npos)
      next = s.size();
    auto pair = s.substr(pos, next - pos);
    auto eq = pair.find('=');
    if (eq == std::string::npos)
      return false;
    std::string key, value;
    if (!decode(pair.substr(0, eq), key) || key.empty() || !decode(pair.substr(eq + 1), value) ||
        out.count(key) || out.size() >= 16)
      return false;
    out[key] = value;
    pos = next + 1;
  }
  return true;
}
}  // namespace lift
