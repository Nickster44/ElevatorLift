#pragma once
struct FakeMdns {bool begin(const char*){return true;} void addService(const char*,const char*,int){}};
inline FakeMdns MDNS;
