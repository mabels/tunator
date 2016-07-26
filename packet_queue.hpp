

#include <algorithm>
#include <memory>
#include <functional>
#include <thread>
#include <iostream>

#include "packet_buffer.hpp"
#include "packet_statistic.hpp"

#include <boost/lockfree/spsc_queue.hpp>

class PacketQueue {
private:
  PacketBuffer pb;
  PacketStatistic ps;
  boost::lockfree::detail::runtime_sized_ringbuffer<Packet*, std::allocator<Packet*>> q;
  const size_t blockWait;
public:
  typedef std::function<ssize_t(Packet *)> PacketAction;

  explicit PacketQueue(size_t mtu, size_t size, size_t blockWait) :
    pb(mtu, size),
    q(size),
    blockWait(blockWait) {
  }

  const PacketBuffer& getPacketBuffer() const {
    return pb;
  }

  PacketStatistic getStatistic() {
    return ps.collect();
  }

  bool push(PacketAction action) {
    for(size_t wait = 0, tw = 0; tw < blockWait; wait += 10) {
        auto packet = pb.alloc();
        if (packet) {
          ps.incAllocOk();
          const auto ret = action(packet);
          if (ret > 0) {
            ps.addPushPacketSize(ret);
            packet->size = ret;
            if (q.push(packet)) {
              ps.incPushOk();
              return true;
            } else {
              packet->free();
              ps.incPushFailed();
              return false;
            }
          } else {
            packet->free();
            ps.incPushActionFailed();
            return false;
          }
        } else {
            ps.incAllocFailed();
        }
        wait = std::min(wait, 100ul);
        tw += wait;
        std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
    ps.incPushTimeout();
    return false;
  }

  bool pop(PacketAction action) {
    for(size_t wait = 0, totalWait = 0; totalWait < blockWait; wait += 10) {
      Packet *pkt;
      if (q.pop(&pkt, 1)) {
        //std::cerr << (size_t)pkt << std::endl;
        ps.addPopPacketSize(pkt->size);
        if (action(pkt) >= 0) {
          ps.incPopOk();
        } else {
          ps.incPopActionFailed();
        }
        pkt->free();
        wait = 0;
        return true;
      } else {
        //std::cerr << "--POP" << std::endl;
        ps.incPopEmpty();
      }
      wait = std::min(wait, 100ul);
      totalWait += wait;
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
    ps.incPopTimeout();
    return false;
  }

};
