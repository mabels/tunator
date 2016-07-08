
#include <algorithm>
#include <chrono>
#include <sstream>

#include <stdio.h>
#include <string.h>

using std::chrono::system_clock;
class PacketStatistic {
  struct S_Data {
    size_t allocFailed;
    size_t allocOk;
    size_t pushActionFailed;
    size_t popActionFailed;
    size_t pushTimeout;
    size_t popTimeout;
    size_t pushPacketSize;
    size_t popPacketSize;
    size_t pushOk;
    size_t popOk;
    size_t popEmpty;
    size_t pushFailed;
  };
  system_clock::time_point started;
  struct S_Data current;
  struct S_Data total;
public:
  PacketStatistic() : started(system_clock::now()) {
    memset(&current, 0, sizeof(S_Data));
    memset(&total, 0, sizeof(S_Data));
  }
  void incAllocFailed() { ++current.allocFailed; }
  void incAllocOk() { ++current.allocOk; }
  void incPushActionFailed() { ++current.pushActionFailed; }
  void incPushTimeout() { ++current.pushTimeout; }
  void incPopTimeout() { ++current.popTimeout; }
  void incPopActionFailed() { ++current.popActionFailed; }
  void addPushPacketSize(size_t pbs) { current.pushPacketSize += pbs; }
  void addPopPacketSize(size_t pbs) { current.popPacketSize += pbs; }
  void incPushOk() { ++current.pushOk; }
  void incPopOk() { ++current.popOk; }
  void incPopEmpty() { ++current.popEmpty; }
  void incPushFailed() { ++current.pushFailed; }

  const struct S_Data& getCurrent() const { return current; }
  const struct S_Data& getTotal() const { return total; }
  system_clock::time_point getStarted() const { return started; }

  PacketStatistic collect() {
    // loosing a bit
    PacketStatistic ret;
    std::swap(ret.current, this->current);
    std::swap(ret.started, this->started);
    total.allocFailed += ret.current.allocFailed;
    total.allocOk += ret.current.allocOk;
    total.pushActionFailed += ret.current.pushActionFailed;
    total.popActionFailed += ret.current.popActionFailed;
    total.pushPacketSize += ret.current.pushPacketSize;
    total.pushTimeout += ret.current.pushTimeout;
    total.popTimeout += ret.current.popTimeout;
    total.popPacketSize += ret.current.popPacketSize;
    total.pushOk += ret.current.pushOk;
    total.popOk += ret.current.popOk;
    total.popEmpty += ret.current.popEmpty;
    total.pushFailed += ret.current.pushFailed;
    ret.total = total;
    return ret;
  }

  std::string asString() const {
      std::stringstream s2;
      s2 << "allocFailed total=" << total.allocFailed << ":current=" << current.allocFailed << std::endl;
      s2 << "allocOk total=" << total.allocOk << ":current=" << current.allocOk << std::endl;
      s2 << "pushActionFailed total=" << total.pushActionFailed << ":current=" << current.pushActionFailed << std::endl;
      s2 << "popActionFailed total=" << total.popActionFailed << ":current=" << current.popActionFailed << std::endl;
      s2 << "pushPacketSize total=" << total.pushPacketSize << ":current=" << current.pushPacketSize << std::endl;
      s2 << "pushTimeout total=" << total.pushTimeout << ":current=" << current.pushTimeout << std::endl;
      s2 << "popTimeout total=" << total.popTimeout << ":current=" << current.popTimeout << std::endl;
      s2 << "popPacketSize total=" << total.popPacketSize << ":current=" << current.popPacketSize << std::endl;
      s2 << "pushOk total=" << total.pushOk << ":current=" << current.pushOk << std::endl;
      s2 << "popOk total=" << total.popOk << ":current=" << current.popOk << std::endl;
      s2 << "popEmpty total=" << total.popEmpty << ":current=" << current.popEmpty << std::endl;
      s2 << "pushFailed total=" << total.pushFailed << ":current=" << current.pushFailed << std::endl;
      return s2.str();
  }
};
