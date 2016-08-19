

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <condition_variable>
#include <chrono>

#include "packet_buffer.hpp"
#include "packet_statistic.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>


class PacketQueue {
private:
  PacketBuffer pb;
  PacketStatistic ps;
  boost::lockfree::detail::runtime_sized_ringbuffer<Packet *, std::allocator<Packet *>> q;
  const size_t blockWait;

  mutable std::mutex mutex;
  mutable std::condition_variable cond;

public:
  typedef std::function<ssize_t(Packet *)> PacketAction;

  explicit PacketQueue(size_t mtu, size_t size, size_t blockWait)
      : pb(mtu, size), q(size), blockWait(blockWait) {
	//LOG(INFO) << "PacketQueue:" << mtu << ":" << size << ":" << blockWait;
  }

  struct timeval getBlockTimeval() const {
    struct timeval timeout = {
      (long int)blockWait/1000,
      1000*((int)blockWait%1000) };
    return timeout;
  }

  const PacketBuffer &getPacketBuffer() const { return pb; }

  PacketStatistic getStatistic() { return ps.collect(); }

  bool push(const PacketAction &action) {
    for (size_t wait = 0, tw = 0; tw < blockWait; wait += 10) {
      auto packet = pb.alloc();
      if (packet) {
        ps.incAllocOk();
        const auto ret = action(packet);
        if (ret > 0) {
          ps.addPushPacketSize(ret);
          packet->size = ret;
          if (q.push(packet)) {
            cond.notify_one();
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
      //LOG(INFO) << "Qpush:wait";
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
    ps.incPushTimeout();
    return false;
  }

  bool pop(const PacketAction &action) {
    do {
      Packet *pkt;
      if (q.pop(&pkt, 1)) {
        // std::cerr << (size_t)pkt << std::endl;
        ps.addPopPacketSize(pkt->size);
        if (action(pkt) >= 0) {
          ps.incPopOk();
        } else {
          ps.incPopActionFailed();
        }
        pkt->free();
        return true;
      } else {
        ps.incPopEmpty();
	//LOG(INFO) << "pop-enter";
        std::unique_lock<std::mutex> lk(mutex);
        if (cond.wait_for(lk, std::chrono::milliseconds(blockWait)) == std::cv_status::timeout) {
          ps.incPopTimeout();
	  //LOG(INFO) << "pop-leave-1";
          return false;
        }
  	//LOG(INFO) << "pop-leave-2";
      }
    } while (true);
  }
};
