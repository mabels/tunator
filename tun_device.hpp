#ifndef __TunDevice__
#define __TunDevice__

#include "packet_queue.hpp"
#include "if_addrs.hpp"

#include <fcntl.h>
#include <string>
#include <thread>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>


extern int tun_alloc(std::string &dev);


class TunDevice {
private:
  size_t seq;
  IfAddrs ifAddrs;
  PacketQueue fromTun;
  PacketQueue toTun;
  volatile bool running;
  int tunFd;
  std::string tunDevName;
  std::unique_ptr<std::thread> fromTunThread;
  std::unique_ptr<std::thread> toTunThread;
  std::unique_ptr<std::thread> recvThread;
  std::unique_ptr<std::thread> statThread;

  int tunDevError(const char *str) {
    LOG(ERROR) << str;
    running = false;
    return 0;
  }

  bool startOnTun() {
    LOG(INFO) << "running tun mode";
    const int fd = tun_alloc(tunDevName);
    if (fd < 0) {
      LOG(ERROR) << "tun_alloc failed with:" << errno;
      return false;
    }
    tunFd = fd;
    for (auto &cmd : ifAddrs.asCommands(tunDevName)) {
      cmd.run();
    }
    auto flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      LOG(ERROR) << "tun_alloc:fcntl failed with:" << errno;
      return false;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
      LOG(ERROR) << "tun_alloc:fcntl failed with:" << errno;
      return false;
    }
    fromTunThread = std::unique_ptr<std::thread>(new std::thread([this]() {
      while(running) {
//        LOG(INFO) << "fromTunThread=" << running;
        fromTun.push([this](Packet *pkt) {
          bool again;
          int ret;
          do {
            again = false;
            ret = read(tunFd, pkt->buf, pkt->maxSize);
            if (ret == 0) {
              return tunDevError("tunDev closed");
            }
            if (ret == -1) {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct timeval timeout = fromTun.getBlockTimeval();
                fd_set rdSet;
                FD_ZERO(&rdSet);
                FD_SET(tunFd, &rdSet);
                auto ret = select(tunFd+1, &rdSet, 0, 0, &timeout);
                if (ret < 0) {
                  return tunDevError("tunDev read error:");
                }
                if (ret == 0) {
                  return 0; // timeout
                }
                again = FD_ISSET(tunFd, &rdSet);
              } else {
                return tunDevError("tunDev read error:");
              }
            }
          } while (again);
          return ret;
        });
      }
      LOG(INFO) << "stopped tun mode";
    }));
    toTunThread = std::unique_ptr<std::thread>(new std::thread([this]() {
      struct timeval tv = toTun.getBlockTimeval();
      setsockopt(tunFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
      while(running) {
        toTun.pop([this](Packet *pkt) {
            return write(tunFd, pkt->buf, pkt->size);
        });
      }
    }));
    return true;
  }

  bool startEcho() {
    LOG(INFO) << "running echo mode";
    tunDevName = "echo";
    fromTunThread = std::unique_ptr<std::thread>(new std::thread([this](){
      // while(running) {
      //   // nothing to do!!
      //   std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // }
    }));
    toTunThread = std::unique_ptr<std::thread>(new std::thread([this]() {
      while(running) {
        toTun.pop([this](Packet *inPkt) {
          auto ret = fromTun.push([this, inPkt](Packet *outPkt) {
            memcpy(outPkt->buf, inPkt->buf, inPkt->size);
            return inPkt->size;
          });
          return ret ? 1 : -1;
        });
      }
      LOG(INFO) << "stopped echo mode";
    }));
    return true;
  }

public:
  TunDevice() : seq(4711), fromTun(100, 5, 100),
    toTun(100, 5, 100), running(false), tunFd(-1), tunDevName("defaultInit") {
    //LOG(INFO) << "TunDevice:DEF:" << this;
  }
  TunDevice(const IfAddrs &ifAddrs, size_t mtu, size_t qSize) :
    seq(0),
    ifAddrs(ifAddrs),
    fromTun(mtu, qSize, 100),
    toTun(mtu, qSize, 100),
    running(false),
    tunFd(-1),
    tunDevName("") {
    LOG(INFO) << "TunDevice:REL:" << this;
  }
  ~TunDevice() {
    LOG(INFO) << "~TunDevice:" << this;
  }

  static void fromJson(Json::Value &json, TunDevice &tun) {
    tun.seq = json.get("seq", (Json::UInt64)tun.seq).asUInt64();
    tun.running = json.get("running", tun.running).asBool();
    tun.tunDevName = json.get("tunDevName", tun.tunDevName).asString();
    IfAddrs::fromJson(json["ifAddrs"], tun.ifAddrs);
  }

  void asJson(Json::Value &val) {
    val["type"] = "TunDevice";
    val["seq"] = Json::Value((Json::UInt64)++seq);
    std::stringstream s2;
    auto facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S%F%Q");
    s2.imbue(std::locale(s2.getloc(), facet));
    s2 << boost::posix_time::second_clock::local_time();
    //boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    val["timeHere"] = s2.str();
    val["running"] = running;
    val["tunDevName"] = tunDevName;
    val["ifAddrs"] = Json::Value();
    ifAddrs.asJson(val["ifAddrs"]);
    val["fromTun"] = Json::Value();
    fromTun.getStatistic().collect().asJson(val["fromTun"]);
    val["toTun"] = Json::Value();
    toTun.getStatistic().collect().asJson(val["toTun"]);
  }

  size_t getSeq() const {
    return seq;
  }

  bool getRunning() const {
    return running;
  }

  const std::string& getTunDevName() const {
    return tunDevName;
  }
  PacketQueue &getToTun() {
    return toTun;
  }
  PacketQueue &getFromTun() {
    return fromTun;
  }

  void setRecvThread(std::thread *thread) {
    recvThread = std::unique_ptr<std::thread>(thread);
  }
  void setStatThread(std::thread *thread) {
    statThread = std::unique_ptr<std::thread>(thread);
  }

  bool start() {
    if (running) {
      LOG(ERROR) << "could not start a running tun device";
      return false;
    }
    if (ifAddrs.getMtu() > fromTun.getPacketBuffer().getMtu()) {
      LOG(ERROR) << "ifAddrs Mtu to big for fromTun:" << ifAddrs.getMtu() << ":" << fromTun.getPacketBuffer().getMtu();
      return false;
    }
    if (ifAddrs.getMtu() > toTun.getPacketBuffer().getMtu()) {
      LOG(ERROR) << "ifAddrs Mtu to big for toTun:" << ifAddrs.getMtu() << ":" << toTun.getPacketBuffer().getMtu();
      return false;
    }
    running = true; // restartable
    if (ifAddrs.isEcho()) {
        return startEcho();
    }
    return startOnTun();
  }

  void stop() {
    // LOG(INFO) << "stop-started:+:" << this;
    running = false;
    join();
    if (tunFd >= 0) {
      close(tunFd);
    }
    // LOG(INFO) << "stop-started:-:" << this;
  }
  void join() {
    //LOG(INFO) << "join-fromTunThread:" << this;
    if (fromTunThread.get()) {
      LOG(INFO) << "join-fromTunThread:" << this;
      //LOG(INFO) << fromTunThread->joinable();
      fromTunThread->join();
    }
    if (toTunThread.get()) {
      LOG(INFO) << "join-toTunThread:" << this;
      toTunThread->join();
    }
    if (recvThread.get()) {
      LOG(INFO) << "join-recvThread:" << this;
      recvThread->join();
    }
    if (statThread.get()) {
      LOG(INFO) << "join-statThread:" << this;
      statThread->join();
    }
    LOG(INFO) << "join done:" << this;
  }

};

#endif
