

#include <algorithm>
#include "packet_buffer.hpp"

#include <boost/lockfree/spsc_queue.hpp>

class PacketQueue {
private:
  PacketBuffer pb;
  boost::lockfree::detail::runtime_sized_ringbuffer<Packet*> q;
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
public:
  typedef std::function<ssize_t(Packet *)> PacketAction;

  PacketQueue(int mtu, int size) : pb(mtu, size), q(size) {
  }

  void push(PacketAction action) {
    for(int wait = 0; true; wait += 10) {
        auto packet = pb.alloc();
        if (packet) {
          ++allocOk;
          const auto ret = action(packet);
          if (ret > 0) {
            pushPacketSize += ret;
            packet->size = ret;
            if (q.push(packet)) {
              ++pushOk;
            } else {
              packet->free();
              ++pushFailed;
            }
          } else {
            packet->free();
            ++pushActionFailed;
          }
          wait = 0;
        } else {
            ++allocFailed;
        }
        wait = std::min(wait, 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
  }

  void pop(PacketAction action) {
    size_t totalWait = 0;
    for(int wait = 0; totalWait >= 1000; wait += 10) {
      Packet *pkt;
      if (q.pop(pkt)) {
        popPacketSize += pkt->size;
        if (action(pkt) >= 0) {
          ++popOk;
        } else {
          ++popActionFailed;
        }
        pkt.free();
        wait = 0;
        return;
      } else {
        ++popEmpty;
      }
      wait = std::min(wait, 100);
      totalWait += wait;
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
  }

};
