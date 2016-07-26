



#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <string>

#include <fcntl.h>

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
#include <string.h>
#include <unistd.h>
int tun_alloc(std::string &dev) {
  int fd = open("/dev/net/tun", O_RDWR);
  if(fd < 0) {
   return -1;
  }
  for (int i  = 0; i < 255; ++i) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    char buf[IFNAMSIZ];
    snprintf(buf, sizeof(buf), "tun%d", i);
    LOG(INFO) << "try:" <<  buf;
    strncpy(ifr.ifr_name, buf, IFNAMSIZ);
    int err = ioctl(fd, TUNSETIFF, &ifr);
    if (err >= 0) {
      dev =  ifr.ifr_name;
      return fd;
    }
  }
  close(fd);
  return -1;
}
#endif
