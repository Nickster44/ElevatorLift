#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <mbedtls/sha256.h>

#include "NetworkConfig.h"
#include "PinMap.h"
#include "VfdParameters.h"
#include "core/Configuration.h"
#include "core/Devices.h"
#include "core/DiagnosticVfd.h"
#include "core/FieldInputs.h"
#include "core/HttpRequest.h"
#include "core/ManualStop.h"
#include "core/SafetyLedger.h"
#include "core/Supervisor.h"
#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif
#ifndef LIFT_API_TOKEN
#define LIFT_API_TOKEN ""
#endif
namespace {
class BoardSpi final : public lift::SpiBus {
 public:
  bool transfer(int cs, const uint8_t* tx, uint8_t* rx, size_t n) override {
    if (n > 68 || (cs != Pins::CounterCs && cs != Pins::MramCs))
      return false;
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(cs, LOW);
    for (size_t i = 0; i < n; ++i)
      rx[i] = SPI.transfer(tx[i]);
    digitalWrite(cs, HIGH);
    SPI.endTransaction();
    return true;
  }
} spi;
class BoardI2c final : public lift::I2cBus {
 public:
  bool read(uint8_t device, uint8_t reg, uint8_t* out, size_t n) override {
    Wire.beginTransmission(device);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
      return false;
    if (Wire.requestFrom(device, uint8_t(n)) != n)
      return false;
    for (size_t i = 0; i < n; ++i)
      out[i] = Wire.read();
    return true;
  }
} i2c;
lift::Counter counter(spi, Pins::CounterCs);
lift::Mram mram(spi, Pins::MramCs);
lift::Rtc rtc(i2c);
lift::Journal snapshots(mram, 0, 2), configurationRecords(mram, 2048, 2),
    safetyRecords(mram, 4096, 2), events(mram, 8192, 768);
lift::SafetyLedger safetyLedger(safetyRecords);
lift::ConfigurationStore configurationStore(configurationRecords);
lift::Configuration configuration;
lift::Supervisor supervisor;
lift::FieldInputs fieldInputs;
lift::DiagnosticVfd diagnosticVfd;
lift::ManualStop manualStop;
HardwareSerial VfdSerial(1);
bool vfdUartReady = false;
constexpr bool FieldContinuityQualified = false;
int discardUartLog(const char*, va_list) { return 0; }
void serviceVfd(uint32_t now) {
  if (!vfdUartReady)
    return;
  for (unsigned n = 0; n < 64 && VfdSerial.available(); ++n)
    diagnosticVfd.receive(char(VfdSerial.read()), now);
  // All frames fit in the hardware TX queue; do not block the supervisor loop.
  if (VfdSerial.availableForWrite() >= 32) {
    const auto frame = diagnosticVfd.transmit(now);
    if (!frame.empty())
      VfdSerial.write(reinterpret_cast<const uint8_t*>(frame.data()), frame.size());
  }
}
NetworkConfig networkStore;
NetworkSettings network;
WiFiServer http(80);
WiFiClient client;
lift::HttpRequest request;
File asset;
String response;
size_t responseOffset = 0;
uint32_t connectedAt = 0, lastSample = 0, lastNetwork = 0, lastRtc = 0, lastPersist = 0,
         unixTime = 0, statusSequence = 0;
bool responding = false, ap = false, counterReady = false, storageReady = false, rtcValid = false,
     fsReady = false;
int64_t position = 0;
const char* previousFault = "";
const char* previousWatchdogState = "unknown";
bool vfdFailureLogged = false;
char bootId[17] = {};
uint32_t lastWriteRequestMs = 0;
String counts(int64_t n) {
  char b[24];
  snprintf(b, sizeof b, "%lld", static_cast<long long>(n));
  return b;
}
bool tokenConfigured() { return strlen(LIFT_API_TOKEN) >= 16; }
bool verifyWebBundle() {
  File manifest = LittleFS.open("/bundle-manifest.json", "r");
  if (!manifest || manifest.size() > 8192)
    return false;
  JsonDocument d;
  if (deserializeJson(d, manifest) || d["apiVersion"] != 1 ||
      d["contractVersion"] != Pins::ContractVersion)
    return false;
  JsonArray files = d["files"].as<JsonArray>();
  if (files.size() < 2 || files.size() > 32)
    return false;
  size_t total = 0;
  bool index = false;
  for (JsonObject entry : files) {
    const char* name = entry["path"] | "";
    const char* expected = entry["sha256"] | "";
    String path = name;
    if (path.length() > 128 || path.indexOf("..") >= 0 || path.indexOf('\\') >= 0 ||
        path.startsWith("/") || strlen(expected) != 64)
      return false;
    if (path != "index.html" && path != "index.html.gz" && !path.startsWith("assets/"))
      return false;
    File file = LittleFS.open("/" + path, "r");
    size_t size = entry["bytes"] | 0;
    if (!file || file.size() != size || size > 2097152 || total > 2097152 - size)
      return false;
    total += size;
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    mbedtls_sha256_starts_ret(&hash, 0);
    uint8_t buffer[256], digest[32];
    while (file.available()) {
      size_t n = file.read(buffer, sizeof buffer);
      if (!n) {
        mbedtls_sha256_free(&hash);
        return false;
      }
      mbedtls_sha256_update_ret(&hash, buffer, n);
    }
    mbedtls_sha256_finish_ret(&hash, digest);
    mbedtls_sha256_free(&hash);
    char actual[65];
    for (unsigned i = 0; i < 32; ++i)
      snprintf(actual + 2 * i, 3, "%02x", digest[i]);
    if (strcmp(actual, expected))
      return false;
    if (path == "index.html")
      index = true;
  }
  return index;
}
void event(const char* code, const char* source, const char* result) {
  if (!storageReady)
    return;
  JsonDocument d;
  d["code"] = code;
  d["source"] = source;
  d["result"] = result;
  d["uptimeMs"] = millis();
  d["position"] = counts(position);
  if (rtcValid)
    d["unixTime"] = unixTime;
  else
    d["unixTime"] = nullptr;
  char bytes[lift::Journal::PayloadMax];
  size_t n = serializeJson(d, bytes, sizeof bytes);
  if (n >= sizeof bytes || !events.append(reinterpret_cast<uint8_t*>(bytes), n))
    storageReady = false;
}
JsonDocument status() {
  JsonDocument d;
  d["apiVersion"] = 1;
  d["contractVersion"] = Pins::ContractVersion;
  d["bootId"] = bootId;
  d["sequence"] = ++statusSequence;
  d["uptimeMs"] = millis();
  d["sampleAgeMs"] = millis() - lastSample;
  d["state"] = lift::stateName(supervisor.state);
  d["deploymentReady"] = false;
  d["position"] = counts(position);
  d["positionValid"] = false;
  d["currentFloor"] = nullptr;
  d["targetFloor"] = nullptr;
  d["motionAllowed"] = false;
  d["canMoveUp"] = false;
  d["canMoveDown"] = false;
  d["stoppedConfirmed"] = false;  // Physical qualification is still absent.
  d["safetyOk"] = supervisor.inputs.safety;
  d["home"] = supervisor.inputs.home;
  const uint32_t now = millis();
  if (fieldInputs.upper.fresh(now))
    d["upperLimit"] = fieldInputs.upper.active;
  else
    d["upperLimit"] = nullptr;
  if (fieldInputs.lower.fresh(now))
    d["lowerLimit"] = fieldInputs.lower.active;
  else
    d["lowerLimit"] = nullptr;
  if (fieldInputs.key.fresh(now))
    d["serviceKey"] = fieldInputs.key.active;
  else
    d["serviceKey"] = nullptr;
  d["inputsQualified"] = fieldInputs.qualified(now, FieldContinuityQualified);
  d["manualControl"]["active"] = manualStop.key;
  d["manualControl"]["hold"] = manualStop.hold;
  d["manualControl"]["up"] = manualStop.up;
  d["manualControl"]["down"] = manualStop.down;
  d["manualControl"]["safetyHealthy"] = manualStop.safety;
  d["manualControl"]["inputsKnown"] = manualStop.known;
  d["manualControl"]["requestedDirection"] = manualStop.requestedDirection;
  d["manualControl"]["motionEnabled"] = false;
  d["fault"] = supervisor.fault;
  auto reasons = d["blockedReasons"].to<JsonArray>();
  for (const char* r :
       {"software-motion-inhibit", "field-inputs-unqualified", "physical-qualification-required"})
    reasons.add(r);
  auto caps = d["capabilities"].to<JsonObject>();
  for (const char* cap :
       {"motion", "homing", "calibration", "floorWrite", "settingsWrite", "vfdRead", "vfdWrite",
        "rfLearn", "rfErase", "backup", "restore", "reboot", "auxiliary"})
    caps[cap] = false;
  caps["status"] = true;
  caps["vfdRead"] = vfdUartReady;
  caps["network"] = tokenConfigured();
  caps["light"] = tokenConfigured();
  caps["logs"] = storageReady;
  auto floors = d["floors"].to<JsonArray>();
  for (int f = 1; f <= 3; ++f) {
    auto floor = floors.add<JsonObject>();
    floor["floor"] = f;
    floor["name"] = *configuration.names[f - 1].data() ? String(configuration.names[f - 1].data())
                                                       : String("Landing ") + f;
    if (configuration.motion.floorsValid)
      floor["position"] = counts(configuration.motion.floors[f - 1]);
    else
      floor["position"] = nullptr;
  }
  d["light"]["on"] = supervisor.light;
  d["light"]["feedbackVerified"] = false;
  d["telemetry"] = nullptr;
  if (diagnosticVfd.healthy(now)) {
    const auto& t = diagnosticVfd.protocol().telemetry;
    d["telemetry"]["ageMs"] = now - t.sampledMs;
    d["telemetry"]["frequency"] = t.frequency;
    d["telemetry"]["current"] = t.current;
    d["telemetry"]["busVolts"] = t.busVolts;
    d["telemetry"]["temperature"] = t.temperature;
  }
  d["vfd"]["commsEnabled"] = vfdUartReady;
  d["vfd"]["healthy"] = diagnosticVfd.healthy(now);
  d["vfd"]["stopTransmitted"] = diagnosticVfd.stopTransmitted;
  d["vfd"]["stopAcknowledged"] = diagnosticVfd.acknowledged();
  d["vfd"]["monitorStopped"] = diagnosticVfd.monitorStopped(now);
  d["vfd"]["communicationFault"] = diagnosticVfd.failed;
  d["vfd"]["stopRefreshMs"] = lift::VfdTiming::StopRefreshMs;
  d["vfd"]["replyTimeoutMs"] = lift::VfdTiming::ReplyTimeoutMs;
  d["vfd"]["watchdog"]["state"] = diagnosticVfd.watchdogState(now);
  const auto& p = diagnosticVfd.protocol();
  if (p.parameters[12] >= 0) {
    d["vfd"]["watchdog"]["timeoutMs"] = p.parameters[12] * 100;
    d["vfd"]["watchdog"]["ageMs"] = now - p.parameterSampleMs[12];
  } else {
    d["vfd"]["watchdog"]["timeoutMs"] = nullptr;
    d["vfd"]["watchdog"]["ageMs"] = nullptr;
  }
  d["calibration"]["valid"] = false;
  d["calibration"]["distance"] = nullptr;
  d["devices"]["counter"] = counterReady ? "sampled-unverified" : "unavailable";
  d["devices"]["mram"] = storageReady ? "readback-unverified" : "unavailable";
  d["devices"]["rtc"] = rtcValid ? "sampled-unverified" : "time-invalid";
  d["network"]["mode"] = WiFi.status() == WL_CONNECTED ? "station" : "fallback_ap";
  d["network"]["stationIp"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  d["network"]["apIp"] = ap ? WiFi.softAPIP().toString() : "";
  d["network"]["hostname"] = "lift.local";
  d["authConfigured"] = tokenConfigured();
  return d;
}
void reply(int code, const JsonDocument& d) {
  asset.close();
  String json;
  serializeJson(d, json);
  response =
      "HTTP/1.1 " + String(code) +
      " Response\r\nContent-Type: application/json\r\nCache-Control: no-store\r\nConnection: "
      "close\r\nX-Content-Type-Options: nosniff\r\nContent-Length: " +
      String(json.length()) + "\r\n\r\n" + json;
  responseOffset = 0;
  responding = true;
}
void error(int code, const char* reason) {
  JsonDocument d;
  d["error"] = reason;
  d["apiVersion"] = 1;
  reply(code, d);
}
bool authorized() {
  auto it = request.headers.find("x-lift-api-token");
  return tokenConfigured() && it != request.headers.end() && it->second == LIFT_API_TOKEN;
}
void dispatch() {
  std::map<std::string, std::string> args;
  if (!lift::HttpRequest::form(request.query, args) ||
      !lift::HttpRequest::form(request.body, args)) {
    error(400, "invalid_arguments");
    return;
  }
  const auto& path = request.path;
  bool post = request.method == "POST";
  if (post && !authorized()) {
    error(401, "authentication_required");
    return;
  }
  if (post && path != "/api/stop") {
    uint32_t now = millis();
    if (now - lastWriteRequestMs < 250) {
      error(429, "write_rate_limited");
      return;
    }
    lastWriteRequestMs = now;
  }
  if (path == "/api/status" && !post) {
    reply(200, status());
    return;
  }
  if (path == "/api/capabilities" && !post) {
    auto s = status();
    JsonDocument d;
    d["apiVersion"] = 1;
    d["capabilities"] = s["capabilities"];
    reply(200, d);
    return;
  }
  if (path == "/api/stop" && post) {
    if (!args.empty()) {
      error(400, "unexpected_argument");
      return;
    }
    supervisor.command({true, 0, lift::Source::Web}, millis());
    if (!vfdUartReady) {
      error(503, "stop_delivery_unavailable");
      return;
    }
    diagnosticVfd.stop(millis());
    event("stop", "web", "queued-not-confirmed-stopped");
    reply(202, status());
    return;
  }
  if (path == "/api/move" && post) {
    if (manualStop.key || !manualStop.known) {
      error(409, "manual_control_priority");
      return;
    }
    uint32_t floor = 0;
    if (args.size() != 1 || !args.count("floor") ||
        !lift::parseUnsigned(args["floor"].c_str(), 3, floor) || floor < 1) {
      error(400, "invalid_floor");
      return;
    }
    const char* reason =
        supervisor.command({false, 1u << (floor - 1), lift::Source::Web}, millis());
    event("move", "web", reason ? reason : "accepted");
    if (reason)
      error(409, reason);
    else
      reply(202, status());
    return;
  }
  if (path == "/api/light" && post) {
    if (args.size() != 1 || !args.count("on") || (args["on"] != "true" && args["on"] != "false")) {
      error(400, "invalid_light_state");
      return;
    }
    supervisor.setLight(args["on"] == "true");
    digitalWrite(Pins::LightControl, supervisor.light ? HIGH : LOW);
    event("light", "web", supervisor.light ? "on-commanded" : "off-commanded");
    reply(200, status());
    return;
  }
  if (path == "/api/network" && !post) {
    auto s = status();
    JsonDocument d;
    d["apiVersion"] = 1;
    d["network"] = s["network"];
    d["ssid"] = network.staSsid;
    reply(200, d);
    return;
  }
  if (path == "/api/network" && post) {
    if (args.size() != 2 || !args.count("ssid") || !args.count("password") ||
        args["ssid"].size() > 32 || args["password"].size() > 63 ||
        (!args["password"].empty() && args["password"].size() < 8)) {
      error(400, "invalid_network_settings");
      return;
    }
    if (supervisor.runRequested() || supervisor.state == lift::State::Stopping) {
      error(409, "stop_required");
      return;
    }
    NetworkSettings updated = network;
    snprintf(updated.staSsid, sizeof updated.staSsid, "%s", args["ssid"].c_str());
    snprintf(updated.staPassword, sizeof updated.staPassword, "%s", args["password"].c_str());
    if (!networkStore.save(updated)) {
      error(500, "network_save_failed");
      return;
    }
    network = updated;
    event("network_saved", "web", "restart-required");
    JsonDocument d;
    d["apiVersion"] = 1;
    d["saved"] = true;
    d["restartRequired"] = true;
    reply(200, d);
    return;
  }
  if (path == "/api/logs/recent" && !post) {
    if (!storageReady) {
      error(503, "durable_log_unavailable");
      return;
    }
    if (!args.empty()) {
      error(400, "unexpected_argument");
      return;
    }
    JsonDocument d;
    d["apiVersion"] = 1;
    d["durable"] = true;
    auto rows = d["events"].to<JsonArray>();
    uint32_t seq = events.sequence();
    for (unsigned count = 0; count < 16 && seq; ++count, --seq) {
      uint8_t p[lift::Journal::PayloadMax];
      size_t n = sizeof p;
      if (!events.readSequence(seq, p, n))
        continue;
      JsonDocument row;
      if (deserializeJson(row, p, n))
        continue;
      row["sequence"] = seq;
      rows.add(row.as<JsonObject>());
    }
    reply(200, d);
    return;
  }
  if (path == "/api/vfd/parameters" && !post) {
    JsonDocument d;
    d["apiVersion"] = 1;
    d["available"] = diagnosticVfd.healthy(millis());
    d["reason"] = d["available"].as<bool>() ? "diagnostic-readback-only" : "vfd_unavailable";
    auto readings = d["readings"].to<JsonArray>();
    for (unsigned n = 0; n < 17; ++n) {
      auto r = readings.add<JsonObject>();
      r["number"] = n;
      const auto& p = diagnosticVfd.protocol();
      r["supported"] = n != 13;
      if (p.parameters[n] >= 0) {
        r["rawValue"] = p.parameters[n];
        r["ageMs"] = millis() - p.parameterSampleMs[n];
      } else {
        r["rawValue"] = nullptr;
        r["ageMs"] = nullptr;
      }
    }
    JsonDocument definitions;
    deserializeJson(definitions, VfdParameters::allDefinitionsJson());
    d["definitions"] = definitions;
    reply(200, d);
    return;
  }
  if (path.compare(0, 5, "/api/") == 0) {
    error(501, "unsupported_capability");
    return;
  }
  if (post || !args.empty() || path.find("..") != std::string::npos ||
      path.find('%') != std::string::npos) {
    error(404, "not_found");
    return;
  }
  String file = path == "/" ? "/index.html" : path.c_str();
  if (!fsReady || (!file.startsWith("/assets/") && file != "/index.html")) {
    error(503, "web_bundle_unavailable");
    return;
  }
  bool gzip = request.headers["accept-encoding"].find("gzip") != std::string::npos &&
              LittleFS.exists(file + ".gz");
  asset = LittleFS.open(gzip ? file + ".gz" : file, "r");
  if (!asset) {
    error(404, "asset_not_found");
    return;
  }
  const char* mime = file.endsWith(".js")    ? "text/javascript"
                     : file.endsWith(".css") ? "text/css"
                                             : "text/html";
  response = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: " + String(mime) +
             "\r\nX-Content-Type-Options: nosniff\r\nContent-Security-Policy: default-src 'self'; "
             "script-src 'self'; style-src 'self' 'unsafe-inline'; connect-src 'self'; "
             "frame-ancestors 'none'\r\nVary: Accept-Encoding\r\nCache-Control: " +
             String(file == "/index.html" ? "no-cache" : "public, max-age=31536000, immutable") +
             "\r\nContent-Length: " + String(asset.size()) + "\r\n" +
             (gzip ? "Content-Encoding: gzip\r\n" : "") + "\r\n";
  responseOffset = 0;
  responding = true;
}
void serviceHttp(uint32_t now) {
  if (!client) {
    asset.close();
    response = "";
    client = http.available();
    if (!client)
      return;
    client.setTimeout(10);
    connectedAt = now;
    request = lift::HttpRequest{};
    responding = false;
  }
  if (now - connectedAt > (responding ? 5000u : 500u)) {
    client.stop();
    asset.close();
    return;
  }
  if (responding) {
    if (responseOffset < response.length()) {
      size_t count = std::min(size_t(512), size_t(response.length() - responseOffset));
      responseOffset +=
          client.write(reinterpret_cast<const uint8_t*>(response.c_str() + responseOffset), count);
    } else if (asset && asset.available()) {
      uint8_t chunk[512];
      size_t pos = asset.position(), count = asset.read(chunk, sizeof chunk),
             sent = client.write(chunk, count);
      if (sent < count)
        asset.seek(pos + sent);
    } else {
      client.stop();
      asset.close();
      response = "";
    }
    return;
  }
  for (unsigned n = 0; n < 128 && client.available(); ++n) {
    auto result = request.feed(char(client.read()));
    if (result == lift::HttpRequest::Result::Complete) {
      dispatch();
      break;
    }
    if (result != lift::HttpRequest::Result::More) {
      error(result == lift::HttpRequest::Result::TooLarge ? 413 : 400,
            "invalid_or_oversized_request");
      break;
    }
  }
}
void startAp() {
  if (ap)
    return;
  WiFi.mode(WIFI_AP_STA);
  ap = WiFi.softAP(network.apSsid, network.apPassword);
}
}  // namespace
void setup() {
  snprintf(bootId, sizeof bootId, "%08x%08x", esp_random(), esp_random());
  // ROM output occurs before setup. Suppress application UART0 logging before reclaiming pins.
  Serial0.setDebugOutput(false);
  Serial0.end();
  esp_rom_install_channel_putc(1, nullptr);
  esp_rom_install_channel_putc(2, nullptr);
  esp_log_level_set("*", ESP_LOG_NONE);
  esp_log_set_vprintf(discardUartLog);
  for (int p : {Pins::VfdCommsEnable, Pins::LightControl, Pins::RfLearn, Pins::StatusLed}) {
    digitalWrite(p, LOW);
    pinMode(p, OUTPUT);
  }
  for (int p : {Pins::CounterCs, Pins::MramCs}) {
    digitalWrite(p, HIGH);
    pinMode(p, OUTPUT);
  }
  for (int p : {Pins::SafetyLoop, Pins::HomeSwitch, Pins::HoldToRun, Pins::ServiceUp,
                Pins::ServiceDown, Pins::UpperLimit, Pins::LowerLimit, Pins::ServiceKey, Pins::RfD0,
                Pins::RfD1, Pins::RfD2, Pins::RfD3, Pins::RfD4, Pins::RfTxId, Pins::RfModeInd})
    pinMode(p, INPUT);
  Serial.begin(115200);
  VfdSerial.begin(9600, SERIAL_8N1, Pins::VfdRx, Pins::VfdTx);
  vfdUartReady = bool(VfdSerial);
  // OE is communications only. It never supplies hardwired motion permission.
  digitalWrite(Pins::VfdCommsEnable, vfdUartReady ? HIGH : LOW);
  SPI.begin(Pins::SpiClock, Pins::SpiMiso, Pins::SpiMosi);
  Wire.begin(Pins::I2cSda, Pins::I2cScl, 100000);
  Wire.setTimeOut(5);
  counterReady = counter.begin();
  storageReady = mram.begin() && snapshots.recover() && configurationRecords.recover() &&
                 safetyRecords.recover() && events.recover();
  if (storageReady && safetyLedger.load() &&
      (safetyLedger.faultLatched || safetyLedger.unfinishedMotion))
    supervisor.fail("durable_fault_or_unfinished_motion");
  if (storageReady && configurationStore.load(configuration))
    supervisor.config = configuration.motion;
  if (storageReady) {
    uint8_t p[24];
    size_t n = sizeof p;
    if (snapshots.latest(p, n) && n == sizeof p && lift::get32(p) == 1 &&
        p[17] == uint8_t(lift::State::Fault))
      supervisor.fail("previous_boot_fault");
  }
  // Snapshots are historical evidence, never proof of incremental position continuity.
  fsReady = LittleFS.begin(false) && verifyWebBundle();
  networkStore.begin();
  if (!networkStore.load(network)) {
    snprintf(network.apSsid, sizeof network.apSsid, "%s", WIFI_AP_SSID);
    snprintf(network.apPassword, sizeof network.apPassword, "%s", WIFI_AP_PASSWORD);
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname("lift");
  if (*network.staSsid)
    WiFi.begin(network.staSsid, network.staPassword);
  startAp();
  http.begin();
  event("boot", "controller", "hardware-inhibited");
}
void loop() {
  uint32_t now = millis();
  // Sample release paths before any UART scheduling, HTTP, SPI or persistent logging.
  const char* manualEvent = manualStop.update(
      digitalRead(Pins::ServiceKey), digitalRead(Pins::HoldToRun), digitalRead(Pins::ServiceUp),
      digitalRead(Pins::ServiceDown), digitalRead(Pins::SafetyLoop), now);
  if (manualEvent) {
    supervisor.stop(now);
    diagnosticVfd.stop(now);
  }
  serviceVfd(now);
  if (diagnosticVfd.failed && !vfdFailureLogged) {
    event("vfd_communication_timeout", "vfd", "fault-latched-stop-refresh-only");
    vfdFailureLogged = true;
  }
  const char* watchdogState = diagnosticVfd.watchdogState(now);
  if (strcmp(watchdogState, previousWatchdogState)) {
    event("vfd_watchdog_state", "vfd", watchdogState);
    previousWatchdogState = watchdogState;
  }
  if (manualEvent)
    event(manualEvent, "manual", "stop-queued-not-confirmed-stopped");
  if (now - lastSample >= 10) {
    counterReady = counterReady && counter.sample(position);
    lift::Inputs i;
    i.sampleMs = now;
    i.position = position;
    i.encoderHealthy = counterReady;
    i.storageHealthy = storageReady;
    i.safety = digitalRead(Pins::SafetyLoop) == LOW;
    i.home = digitalRead(Pins::HomeSwitch) == LOW;
    i.hold = digitalRead(Pins::HoldToRun) == LOW;
    i.up = digitalRead(Pins::ServiceUp) == LOW;
    i.down = digitalRead(Pins::ServiceDown) == LOW;
    fieldInputs.upper.sample(digitalRead(Pins::UpperLimit), now);
    fieldInputs.lower.sample(digitalRead(Pins::LowerLimit), now);
    fieldInputs.key.sample(digitalRead(Pins::ServiceKey), now);
    i.upper = fieldInputs.upper.active;
    i.lower = fieldInputs.lower.active;
    i.key = fieldInputs.key.active;
    i.limitsKnown = i.keyKnown = fieldInputs.qualified(now, FieldContinuityQualified);
    i.hardwareReady = Pins::DeploymentReady;
    i.communicationHealthy = diagnosticVfd.healthy(now);
    i.stopped = diagnosticVfd.monitorStopped(now);
    i.frequency = diagnosticVfd.protocol().telemetry.frequency;
    supervisor.tick(i, now);
    lastSample = now;
    digitalWrite(Pins::RfLearn, LOW);
    if (previousFault != supervisor.fault) {
      if (storageReady && !safetyLedger.latchFault())
        storageReady = false;
      event("fault", "controller", supervisor.fault);
      previousFault = supervisor.fault;
    }
  }
  if (now - lastRtc >= 1000) {
    rtcValid = rtc.readUnix(unixTime);
    lastRtc = now;
  }
  if (storageReady && now - lastPersist >= 1000) {
    uint8_t p[24] = {};
    lift::put32(p, 1);
    lift::put32(p + 4, now);
    lift::put32(p + 8, uint64_t(position));
    lift::put32(p + 12, uint64_t(position) >> 32);
    p[16] = 0;
    p[17] = uint8_t(supervisor.state);
    storageReady = snapshots.append(p, sizeof p);
    lastPersist = now;
  }
  serviceHttp(now);
  if (now - lastNetwork >= 5000) {
    lastNetwork = now;
    if (WiFi.status() != WL_CONNECTED)
      startAp();
    else if (ap) {
      WiFi.softAPdisconnect(true);
      ap = false;
      WiFi.mode(WIFI_STA);
      MDNS.begin("lift");
      MDNS.addService("http", "tcp", 80);
    }
  }
  delay(1);
}
