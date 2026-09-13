// Executes unmodified application logic, not a reimplementation of its state machine.
#include "../../../../firmware/src/main.cpp"
#include <iostream>
int failures=0,checks=0;
void check(bool result,const char* name){++checks;std::cout<<(result?"CONFIRMED ":"NOT REPRODUCED ")<<name<<"\n";if(!result)++failures;}
void reset(){
  fakeMs=1000;fakePins.clear();
  fakePins[Pins::SafetyLoop]=LOW;
  for(int p:{Pins::ButtonTop,Pins::ButtonRight,Pins::ButtonBottom,Pins::ButtonLeft,Pins::ButtonCenter})fakePins[p]=LOW;
  motionState=MotionState::Idle;activeCommand={};pulsePositionCounts=0;lastFault="";lastVfdCommand="";vfdRxBuffer="";
  VfdSerial.sent.clear();VfdSerial.incoming="";lastVfdCommandMs=0;lastStopCommandMs=0;
  restartRequested=false;ESP.restarted=false;fallbackApEnabled=false;mdnsStarted=false;lastPersistMs=0;lastWifiCheckMs=0;lastButtonTop=lastButtonRight=lastButtonBottom=lastButtonLeft=lastButtonCenter=false;
  Preferences::data.clear();Preferences::failWrites=false;store.begin();liftSettingsStore.begin();networkConfig.begin();
  storedState={};networkSettings={};liftSettings={};eventLog.begin();
  server.headers.clear();server.headers[ApiAuth::TokenHeader]="review-test-token";
}
int main(){
  setupWebServer();reset();
  check(VfdProtocol::stop()=="(3)84" && VfdProtocol::monitor()=="(0)81","protocol documented stop/monitor vectors");
  check(VfdProtocol::run(VfdDirection::Forward,450)=="(10450)4;","protocol 45Hz forward vector");
  check(VfdProtocol::isStopAck("noise(3)84unframed"),"DEFECT acknowledgement accepts unframed substring");
  reset();setup();check(motionState==MotionState::Idle && readPositionCounts()==0,"DEFECT fresh empty NVS boots idle at invented zero");
  check(VfdSerial.sent.size()==4 && VfdSerial.sent[0]!=VfdProtocol::stop(),"DEFECT boot writes four drive parameters without initial STOP or ACK");
  reset();requestMoveToFloor(2);fakeMs=3600000;serviceMotion();check(motionState==MotionState::Moving && !VfdSerial.sent.empty(),"DEFECT no encoder pulses or VFD replies for one hour still requests run");
  reset();requestMoveToFloor(2);setPositionCounts(25000);serviceMotion();check(motionState==MotionState::Moving,"DEFECT overshot stop window continues forward");
  reset();vfdRxBuffer="(3)84";beginStopping();serviceVfdRx();check(motionState==MotionState::Idle,"DEFECT stale stop ACK marks new stop idle");
  reset();enterFault("test_fault");server.call("/api/stop",HTTP_POST);VfdSerial.incoming="(3)84";serviceVfdRx();check(motionState==MotionState::Idle && requestMoveToFloor(2),"DEFECT STOP then ACK bypasses fault latch without explicit reset");
  reset();fakePins[Pins::SafetyLoop]=HIGH;check(!requestMoveToFloor(2)&&motionState==MotionState::Fault,"safety-open request rejected and STOP emitted");
  reset();requestMoveToFloor(2);fakePins[Pins::UpperLimit]=LOW;serviceMotion();check(motionState==MotionState::Fault,"forward active upper limit faults under provisional GPIO mapping");
  reset();beginStopping();fakeMs+=2001;serviceMotion();check(motionState==MotionState::Fault,"missing stop ACK timeout faults");
  auto n=VfdSerial.sent.size();fakeMs+=10000;serviceMotion();check(VfdSerial.sent.size()==n,"DEFECT fault state stops STOP retrying");
  reset();setPositionCounts(40000);fakePins[Pins::HomeSwitch]=LOW;serviceMotion();check(readPositionCounts()==0,"DEFECT top HOME resets coordinate to floor-one zero");
  reset();server.call("/api/settings",HTTP_POST,{{"stopOffsetCounts","17"}});requestMoveToFloor(2);check(server.code==200&&liftSettings.stopOffsetCounts==17&&activeCommand.stopOffsetCounts==3500,"DEFECT accepted stored stopping offset not used by motion");
  server.call("/api/settings",HTTP_POST,{{"normalRunTenthsHz","700"}});serviceMotion();check(server.code==200 && lastVfdCommand==VfdProtocol::run(VfdDirection::Forward,700),"DEFECT settings update changes commanded speed during movement");
  reset();server.call("/api/move",HTTP_POST,{{"floor","258"}});check(server.code==202&&activeCommand.floor==2,"DEFECT invalid floor258 wraps to floor2");
  reset();server.call("/api/move",HTTP_POST,{{"floor","2junk"}});check(server.code==202,"DEFECT partially numeric command accepted");
  reset();server.call("/api/vfd/parameter",HTTP_POST,{{"number","266"},{"value","1000"}});check(server.code==202,"DEFECT parameter266 wraps to FMAX10");
  reset();requestMoveToFloor(2);server.call("/api/vfd/parameter",HTTP_POST,{{"number","3"},{"value","10"}});check(server.code==409,"parameter write blocked while moving");
  server.headers.clear();server.call("/api/vfd/parameter",HTTP_GET,{{"number","3"}});check(server.code==202&&!VfdSerial.sent.empty(),"DEFECT unauthenticated parameter read sends UART during motion without arbiter");
  reset();server.headers.clear();server.call("/api/move",HTTP_POST,{{"floor","2"}});check(server.code==401,"configured token rejects unauthorized writes");
  reset();requestMoveToFloor(2);server.call("/api/reboot",HTTP_POST);check(server.code==202&&restartRequested&&motionState==MotionState::Moving,"DEFECT reboot accepted while moving without STOP");
  fakeMs=restartAtMs;loop();check(ESP.restarted&&motionState==MotionState::Moving,"DEFECT main loop restarts with active run state");
  reset();server.call("/api/light/toggle",HTTP_POST);check(server.code==404,"DEFECT UI light endpoint absent");
  reset();server.call("/api/settings",HTTP_POST,{{"homingTimeoutMs","99"},{"logRetentionRecords","100"}});check(server.code==200&&liftSettings.homingTimeoutMs==30000&&liftSettings.logRetentionRecords==2048,"DEFECT unsupported advertised setting fields silently ignored");
  reset();StoredLiftState x{};x.currentPosition=123456;StoredLiftState y{};check(store.save(x)&&store.load(y)&&y.currentPosition==123456,"position persistence round-trip in simulated Preferences");
  Preferences::data["lift/state"][12]^=1;check(!store.load(y),"position CRC rejects corrupted simulated blob");
  reset();Preferences::failWrites=true;servicePersistence();check(motionState==MotionState::Idle,"DEFECT periodic persistence failure not surfaced as fault");
  reset();for(int i=0;i<40;i++)eventLog.append(EventCode::Boot);String log=eventLog.jsonRecent();check(log.find("\"seq\":9,")!=String::npos && log.find("\"seq\":1,")==String::npos,"log retention is32 RAM events despite2048 setting");
  reset();fakePins[Pins::ButtonLeft]=HIGH;serviceButtons();check(motionState==MotionState::Moving,"DEFECT initial high button interpreted as new command");
  reset();std::strcpy(networkSettings.staSsid,"quote\"ssid");check(jsonNetworkStatus().find("quote\"ssid")!=String::npos,"DEFECT network SSID emitted without JSON escaping");
  reset();fakePins[Pins::UpperLimit]=LOW;fakePins[Pins::LowerLimit]=LOW;check(requestMoveToFloor(2),"DEFECT conflicting limits not rejected at command acceptance");
  reset();setPositionCounts(700);serviceMotion();check(motionState==MotionState::Idle,"DEFECT unexpected idle movement not detected");
  reset();fakePins[Pins::ButtonLeft]=HIGH;fakePins[Pins::ButtonTop]=HIGH;serviceButtons();check(activeCommand.floor==2,"DEFECT simultaneous floor buttons silently choose code-order winner");
  reset();fakePins[Pins::SafetyLoop]=HIGH;setup();check(motionState==MotionState::Fault&&VfdSerial.sent.size()==4&&VfdSerial.sent.back()!=VfdProtocol::stop(),"DEFECT safety-open boot writes drive parameters but never sends STOP");
  check(VfdProtocol::run(VfdDirection::Forward,100)=="(10100)43"&&VfdProtocol::run(VfdDirection::Reverse,1000)=="(21000)44"&&VfdProtocol::setParameter(13,1)=="(4130001)::"&&VfdProtocol::getParameter(0)=="(500)>6","additional original EM01 manual transmit vectors");
  reset();requestMoveToFloor(3);setPositionCounts(12000);
  std::cout<<"STATUS_JSON "<<jsonStatus()<<"\n";
  std::cout<<"Checks="<<checks<<" unexpected="<<failures<<"\n";
  return failures?1:0;
}
