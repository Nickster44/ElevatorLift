#pragma once

#include <WebServer.h>

namespace ApiAuth {

constexpr const char* TokenHeader = "X-Lift-Api-Token";

bool tokenConfigured();
bool requestAuthorized(WebServer& server);
void sendUnauthorized(WebServer& server);

}  // namespace ApiAuth
