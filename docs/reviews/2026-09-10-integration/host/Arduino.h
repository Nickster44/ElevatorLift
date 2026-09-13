#pragma once
// Review-only host substitutes. No serial port, GPIO, network or real NVS access.
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <type_traits>
class String : public std::string {
 public:
  using std::string::string;
  String(const std::string& s):std::string(s){}
  String& operator+=(const String& s){append(s);return *this;}
  String& operator+=(const char* s){append(s);return *this;}
  String& operator+=(char c){push_back(c);return *this;}
  template<class T, std::enable_if_t<std::is_arithmetic_v<T>,int> =0>
  String& operator+=(T n){append(std::to_string(n));return *this;}
  long toInt() const {return std::strtol(c_str(),nullptr,10);}
  int indexOf(const char* s) const {auto n=find(s);return n==npos ? -1 : int(n);}
  void remove(size_t i,size_t n){erase(i,n);}
};
#define IRAM_ATTR
constexpr int LOW=0,HIGH=1,INPUT=0,INPUT_PULLUP=2,OUTPUT=1,RISING=3,SERIAL_8N1=0;
inline uint32_t fakeMs=0;
inline std::map<int,int> fakePins;
inline uint32_t millis(){return fakeMs;}
inline void delay(uint32_t ms){fakeMs+=ms;}
inline int digitalRead(int p){return fakePins.count(p)?fakePins[p]:HIGH;}
inline void digitalWrite(int p,int v){fakePins[p]=v;}
inline void pinMode(int,int){}
inline void noInterrupts(){}
inline void interrupts(){}
inline int digitalPinToInterrupt(int p){return p;}
inline void attachInterrupt(int,void(*)(),int){}
class HardwareSerial {
 public:
  std::vector<String> sent;
  String incoming;
  HardwareSerial(int){}
  template<class... T> void begin(T...){}
  void print(const String& s){sent.push_back(s);}
  template<class T> void println(T){}
  int available(){return int(incoming.size());}
  int read(){char c=incoming[0];incoming.erase(0,1);return c;}
};
inline HardwareSerial Serial(0);
struct FakeEsp {bool restarted=false;void restart(){restarted=true;}};
inline FakeEsp ESP;
