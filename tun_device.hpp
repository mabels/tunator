
#include "packet_queue.hpp"
#include "if_addrs.hpp"

#include <fcntl.h>
#include <string>
#include <thread>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#ifdef __APPLE_CC__
int tun_alloc(std::string &dev) {
  int fd = 0;
  for(int i = 0; i < 255; ++i) {
    char buf[128];
    sprintf(buf, "/dev/tun%d", i);
    printf("try:%s\n", buf);
    if( (fd = open(buf, O_RDWR)) > 0 ) {
      char *slash = strrchr(buf, '/');
      if (slash) {
        dev = slash + 1;
      } else {
        dev = buf;
      }
      return fd;
    }
  }
  return -1;
}
#else
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
int tun_alloc(std::string &dev) {
  int fd = open("/dev/net/tun", O_RDWR);
  if(fd < 0) {
   return -1;
  }
  for (int i  = 0; i < 255; ++i) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    char buf[128];
    sprintf(buf, "/dev/tun%d", i);
    printf("try:%s\n", buf);
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    int err = ioctl(fd, TUNSETIFF, &ifr);
    if (err >= 0) {
      dev =  ifr.ifr_name
      return fd;
    }
  }
  close(fd);
  return -1;
#endif


class TunDevice {
private:
  const IfAddrs &ifAddrs;
  PacketQueue fromTun;
  PacketQueue toTun;
  bool running;
  int tunFd;
  std::string tunDevName;
  std::unique_ptr<std::thread> fromTunThread;
  std::unique_ptr<std::thread> toTunThread;


  bool startOnTun() {
    const int fd = tun_alloc(tunDevName);
    if (fd < 0) {
      LOG(ERROR) << "tun_alloc failed with:" << errno;
      return false;
    }
    tunFd = fd;
    fromTunThread = std::unique_ptr<std::thread>(new std::thread([this]() {
      while(running) {
        fromTun.push([this](Packet *pkt) {
            return read(tunFd, pkt->buf, pkt->max_size);
        });
      }
    }));
    toTunThread = std::unique_ptr<std::thread>(new std::thread([this]() {
      while(running) {
        toTun.pop([this](Packet *pkt) {
            return write(tunFd, pkt->buf, pkt->size);
        });
      }
    }));
    return true;
  }

  bool startEcho() {
    fromTunThread = std::unique_ptr<std::thread>(new std::thread([this](){
      while(running) {
        // nothing to do!!
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
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
    }));
    return true;
  }


public:
  TunDevice(const IfAddrs &ifAddrs, size_t mtu, size_t qSize) :
    ifAddrs(ifAddrs),
    fromTun(mtu, qSize, 500),
    toTun(mtu, qSize, 500),
    running(false),
    tunDevName("") {
  }

  PacketQueue &getToTun() {
    return toTun;
  }
  PacketQueue &getFromTun() {
    return fromTun;
  }

  bool start() {
    if (running) {
      LOG(ERROR) << "could not start a running tun device";
      return false;
    }
    running = true; // restartable
    if (ifAddrs.isEcho()) {
        return startEcho();
    }
    return startOnTun();
  }

  void stop() {
      running = false;
  }
  void join() {
    fromTunThread->join();
    toTunThread->join();
  }

};
