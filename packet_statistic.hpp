
#include <chrono>

using std::chrono::system_clock;
class PacketStatistic {
  struct S_Data {
    size_t allocFailed;
    size_t allocOk;
    size_t pushActionFailed;
    size_t popActionFailed;
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
  }
  void incAllocFailed() { ++current.allocFailed; }
  void incAllocOk() { ++current.allocOk; }
  void incPushActionFailed() { ++current.pushActionFailed; }
  void incPopActionFailed() { ++current.popActionFailed; }
  void incPushPacketSize() { ++current.pushPacketSize; }
  void incPopPacketSize() { ++current.popPacketSize; }
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
    total.popPacketSize += ret.current.popPacketSize;
    total.pushOk += ret.current.pushOk;
    total.popOk += ret.current.popOk;
    total.popEmpty += ret.current.popEmpty;
    total.pushFailed += ret.current.pushFailed;
    ret.total = total;
    return ret;
  }
};
