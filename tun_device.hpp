#include <string>


#include <thread>

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
  const PacketQueue fromTun;
  const PacketQueue toTun;
  const string tunDevName;
  int tunFd;
  bool running;
  std::unique_ptr<std::thread> fromTunThread;
  std::unique_ptr<std::thread> toTunThread;

  static void fromTunDelegate(TunDevice *td) {
    td->fromTunAction();
  }
  void fromTunAction() {
    while(running) {
      fromTun.push([this](Packet *pkt) {
          return read(tunFd, pkt->buf, pkt->max_size);
      });
    }
  }

  static void toTunDelegate(TunDevice *td) {
    td->toTunAction();
  }
  void toTunAction() {
    while(running) {
      toTun.pop([this](Packet *pkt) {
          return write(tunFd, pkt->buf, pkt->size);
      });
    }
  }

public:
  TunDevice(size_t mtu, size_t qSize) :
    running(true),
    fromTun(mtu, qSize),
    toTun(mtu, qSize),
    tunDevName("") {
  }

  bool start() {
    const int fd = tun_alloc(tunDevName);
    if (fd < 0) {
      LOG(ERROR) << "tun_alloc failed with:" << errno;
      return false;
    }
    tunFd = fd;
    fromTunThread = std::unique_ptr<std::thread>(new std::thread(fromTunAction, this));
    toTunThread = std::unique_ptr<std::thread>(new std::thread(toTunAction, this));
    return true;
  }
  void stop() {
      running = false;
  }
  bool join() {
    fromTunThread->join();
    tunTunThread->join();
  }

};
