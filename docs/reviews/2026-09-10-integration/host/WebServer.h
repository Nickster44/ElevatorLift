#pragma once
#include "Arduino.h"
#include <functional>
constexpr int HTTP_GET=0,HTTP_POST=1;
class WebServer {
 public:
  std::map<std::pair<std::string,int>,std::function<void()>> routes;
  std::map<std::string,String> args,headers;
  int code=0;String body,type;
  WebServer(int){}
  void collectHeaders(const char**,int){}
  void on(const char* p,int m,std::function<void()> cb){routes[{p,m}]=cb;}
  bool hasArg(const char* k){return args.count(k);}
  String arg(const char* k){return args[k];}
  String header(const char* k){return headers[k];}
  void send(int c,const char* t,const String& b){code=c;type=t;body=b;}
  void begin(){}
  void handleClient(){}
  void call(const char* p,int m,std::map<std::string,String> a={}){args=a;code=0;body="";auto i=routes.find({p,m});if(i==routes.end()){code=404;return;}i->second();}
};
