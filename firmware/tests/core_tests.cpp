#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "core/Configuration.h"
#include "core/ControllerSession.h"
#include "core/Devices.h"
#include "core/DriveScheduler.h"
#include "core/Em01.h"
#include "core/HttpRequest.h"
#include "core/ParameterJobs.h"
#include "core/Rf.h"
#include "core/SafetyLedger.h"
#include "core/Supervisor.h"
using namespace lift;
unsigned checks = 0;
#define CHECK(x)                                                                                 \
  do {                                                                                           \
    ++checks;                                                                                    \
    if (!(x))                                                                                    \
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " #x); \
  } while (0)
struct Rig {
  Supervisor s;
  Inputs i;
  uint32_t now = 0;
  Rig() {
    i.hardwareReady = i.safety = i.limitsKnown = i.encoderHealthy = i.storageHealthy =
        i.communicationHealthy = i.stopped = true;
    s.config.floors = {0, 20000, 40000};
    s.config.home = 39500;
    s.config.floorsValid = true;
    s.config.calibration = {true, 1000, 1, 1, 0, 0, 0};
    tick(0);
    tick(400);
  }
  void tick(uint32_t time) {
    now = time;
    i.sampleMs = time;
    s.tick(i, time);
  }
  void known() {
    s.positionValid = true;
    s.state = State::Idle;
  }
  void move() {
    known();
    CHECK(!s.command({false, 2, Source::Web}, now));
  }
};
struct Memory : ByteStore {
  std::vector<uint8_t> bytes = std::vector<uint8_t>(Mram::Capacity, 0xff);
  int budget = -1;
  bool read(uint32_t a, uint8_t* p, size_t n) override {
    if (a + n > bytes.size())
      return false;
    memcpy(p, bytes.data() + a, n);
    return true;
  }
  bool write(uint32_t a, const uint8_t* p, size_t n) override {
    if (a + n > bytes.size())
      return false;
    for (size_t j = 0; j < n; ++j) {
      if (budget == 0)
        return false;
      if (budget > 0)
        --budget;
      bytes[a + j] = p[j];
    }
    return true;
  }
};
struct ScriptSpi : SpiBus {
  struct Exchange {
    int cs;
    std::vector<uint8_t> tx, rx;
  };
  std::vector<Exchange> queue;
  size_t at = 0;
  bool transfer(int cs, const uint8_t* t, uint8_t* r, size_t n) override {
    CHECK(at < queue.size());
    auto& x = queue[at++];
    CHECK(cs == x.cs);
    CHECK(std::vector<uint8_t>(t, t + n) == x.tx);
    memcpy(r, x.rx.data(), n);
    return true;
  }
  void add(int cs, std::vector<uint8_t> t, std::vector<uint8_t> r) { queue.push_back({cs, t, r}); }
};
void motionTests() {
  {
    Rig r;
    CHECK(r.s.state == State::Unknown);
    CHECK(!r.s.positionValid);
    CHECK(r.s.command({false, 1}, r.now));
    r.i.home = true;
    r.tick(410);
    CHECK(!r.s.positionValid);
    CHECK(r.i.position == 0);
  }
  {
    Rig r;
    r.move();
    CHECK(r.s.runRequested());
    CHECK(!r.s.guarded());
    r.s.stop(420);
    CHECK(r.s.state == State::Stopping);
    r.i.stopped = false;
    r.tick(430);
    CHECK(r.s.state == State::Stopping);
    r.tick(6000);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.move();
    r.i.stopped = false;
    r.tick(1401);
    CHECK(std::string(r.s.fault) == "encoder_no_progress");
    r.s.command({true, 0}, 1500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.move();
    r.i.position = 21000;
    r.tick(500);
    CHECK(std::string(r.s.fault) == "overshoot");
  }
  {
    Rig r;
    r.move();
    r.i.position = -1;
    r.tick(500);
    CHECK(std::string(r.s.fault) == "wrong_direction");
  }
  {
    Rig r;
    r.known();
    r.i.upper = r.i.lower = true;
    r.tick(500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.known();
    r.i.position = 1;
    r.tick(500);
    CHECK(std::string(r.s.fault) == "unexpected_idle_movement");
  }
  {
    Rig r;
    r.move();
    r.i.communicationHealthy = false;
    r.tick(500);
    CHECK(std::string(r.s.fault) == "communication_lost");
  }
  {
    Rig r;
    r.move();
    r.i.encoderHealthy = false;
    r.tick(500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.move();
    r.i.safety = false;
    r.tick(500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.move();
    r.i.storageHealthy = false;
    r.tick(500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.known();
    CHECK(r.s.command({false, 3}, 500));
    CHECK(r.s.state == State::Idle);
    CHECK(!r.s.command({true, 3}, 500));
    CHECK(!r.s.runRequested());
  }
  {
    Rig r;
    r.i.hardwareReady = false;
    r.tick(500);
    r.known();
    CHECK(r.s.command({false, 2}, 500));
  }
  {
    Rig r;
    r.move();
    r.i.upper = true;
    r.tick(500);
    CHECK(r.s.state == State::Fault);
  }
  {
    Rig r;
    r.move();
    CHECK(r.s.configure(r.s.config));
    r.i.key = true;
    r.s.stop(410);
    r.i.stopped = true;
    r.tick(720);
    CHECK(r.s.state == State::Fault);
    CHECK(std::string(r.s.fault) == "landing_outside_tolerance");
  }
  {
    Rig r;
    r.known();
    r.i.key = true;
    r.tick(500);
    auto next = r.s.config;
    next.speed = 500;
    CHECK(!r.s.configure(next));
    CHECK(!r.s.config.calibration.valid);
    CHECK(r.s.positionValid);
  }
  {
    Rig r;
    r.i.key = true;
    r.tick(500);
    r.i.hold = true;
    r.tick(510);
    CHECK(!r.s.home(510));
    r.i.position = 100;
    r.i.stopped = false;
    r.tick(600);
    r.i.position = 200;
    r.i.home = true;
    r.tick(700);
    CHECK(r.s.state == State::Stopping);
    r.i.position = 250;
    r.i.stopped = true;
    r.tick(800);
    r.tick(1110);
    CHECK(!r.s.positionValid);
    CHECK(r.s.referencePending);
    CHECK(r.s.referencePosition == 39550);
    r.s.confirmReference(true);
    CHECK(r.s.positionValid);
  }
  {
    Rig r;
    r.i.key = true;
    r.tick(500);
    r.i.hold = r.i.up = true;
    r.tick(510);
    CHECK(r.s.state == State::Service);
    r.i.hold = false;
    r.tick(520);
    CHECK(r.s.state == State::Stopping);
  }
  {
    Rig r;
    r.known();
    r.i.key = true;
    r.tick(500);
    r.i.hold = true;
    r.tick(510);
    CHECK(!r.s.calibrate(510));
    r.i.stopped = false;
    r.i.frequency = 450;
    for (uint32_t t = 600; t <= 3600; t += 100) {
      r.i.position += 100;
      r.tick(t);
    }
    CHECK(r.s.state == State::Stopping);
    r.i.position += 800;
    r.i.stopped = true;
    r.tick(3700);
    r.tick(4010);
    CHECK(r.s.config.calibration.valid);
    CHECK(r.s.config.calibration.distance == 800);
  }
}
void protocolTests() {
  CHECK(Em01::frame("(10100)") == "(10100)43");
  CHECK(Em01::frame("(3)") == "(3)84");
  CHECK(!Em01::valid("junk(3)84"));
  CHECK(!Em01::valid("(3)85"));
  Em01 d;
  CHECK(d.request('3', 0, 0, 0));
  CHECK(d.transmit(0) == "(3)84");
  for (char c : std::string("(3)84"))
    d.receive(c, 1);
  CHECK(d.stopAcknowledged);
  CHECK(!d.confirmedStopped(1));
  CHECK(d.request('0', 0, 0, 2));
  d.transmit(2);
  for (char c : Em01::frame("(032600000340000"))
    d.receive(c, 3);
  CHECK(d.busy());
  for (char c : Em01::frame("(032600000340000)"))
    d.receive(c, 4);
  CHECK(d.confirmedStopped(4));
  CHECK(!d.healthy(600));
  CHECK(d.request('4', 10, 4, 10));
  d.transmit(10);
  for (char c : Em01::frame("(4)"))
    d.receive(c, 11);
  CHECK(d.transmit(12) == Em01::frame("(504)"));
  for (char c : Em01::frame("(50010)"))
    d.receive(c, 13);
  CHECK(d.result == Em01::Result::Verified);
  CHECK(d.parameters[4] == 10);
  Em01 timeout;
  CHECK(timeout.request('0', 0, 0, 0));
  CHECK(!timeout.transmit(0).empty());
  CHECK(!timeout.transmit(150).empty());
  CHECK(!timeout.transmit(300).empty());
  CHECK(timeout.transmit(450).empty());
  CHECK(timeout.result == Em01::Result::Timeout);
  CHECK(!timeout.request('0', 0, 0, 500));
  for (char c : Em01::frame("(032600000340000)"))
    timeout.receive(c, 501);
  CHECK(!timeout.healthy(501));
}
void storageTests() {
  Memory m;
  Journal j(m, 0, 2);
  CHECK(j.recover());
  uint8_t a[4] = {1, 2, 3, 4}, b[8];
  size_t n = 8;
  CHECK(!j.latest(b, n));
  CHECK(j.append(a, 4));
  auto baseline = m.bytes;
  for (int cut = 0; cut <= 580; ++cut) {
    m.bytes = baseline;
    m.budget = cut;
    Journal writer(m, 0, 2);
    CHECK(writer.recover());
    uint8_t replacement[4] = {5, 6, 7, 8};
    writer.append(replacement, 4);
    m.budget = -1;
    Journal recovered(m, 0, 2);
    CHECK(recovered.recover());
    n = 8;
    CHECK(recovered.latest(b, n));
    CHECK(b[0] == 1 || b[0] == 5);
  }
  m.bytes = baseline;
  m.bytes[576 + 20] ^= 1;
  Journal corrupt(m, 0, 2);
  CHECK(corrupt.recover());
  n = 8;
  CHECK(!corrupt.latest(b, n));
  ScriptSpi spi;
  spi.add(10, {0x88, 3}, {0, 0});
  spi.add(10, {0x90, 0}, {0, 0});
  spi.add(10, {0x48, 0}, {0, 3});
  spi.add(10, {0x50, 3}, {0, 0});
  spi.add(10, {0x30}, {0});
  auto sample = [&](uint32_t raw) {
    spi.add(10, {0x48, 0}, {0, 3});
    spi.add(10, {0x50, 3}, {0, 0});
    spi.add(10, {0x70, 0}, {0, 8});
    spi.add(10, {0x60, 0, 0, 0, 0},
            {0, uint8_t(raw >> 24), uint8_t(raw >> 16), uint8_t(raw >> 8), uint8_t(raw)});
  };
  sample(0xfffffffe);
  sample(1);
  Counter c(spi, 10);
  CHECK(c.begin());
  int64_t p;
  CHECK(c.sample(p));
  CHECK(p == 0);
  CHECK(c.sample(p));
  CHECK(p == 3);
  Memory configMemory;
  Journal configJournal(configMemory, 2048, 2);
  CHECK(configJournal.recover());
  ConfigurationStore configurations(configJournal);
  Configuration config;
  config.motion.floors = {0, 10000, 20000};
  config.motion.home = 19900;
  config.motion.floorsValid = true;
  strcpy(config.names[0].data(), "Bottom");
  CHECK(configurations.save(config));
  Configuration restored;
  CHECK(configurations.load(restored));
  CHECK(restored.motion.home == 19900);
  CHECK(std::string(restored.names[0].data()) == "Bottom");
  CHECK(!restored.motion.calibration.valid);
}
struct SpiMemory : SpiBus {
  std::vector<uint8_t> memory = std::vector<uint8_t>(524288);
  bool enabled = false;
  uint32_t lastWord = 0;
  bool present = true;
  bool transfer(int cs, const uint8_t* t, uint8_t* r, size_t n) override {
    CHECK(cs == 9);
    memset(r, 0, n);
    if (!present) {
      memset(r, 255, n);
      return true;
    }
    if (t[0] == 0x9f) {
      CHECK(n == 20);
      r[4] = 0x29;
      r[5] = 0x55;
    } else if (t[0] == 0xb5) {
      CHECK(n == 5);
    } else if (t[0] == 6) {
      enabled = true;
      CHECK(n == 1);
    } else if (t[0] == 4) {
      enabled = false;
      CHECK(n == 1);
    } else {
      CHECK(t[0] == 2 || t[0] == 3);
      CHECK(n >= 6 && !(n & 1));
      lastWord = (uint32_t(t[1]) << 16) | (uint32_t(t[2]) << 8) | t[3];
      uint32_t a = lastWord * 2;
      CHECK(a + n - 4 <= memory.size());
      if (t[0] == 2) {
        CHECK(enabled);
        memcpy(memory.data() + a, t + 4, n - 4);
      } else
        memcpy(r + 4, memory.data() + a, n - 4);
    }
    return true;
  }
};
struct ClockBus : I2cBus {
  uint32_t value = 1700000000;
  bool powerLost = false, available = true, inconsistent = false;
  bool read(uint8_t device, uint8_t reg, uint8_t* p, size_t n) override {
    CHECK(device == 0x52);
    if (!available)
      return false;
    if (reg == 0x0e) {
      CHECK(n == 1);
      *p = powerLost ? 1 : 0;
    } else {
      CHECK(reg == 0x1b && n == 4);
      put32(p, value);
      if (inconsistent)
        ++value;
    }
    return true;
  }
};
void deviceTests() {
  Memory ledgerMemory;
  Journal ledgerRecords(ledgerMemory, 4096, 2);
  CHECK(ledgerRecords.recover());
  SafetyLedger ledger(ledgerRecords);
  CHECK(!ledger.load());
  CHECK(ledger.prepareMotion());
  CHECK(!ledger.confirmStop(false));
  Journal bootRecords(ledgerMemory, 4096, 2);
  CHECK(bootRecords.recover());
  SafetyLedger boot(bootRecords);
  CHECK(boot.load());
  CHECK(boot.unfinishedMotion);
  CHECK(!boot.prepareMotion());
  CHECK(boot.latchFault());
  CHECK(boot.confirmStop(true));
  CHECK(!boot.reset(false));
  CHECK(boot.reset(true));
  SpiMemory bus;
  Mram ram(bus, 9);
  CHECK(ram.begin());
  uint8_t value[4] = {10, 20, 30, 40}, out[4];
  CHECK(ram.write(0x20002, value, 4));
  CHECK(bus.lastWord == 0x10001);
  CHECK(!bus.enabled);
  CHECK(ram.read(0x20002, out, 4));
  CHECK(memcmp(value, out, 4) == 0);
  CHECK(!ram.read(1, out, 4));
  CHECK(!ram.write(524286, value, 4));
  CHECK(!ram.write(0, value, 3));
  SpiMemory absent;
  absent.present = false;
  Mram missing(absent, 9);
  CHECK(!missing.begin());
  ClockBus i2c;
  Rtc rtc(i2c);
  uint32_t time = 0;
  CHECK(rtc.readUnix(time));
  CHECK(time == 1700000000);
  i2c.powerLost = true;
  CHECK(!rtc.readUnix(time));
  i2c.powerLost = false;
  i2c.inconsistent = true;
  CHECK(!rtc.readUnix(time));
  i2c.available = false;
  CHECK(!rtc.readUnix(time));
  Supervisor s;
  Inputs i;
  i.hardwareReady = i.safety = i.limitsKnown = i.encoderHealthy = i.communicationHealthy =
      i.storageHealthy = i.stopped = i.key = i.hold = i.up = true;
  i.sampleMs = 0;
  s.tick(i, 0);
  i.sampleMs = 400;
  s.tick(i, 400);
  CHECK(!s.runRequested());
  CommandArbiter arbiter;
  arbiter.submit({false, 1, Source::Web});
  arbiter.submit({false, 4, Source::Rf});
  CHECK(arbiter.take().floorMask == 5);
  arbiter.submit({false, 2});
  arbiter.submit({true, 0});
  CHECK(arbiter.take().stop);
  DriveScheduler scheduler;
  Rig r;
  r.move();
  scheduler.tick(r.s, 500);
  CHECK(!scheduler.protocol.transmit(500).empty());
  r.s.fail("test");
  scheduler.tick(r.s, 510);
  CHECK(scheduler.protocol.transmit(510) == "(3)84");
}
void requestTests() {
  uint32_t n;
  CHECK(!parseUnsigned("258", 3, n));
  CHECK(!parseUnsigned("2junk", 3, n));
  CHECK(!parseUnsigned("-1", 3, n));
  CHECK(!parseUnsigned("42949672960", 0xffffffff, n));
  CHECK(parseUnsigned("3", 3, n) && n == 3);
  HttpRequest h;
  HttpRequest::Result result{};
  for (char c : std::string("POST /api/move HTTP/1.1\r\nContent-Length: 7\r\nContent-Type: "
                            "application/x-www-form-urlencoded\r\n\r\nfloor=2"))
    result = h.feed(c);
  CHECK(result == HttpRequest::Result::Complete);
  CHECK(h.body == "floor=2");
  std::map<std::string, std::string> args;
  CHECK(!HttpRequest::form("floor=2&floor=3", args));
  args.clear();
  CHECK(!HttpRequest::form("floor=%00", args));
  HttpRequest huge;
  for (char c : std::string("POST /api/move HTTP/1.1\r\nContent-Length: 999999\r\n\r\n"))
    result = huge.feed(c);
  CHECK(result == HttpRequest::Result::TooLarge);
  Rf rf;
  rf.capture(1, 0);
  CHECK(rf.associate(1));
  CHECK(!rf.buttons(1, 1, true).floorMask);
  rf.buttons(0, 2, true);
  CHECK(rf.buttons(1, 3, true).floorMask == 1);
  CHECK(!rf.buttons(1, 4, true).floorMask);
  rf.invalidateAssociations();
  rf.capture(1, 5);
  rf.buttons(0, 6, true);
  CHECK(!rf.buttons(1, 7, true).floorMask);
  rf.buttons(0, 8, true);
  CHECK(rf.buttons(8, 9, true).stop);
  CHECK(rf.beginLearn(true, 100));
  rf.tickLearn(false, true, 160);
  CHECK(!rf.learnOutput);
  rf.tickLearn(false, false, 170);
  CHECK(rf.learning == Rf::LearnState::Unconfirmed);
  Rf erase;
  CHECK(!erase.beginErase(true, false, 0));
  CHECK(erase.beginErase(true, true, 0));
  erase.tickErase(true, true, 100);
  erase.tickErase(true, true, 9900);
  CHECK(erase.learnOutput);
  erase.tickErase(false, true, 10000);
  CHECK(!erase.learnOutput);
  CHECK(erase.erasing == Rf::EraseState::AwaitHigh);
  erase.tickErase(true, true, 10010);
  erase.tickErase(false, true, 12010);
  CHECK(erase.erasing == Rf::EraseState::Confirmed);
  Rf disconnected;
  CHECK(disconnected.beginErase(true, true, 0));
  disconnected.tickErase(false, true, 10501);
  CHECK(disconnected.erasing == Rf::EraseState::Unconfirmed);
  CHECK(!disconnected.learnOutput);
}
void sessionTests() {
  Memory memory;
  Journal configRecords(memory, 2048, 2), ledgerRecords(memory, 4096, 2);
  CHECK(configRecords.recover());
  CHECK(ledgerRecords.recover());
  ConfigurationStore store(configRecords);
  SafetyLedger ledger(ledgerRecords);
  ControllerSession session(store, ledger);
  Rig rig;
  session.configuration.motion = rig.s.config;
  session.supervisor = rig.s;
  session.supervisor.positionValid = true;
  session.supervisor.state = State::Idle;
  CHECK(store.save(session.configuration));
  session.submit({false, 1, Source::Web});
  session.submit({false, 4, Source::Rf});
  rig.i.sampleMs = 410;
  session.tick(rig.i, 410);
  CHECK(std::string(session.lastResult) == "conflicting_or_invalid_floor");
  CHECK(!session.supervisor.runRequested());
  CHECK(!session.request({false, 2}, 420));
  CHECK(ledger.unfinishedMotion);
  SafetyLedger resetLedger(ledgerRecords);
  CHECK(resetLedger.load());
  CHECK(resetLedger.unfinishedMotion);
  CHECK(!session.mayReboot());
  rig.i.position = 19000;
  rig.i.stopped = false;
  rig.i.sampleMs = 500;
  session.tick(rig.i, 500);
  CHECK(session.supervisor.state == State::Stopping);
  rig.i.position = 20000;
  rig.i.stopped = true;
  rig.i.sampleMs = 600;
  session.tick(rig.i, 600);
  rig.i.sampleMs = 910;
  session.tick(rig.i, 910);
  CHECK(session.supervisor.state == State::Idle);
  CHECK(!ledger.unfinishedMotion);
  rig.i.key = true;
  rig.i.sampleMs = 920;
  session.tick(rig.i, 920);
  CHECK(session.mayReboot());
  auto updated = session.configuration;
  updated.motion.speed = 500;
  CHECK(!session.configure(updated));
  CHECK(!session.supervisor.config.calibration.valid);
  CHECK(session.supervisor.positionValid);
  CHECK(!session.beginProgramming());
  CHECK(session.request({false, 1}, 930));
  CHECK(!session.mayReboot());
  rig.i.hold = true;
  rig.i.sampleMs = 940;
  session.tick(rig.i, 940);
  CHECK(!session.leaveProgramming(950));
  CHECK(session.supervisor.state == State::Calibrating);
  CHECK(ledger.unfinishedMotion);
  CHECK(session.request({true, 0}, 960) == nullptr);
  rig.i.hold = false;
  rig.i.sampleMs = 1000;
  session.tick(rig.i, 1000);
  rig.i.sampleMs = 1300;
  session.tick(rig.i, 1300);
  CHECK(!session.supervisor.config.calibration.valid);
  session.supervisor.fail("simulated_fault");
  rig.i.sampleMs = 1310;
  session.tick(rig.i, 1310);
  CHECK(ledger.faultLatched);
  ControllerSession rebooted(store, resetLedger);
  CHECK(rebooted.load());
  CHECK(rebooted.supervisor.state == State::Fault);
  CHECK(!rebooted.supervisor.positionValid);
  CHECK(!session.resetFault());
  CHECK(!ledger.faultLatched);
  CHECK(!session.supervisor.positionValid);
  session.supervisor.positionValid = true;
  session.supervisor.state = State::Idle;
  rig.i.key = false;
  rig.i.sampleMs = 1320;
  session.tick(rig.i, 1320);
  session.supervisor.config.calibration = {true, 1000, session.supervisor.config.revision, 1, 0,
                                           0,    0};
  memory.budget = 0;
  CHECK(session.request({false, 1}, 1330));
  CHECK(!session.supervisor.runRequested());
  CHECK(session.supervisor.state == State::Fault);
}
void parameterJobTests() {
  Memory memory;
  Journal configRecords(memory, 2048, 2), ledgerRecords(memory, 4096, 2), cache(memory, 6144, 2);
  CHECK(configRecords.recover());
  CHECK(ledgerRecords.recover());
  CHECK(cache.recover());
  ConfigurationStore store(configRecords);
  SafetyLedger ledger(ledgerRecords);
  ControllerSession session(store, ledger);
  Rig rig;
  rig.known();
  rig.i.key = true;
  rig.tick(410);
  session.supervisor = rig.s;
  session.configuration.motion = rig.s.config;
  Em01 protocol;
  ParameterJobs jobs(protocol, session, cache);
  CHECK(jobs.start(true, 266, 10, ParameterAccess::Advanced, 420));
  CHECK(jobs.start(true, 13, 1, ParameterAccess::Advanced, 420));
  CHECK(jobs.start(true, 12, 0, ParameterAccess::Advanced, 420));
  CHECK(jobs.start(true, 4, 10, ParameterAccess::User, 420));
  CHECK(!jobs.start(true, 4, 20, ParameterAccess::Installer, 420));
  CHECK(!session.supervisor.config.calibration.valid);
  CHECK(protocol.transmit(420) == Em01::frame("(4040020)"));
  for (char c : Em01::frame("(4)"))
    protocol.receive(c, 430);
  CHECK(protocol.transmit(431) == Em01::frame("(504)"));
  for (char c : Em01::frame("(50020)"))
    protocol.receive(c, 440);
  jobs.tick(441);
  CHECK(std::string(jobs.result) == "verified");
  CHECK(session.supervisor.config.decel == 20);
  CHECK(jobs.values[4] == 20);
  ParameterJobs loaded(protocol, session, cache);
  CHECK(loaded.loadCache());
  CHECK(loaded.values[4] == 20);
  CHECK(loaded.sampledMs[4] == 0);
  CHECK(!jobs.start(false, 4, 0, ParameterAccess::Installer, 600));
  protocol.transmit(600);
  for (char c : Em01::frame("(504)"))
    protocol.receive(c, 601);
  CHECK(protocol.busy());  // Request echo is not a response.
  for (char c : Em01::frame("(50020)"))
    protocol.receive(c, 602);
  jobs.tick(603);
  CHECK(!jobs.active);
  CHECK(!jobs.start(true, 4, 30, ParameterAccess::Installer, 800));
  protocol.transmit(800);
  for (char c : Em01::frame("(4)"))
    protocol.receive(c, 801);
  protocol.transmit(802);
  for (char c : Em01::frame("(50020)"))
    protocol.receive(c, 803);
  jobs.tick(804);
  CHECK(session.supervisor.state == State::Fault);
  CHECK(std::string(jobs.result) == "transaction_failed");
}
int main() {
  try {
    motionTests();
    protocolTests();
    storageTests();
    requestTests();
    deviceTests();
    sessionTests();
    parameterJobTests();
    std::cout << "PASS " << checks << " assertions (host simulation only)\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
