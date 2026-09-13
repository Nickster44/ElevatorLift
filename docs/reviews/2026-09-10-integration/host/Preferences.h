#pragma once
#include "Arduino.h"
class Preferences {
  std::string ns;
 public:
  inline static std::map<std::string,std::vector<uint8_t>> data;
  inline static bool failWrites=false;
  bool begin(const char* n,bool){ns=n;return true;}
  bool isKey(const char* k){return data.count(ns+"/"+k);}
  size_t getBytes(const char* k,void* p,size_t n){auto& v=data[ns+"/"+k];if(v.size()>n)return 0;memcpy(p,v.data(),v.size());return v.size();}
  size_t putBytes(const char* k,const void* p,size_t n){if(failWrites)return 0;auto b=static_cast<const uint8_t*>(p);data[ns+"/"+k]={b,b+n};return n;}
  bool remove(const char* k){return data.erase(ns+"/"+k);}
};
