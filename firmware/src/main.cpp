#include <Arduino.h>
#include <cstring>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "ApiAuth.h"
#include "EventLog.h"
#include "LiftConfig.h"
#include "LiftSettings.h"
#include "NetworkConfig.h"
#include "PinMap.h"
#include "PositionStore.h"
#include "VfdProtocol.h"
#include "VfdParameters.h"

#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif

enum class MotionState : uint8_t {
  Boot,
  Idle,
  Moving,
  Stopping,
  Fault,
};

struct MotionCommand {
  bool active = false;
  uint8_t floor = 0;
  int64_t targetCounts = 0;
  uint32_t stopOffsetCounts = 0;
  VfdDirection direction = VfdDirection::Forward;
};

HardwareSerial VfdSerial(2);
WebServer server(80);
PositionStore store;
NetworkConfig networkConfig;
LiftSettingsStore liftSettingsStore;
EventLog eventLog;
StoredLiftState storedState;
NetworkSettings networkSettings;
LiftSettingsData liftSettings;

volatile int64_t pulsePositionCounts = 0;

MotionState motionState = MotionState::Boot;
MotionCommand activeCommand;
String lastFault;
String lastVfdCommand;
String vfdRxBuffer;

uint32_t lastVfdCommandMs = 0;
uint32_t lastStopCommandMs = 0;
uint32_t stopStartedMs = 0;
uint32_t lastPersistMs = 0;
uint32_t lastWifiCheckMs = 0;
uint32_t restartAtMs = 0;

bool lastButtonTop = false;
bool lastButtonRight = false;
bool lastButtonBottom = false;
bool lastButtonLeft = false;
bool lastButtonCenter = false;
bool fallbackApEnabled = false;
bool mdnsStarted = false;
bool restartRequested = false;

void IRAM_ATTR onUpPulse() {
  ++pulsePositionCounts;
}

void IRAM_ATTR onDownPulse() {
  --pulsePositionCounts;
}

int64_t readPositionCounts() {
  noInterrupts();
  const int64_t snapshot = pulsePositionCounts;
  interrupts();
  return snapshot;
}

void setPositionCounts(int64_t value) {
  noInterrupts();
  pulsePositionCounts = value;
  interrupts();
}

const char* stateName(MotionState state) {
  switch (state) {
    case MotionState::Boot:
      return "boot";
    case MotionState::Idle:
      return "idle";
    case MotionState::Moving:
      return "moving";
    case MotionState::Stopping:
      return "stopping";
    case MotionState::Fault:
      return "fault";
  }
  return "unknown";
}

bool safetyLoopHealthy() {
  return digitalRead(Pins::SafetyLoop) == LOW;
}

bool homeSwitchActive() {
  return digitalRead(Pins::HomeSwitch) == LOW;
}

bool lowerLimitActive() {
  return digitalRead(Pins::LowerLimit) == LOW;
}

bool upperLimitActive() {
  return digitalRead(Pins::UpperLimit) == LOW;
}

const FloorTarget* findFloor(uint8_t floor) {
  for (size_t i = 0; i < LiftConfig::FloorCount; ++i) {
    if (LiftConfig::DefaultFloors[i].floor == floor) {
      return &LiftConfig::DefaultFloors[i];
    }
  }
  return nullptr;
}

void sendVfd(const String& command) {
  VfdSerial.print(command);
  lastVfdCommand = command;
  Serial.print("VFD <- ");
  Serial.println(command);
}

void enterFault(const String& reason) {
  lastFault = reason;
  motionState = MotionState::Fault;
  activeCommand.active = false;
  eventLog.append(EventCode::Fault);
  sendVfd(VfdProtocol::stop());
  digitalWrite(Pins::StatusLed, HIGH);
}

void beginStopping() {
  if (motionState == MotionState::Stopping) {
    return;
  }
  motionState = MotionState::Stopping;
  stopStartedMs = millis();
  lastStopCommandMs = 0;
  eventLog.append(EventCode::MotionStopped);
  sendVfd(VfdProtocol::stop());
}

bool requestMoveToFloor(uint8_t floor) {
  if (motionState != MotionState::Idle) {
    return false;
  }
  if (!safetyLoopHealthy()) {
    enterFault("safety_loop_open");
    return false;
  }

  const FloorTarget* target = findFloor(floor);
  if (target == nullptr) {
    return false;
  }

  const int64_t current = readPositionCounts();
  if (target->positionCounts == current) {
    return true;
  }

  activeCommand.active = true;
  activeCommand.floor = floor;
  activeCommand.targetCounts = target->positionCounts;
  activeCommand.stopOffsetCounts = target->stopOffsetCounts;
  activeCommand.direction =
      target->positionCounts > current ? VfdDirection::Forward : VfdDirection::Reverse;

  motionState = MotionState::Moving;
  lastVfdCommandMs = 0;
  eventLog.append(EventCode::MotionAccepted, floor, static_cast<int32_t>(target->positionCounts));
  return true;
}

String jsonStatus() {
  char currentBuffer[24] = {};
  char targetBuffer[24] = {};
  snprintf(currentBuffer, sizeof(currentBuffer), "%lld",
           static_cast<long long>(readPositionCounts()));
  snprintf(targetBuffer, sizeof(targetBuffer), "%lld",
           static_cast<long long>(activeCommand.targetCounts));

  String json = "{";
  json += "\"state\":\"";
  json += stateName(motionState);
  json += "\",\"position\":";
  json += currentBuffer;
  json += ",\"target\":";
  json += targetBuffer;
  json += ",\"targetFloor\":";
  json += activeCommand.floor;
  json += ",\"normalRunTenthsHz\":";
  json += liftSettings.normalRunTenthsHz;
  json += ",\"safetyOk\":";
  json += safetyLoopHealthy() ? "true" : "false";
  json += ",\"home\":";
  json += homeSwitchActive() ? "true" : "false";
  json += ",\"lowerLimit\":";
  json += lowerLimitActive() ? "true" : "false";
  json += ",\"upperLimit\":";
  json += upperLimitActive() ? "true" : "false";
  json += ",\"lastVfdCommand\":\"";
  json += lastVfdCommand;
  json += "\",\"lastFault\":\"";
  json += lastFault;
  json += "\",\"networkMode\":\"";
  json += WiFi.status() == WL_CONNECTED ? "station" : "fallback_ap";
  json += "\",\"stationIp\":\"";
  json += WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  json += "\",\"apIp\":\"";
  json += fallbackApEnabled ? WiFi.softAPIP().toString() : "";
  json += "\"}";
  return json;
}

String jsonSettings() {
  String json = "{";
  json += "\"normalRunTenthsHz\":";
  json += liftSettings.normalRunTenthsHz;
  json += ",\"serviceJogTenthsHz\":";
  json += liftSettings.serviceJogTenthsHz;
  json += ",\"homingTenthsHz\":";
  json += liftSettings.homingTenthsHz;
  json += ",\"stopOffsetCounts\":";
  json += liftSettings.stopOffsetCounts;
  json += ",\"homingTimeoutMs\":";
  json += liftSettings.homingTimeoutMs;
  json += ",\"logRetentionRecords\":";
  json += liftSettings.logRetentionRecords;
  json += "}";
  return json;
}

void copyBounded(char* destination, size_t destinationSize, const String& source) {
  if (destinationSize == 0) {
    return;
  }
  snprintf(destination, destinationSize, "%s", source.c_str());
}

void loadNetworkSettings() {
  if (networkConfig.begin() && networkConfig.load(networkSettings)) {
    return;
  }

  copyBounded(networkSettings.apSsid, sizeof(networkSettings.apSsid), WIFI_AP_SSID);
  copyBounded(networkSettings.apPassword, sizeof(networkSettings.apPassword), WIFI_AP_PASSWORD);
}

void startFallbackAp() {
  if (fallbackApEnabled) {
    return;
  }

  if (strlen(networkSettings.apSsid) == 0) {
    copyBounded(networkSettings.apSsid, sizeof(networkSettings.apSsid), WIFI_AP_SSID);
  }
  if (strlen(networkSettings.apPassword) < 8) {
    copyBounded(networkSettings.apPassword, sizeof(networkSettings.apPassword), WIFI_AP_PASSWORD);
  }

  WiFi.mode(strlen(networkSettings.staSsid) > 0 ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(networkSettings.apSsid, networkSettings.apPassword);
  fallbackApEnabled = true;
  Serial.print("Fallback AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void serviceMdns() {
  if (mdnsStarted) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!MDNS.begin(LiftConfig::Hostname)) {
    Serial.println("mDNS start failed");
    return;
  }

  MDNS.addService("http", "tcp", 80);
  mdnsStarted = true;
  Serial.print("mDNS hostname: ");
  Serial.print(LiftConfig::Hostname);
  Serial.println(".local");
}

String jsonNetworkStatus() {
  String json = "{";
  json += "\"stationConfigured\":";
  json += strlen(networkSettings.staSsid) > 0 ? "true" : "false";
  json += ",\"stationSsid\":\"";
  json += networkSettings.staSsid;
  json += "\",\"stationConnected\":";
  json += WiFi.status() == WL_CONNECTED ? "true" : "false";
  json += ",\"stationIp\":\"";
  json += WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  json += "\",\"fallbackApEnabled\":";
  json += fallbackApEnabled ? "true" : "false";
  json += ",\"apSsid\":\"";
  json += networkSettings.apSsid;
  json += "\",\"apIp\":\"";
  json += fallbackApEnabled ? WiFi.softAPIP().toString() : "";
  json += "\",\"hostname\":\"";
  json += LiftConfig::Hostname;
  json += ".local";
  json += "\",\"writeAuthRequired\":";
  json += ApiAuth::tokenConfigured() ? "true" : "false";
  json += "}";
  return json;
}

void setupWebServer() {
  const char* authHeaders[] = {ApiAuth::TokenHeader};
  server.collectHeaders(authHeaders, 1);

  server.on("/", HTTP_GET, []() {
    String html =
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Lift Controller</title></head><body><h1>Lift Controller</h1>"
        "<pre id='status'></pre>"
        "<button onclick=\"fetch('/api/stop',{method:'POST'})\">Stop</button>"
        "<form onsubmit=\"event.preventDefault();fetch('/api/network',{method:'POST',body:new URLSearchParams(new FormData(this))}).then(tick)\">"
        "<input name='ssid' placeholder='Network SSID'><input name='password' placeholder='Password' type='password'>"
        "<button>Save network</button></form>"
        "<button onclick=\"fetch('/api/reboot',{method:'POST'})\">Reboot</button>"
        "<script>async function tick(){let r=await fetch('/api/status');"
        "document.getElementById('status').textContent=JSON.stringify(await r.json(),null,2)}"
        "setInterval(tick,1000);tick();</script></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", jsonStatus());
  });

  server.on("/api/network", HTTP_GET, []() {
    server.send(200, "application/json", jsonNetworkStatus());
  });

  server.on("/api/settings", HTTP_GET, []() {
    server.send(200, "application/json", jsonSettings());
  });

  server.on("/api/settings", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    LiftSettingsData updated = liftSettings;

    if (server.hasArg("normalRunTenthsHz")) {
      const int value = server.arg("normalRunTenthsHz").toInt();
      if (value < 1 || value > 2000) {
        server.send(400, "application/json", "{\"error\":\"normal_run_out_of_range\"}");
        return;
      }
      updated.normalRunTenthsHz = static_cast<uint16_t>(value);
    }
    if (server.hasArg("serviceJogTenthsHz")) {
      const int value = server.arg("serviceJogTenthsHz").toInt();
      if (value < 1 || value > 2000) {
        server.send(400, "application/json", "{\"error\":\"service_jog_out_of_range\"}");
        return;
      }
      updated.serviceJogTenthsHz = static_cast<uint16_t>(value);
    }
    if (server.hasArg("homingTenthsHz")) {
      const int value = server.arg("homingTenthsHz").toInt();
      if (value < 1 || value > 2000) {
        server.send(400, "application/json", "{\"error\":\"homing_speed_out_of_range\"}");
        return;
      }
      updated.homingTenthsHz = static_cast<uint16_t>(value);
    }
    if (server.hasArg("stopOffsetCounts")) {
      const int value = server.arg("stopOffsetCounts").toInt();
      if (value < 0 || value > 60000) {
        server.send(400, "application/json", "{\"error\":\"stop_offset_out_of_range\"}");
        return;
      }
      updated.stopOffsetCounts = static_cast<uint16_t>(value);
    }

    if (!liftSettingsStore.save(updated)) {
      server.send(500, "application/json", "{\"error\":\"settings_save_failed\"}");
      return;
    }

    liftSettings = updated;
    eventLog.append(EventCode::SettingsChanged);
    server.send(200, "application/json", jsonSettings());
  });

  server.on("/api/logs/recent", HTTP_GET, []() {
    server.send(200, "application/json", eventLog.jsonRecent());
  });

  server.on("/api/vfd/parameters", HTTP_GET, []() {
    server.send(200, "application/json", VfdParameters::allDefinitionsJson());
  });

  server.on("/api/vfd/parameter", HTTP_GET, []() {
    if (!server.hasArg("number")) {
      server.send(400, "application/json", "{\"error\":\"missing_number\"}");
      return;
    }

    const uint8_t number = static_cast<uint8_t>(server.arg("number").toInt());
    const VfdParameterDefinition* definition = VfdParameters::find(number);
    if (definition == nullptr) {
      server.send(404, "application/json", "{\"error\":\"unknown_parameter\"}");
      return;
    }

    sendVfd(VfdProtocol::getParameter(number));
    eventLog.append(EventCode::VfdParameterRead, number);

    String json = "{";
    json += "\"definition\":";
    json += VfdParameters::definitionJson(*definition);
    json += ",\"commandSent\":\"";
    json += lastVfdCommand;
    json += "\",\"readbackParsing\":\"pending\"}";
    server.send(202, "application/json", json);
  });

  server.on("/api/vfd/parameter", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    if (motionState != MotionState::Idle) {
      server.send(409, "application/json", "{\"error\":\"lift_must_be_idle\"}");
      return;
    }
    if (!server.hasArg("number") || !server.hasArg("value")) {
      server.send(400, "application/json", "{\"error\":\"missing_number_or_value\"}");
      return;
    }

    const uint8_t number = static_cast<uint8_t>(server.arg("number").toInt());
    const int requestedValue = server.arg("value").toInt();
    const VfdParameterDefinition* definition = VfdParameters::find(number);
    if (definition == nullptr) {
      server.send(404, "application/json", "{\"error\":\"unknown_parameter\"}");
      return;
    }
    if (!definition->writable) {
      server.send(403, "application/json", "{\"error\":\"parameter_read_only\"}");
      return;
    }
    if (requestedValue < definition->minValue || requestedValue > definition->maxValue) {
      server.send(400, "application/json", "{\"error\":\"value_out_of_range\"}");
      return;
    }

    sendVfd(VfdProtocol::setParameter(number, static_cast<uint16_t>(requestedValue)));
    eventLog.append(EventCode::VfdParameterWrite, number, requestedValue);

    String json = "{";
    json += "\"definition\":";
    json += VfdParameters::definitionJson(*definition);
    json += ",\"requestedValue\":";
    json += requestedValue;
    json += ",\"commandSent\":\"";
    json += lastVfdCommand;
    json += "\",\"readbackVerification\":\"pending\"}";
    server.send(202, "application/json", json);
  });

  server.on("/api/network", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    if (!server.hasArg("ssid")) {
      server.send(400, "application/json", "{\"error\":\"missing_ssid\"}");
      return;
    }

    copyBounded(networkSettings.staSsid, sizeof(networkSettings.staSsid), server.arg("ssid"));
    copyBounded(networkSettings.staPassword, sizeof(networkSettings.staPassword),
                server.arg("password"));

    if (!networkConfig.save(networkSettings)) {
      server.send(500, "application/json", "{\"error\":\"save_failed\"}");
      return;
    }

    eventLog.append(EventCode::NetworkChanged);
    server.send(202, "application/json", "{\"saved\":true,\"restartRequired\":true}");
  });

  server.on("/api/reboot", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    restartRequested = true;
    restartAtMs = millis() + 1000;
    server.send(202, "application/json", "{\"rebooting\":true}");
  });

  server.on("/api/move", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    if (!server.hasArg("floor")) {
      server.send(400, "application/json", "{\"error\":\"missing_floor\"}");
      return;
    }
    const uint8_t floor = static_cast<uint8_t>(server.arg("floor").toInt());
    if (!requestMoveToFloor(floor)) {
      eventLog.append(EventCode::MotionRejected, floor);
      server.send(409, "application/json", "{\"error\":\"move_rejected\"}");
      return;
    }
    server.send(202, "application/json", jsonStatus());
  });

  server.on("/api/stop", HTTP_POST, []() {
    if (!ApiAuth::requestAuthorized(server)) {
      ApiAuth::sendUnauthorized(server);
      return;
    }

    beginStopping();
    server.send(202, "application/json", jsonStatus());
  });

  server.begin();
}

void setupWiFi() {
  loadNetworkSettings();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(LiftConfig::Hostname);

  if (strlen(networkSettings.staSsid) > 0) {
    WiFi.begin(networkSettings.staSsid, networkSettings.staPassword);
  }

  startFallbackAp();
}

void serviceWiFi() {
  const uint32_t now = millis();
  if (now - lastWifiCheckMs < 5000) {
    return;
  }
  lastWifiCheckMs = now;

  if (strlen(networkSettings.staSsid) == 0) {
    startFallbackAp();
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (fallbackApEnabled) {
      WiFi.softAPdisconnect(true);
      fallbackApEnabled = false;
      WiFi.mode(WIFI_STA);
    }
    return;
  }

  startFallbackAp();
  WiFi.reconnect();
}

void initializeVfd() {
  sendVfd(VfdProtocol::setParameter(3, 10));
  delay(50);
  sendVfd(VfdProtocol::setParameter(4, 10));
  delay(50);
  sendVfd(VfdProtocol::setParameter(12, 1));
  delay(50);
  sendVfd(VfdProtocol::setParameter(10, 1200));
  delay(50);
}

void serviceVfdRx() {
  while (VfdSerial.available()) {
    const char c = static_cast<char>(VfdSerial.read());
    vfdRxBuffer += c;
    if (vfdRxBuffer.length() > 96) {
      vfdRxBuffer.remove(0, vfdRxBuffer.length() - 96);
    }
  }

  if (motionState == MotionState::Stopping && VfdProtocol::isStopAck(vfdRxBuffer)) {
    activeCommand.active = false;
    motionState = MotionState::Idle;
    vfdRxBuffer = "";
  }
}

void serviceMotion() {
  if (!safetyLoopHealthy() && motionState != MotionState::Fault) {
    enterFault("safety_loop_open");
    return;
  }

  if (homeSwitchActive() && motionState == MotionState::Idle) {
    setPositionCounts(0);
  }

  const uint32_t now = millis();

  if (motionState == MotionState::Moving && activeCommand.active) {
    const int64_t current = readPositionCounts();
    const uint64_t remaining =
        static_cast<uint64_t>(llabs(activeCommand.targetCounts - current));

    if (remaining <= activeCommand.stopOffsetCounts) {
      beginStopping();
      return;
    }

    if (activeCommand.direction == VfdDirection::Forward && upperLimitActive()) {
      enterFault("upper_limit_active");
      return;
    }
    if (activeCommand.direction == VfdDirection::Reverse && lowerLimitActive()) {
      enterFault("lower_limit_active");
      return;
    }

    if (now - lastVfdCommandMs >= LiftConfig::VfdCommandRefreshMs) {
      sendVfd(VfdProtocol::run(activeCommand.direction, liftSettings.normalRunTenthsHz));
      lastVfdCommandMs = now;
    }
  }

  if (motionState == MotionState::Stopping) {
    if (now - lastStopCommandMs >= LiftConfig::StopRetryMs) {
      sendVfd(VfdProtocol::stop());
      lastStopCommandMs = now;
    }
    if (now - stopStartedMs > LiftConfig::StopAckTimeoutMs) {
      enterFault("stop_ack_timeout");
    }
  }
}

bool risingEdge(uint8_t pin, bool& lastState) {
  const bool current = digitalRead(pin) == HIGH;
  const bool rising = current && !lastState;
  lastState = current;
  return rising;
}

void serviceButtons() {
  if (risingEdge(Pins::ButtonLeft, lastButtonLeft)) {
    requestMoveToFloor(2);
  }
  if (risingEdge(Pins::ButtonTop, lastButtonTop)) {
    requestMoveToFloor(3);
  }
  if (risingEdge(Pins::ButtonRight, lastButtonRight)) {
    requestMoveToFloor(4);
  }
  if (risingEdge(Pins::ButtonCenter, lastButtonCenter)) {
    beginStopping();
  }
  if (risingEdge(Pins::ButtonBottom, lastButtonBottom)) {
    beginStopping();
  }
}

void servicePersistence() {
  const uint32_t now = millis();
  if (now - lastPersistMs < LiftConfig::PositionPersistMs) {
    return;
  }
  storedState.currentPosition = readPositionCounts();
  store.save(storedState);
  lastPersistMs = now;
}

void setupPins() {
  pinMode(Pins::ButtonTop, INPUT);
  pinMode(Pins::ButtonRight, INPUT);
  pinMode(Pins::ButtonBottom, INPUT);
  pinMode(Pins::ButtonLeft, INPUT);
  pinMode(Pins::ButtonCenter, INPUT);

  pinMode(Pins::SafetyLoop, INPUT_PULLUP);
  pinMode(Pins::HomeSwitch, INPUT_PULLUP);
  pinMode(Pins::UpperLimit, INPUT_PULLUP);
  pinMode(Pins::LowerLimit, INPUT_PULLUP);

  pinMode(Pins::PositionUpPulse, INPUT);
  pinMode(Pins::PositionDownPulse, INPUT);
  pinMode(Pins::StatusLed, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(Pins::PositionUpPulse), onUpPulse, RISING);
  attachInterrupt(digitalPinToInterrupt(Pins::PositionDownPulse), onDownPulse, RISING);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  setupPins();
  VfdSerial.begin(LiftConfig::VfdBaud, SERIAL_8N1, Pins::VfdRx, Pins::VfdTx);
  eventLog.begin();

  if (store.begin() && store.load(storedState)) {
    setPositionCounts(storedState.currentPosition);
  } else {
    storedState = StoredLiftState{};
    store.save(storedState);
  }
  ++storedState.bootCount;
  store.save(storedState);

  if (!liftSettingsStore.begin() || !liftSettingsStore.load(liftSettings)) {
    liftSettings = liftSettingsStore.defaults();
    liftSettingsStore.save(liftSettings);
  }

  setupWiFi();
  setupWebServer();
  serviceMdns();
  initializeVfd();
  eventLog.append(EventCode::Boot, static_cast<int32_t>(storedState.bootCount));

  motionState = safetyLoopHealthy() ? MotionState::Idle : MotionState::Fault;
  if (motionState == MotionState::Fault) {
    lastFault = "safety_loop_open_at_boot";
  }
}

void loop() {
  server.handleClient();
  serviceWiFi();
  serviceMdns();
  serviceVfdRx();
  serviceButtons();
  serviceMotion();
  servicePersistence();

  if (restartRequested && millis() >= restartAtMs) {
    ESP.restart();
  }
}
