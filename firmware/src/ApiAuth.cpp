#include "ApiAuth.h"

#include <Arduino.h>
#include <cstring>

#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif

#ifndef LIFT_API_TOKEN
#define LIFT_API_TOKEN ""
#endif

namespace ApiAuth {

bool tokenConfigured() {
  return std::strlen(LIFT_API_TOKEN) > 0;
}

bool requestAuthorized(WebServer& server) {
  if (!tokenConfigured()) {
    return true;
  }

  String suppliedToken = server.header(TokenHeader);
  if (suppliedToken.length() == 0 && server.hasArg("token")) {
    suppliedToken = server.arg("token");
  }

  return suppliedToken == LIFT_API_TOKEN;
}

void sendUnauthorized(WebServer& server) {
  server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
}

}  // namespace ApiAuth
