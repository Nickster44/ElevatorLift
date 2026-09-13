#pragma once
#include "Arduino.h"
constexpr int WL_CONNECTED=3,WIFI_STA=1,WIFI_AP=2,WIFI_AP_STA=3;
struct FakeIp {String toString(){return "192.0.2.1";} operator String(){return toString();}};
struct FakeWiFi {
  int status(){return 0;}
  void mode(int){}
  void setHostname(const char*){}
  void begin(const char*,const char*){}
  void softAP(const char*,const char*){}
  void softAPdisconnect(bool){}
  void reconnect(){}
  FakeIp localIP(){return {};}
  FakeIp softAPIP(){return {};}
};
inline FakeWiFi WiFi;
