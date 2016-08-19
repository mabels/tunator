

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <string>

#include <fcntl.h>

#ifdef __APPLE_CC__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <unistd.h>

int tun_alloc(std::string &dev) {
  int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
  if (fd == -1) {
    perror("socket(SYSPROTO_CONTROL)");
    return -1;
  }
  struct ctl_info ctlInfo;
  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
      sizeof(ctlInfo.ctl_name)) {
    fprintf(stderr, "UTUN_CONTROL_NAME too long");
    close(fd);
    return -1;
  }
  if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
    perror("ioctl(CTLIOCGINFO)");
    close(fd);
    return -1;
  }
  struct sockaddr_ctl sc;
  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = 2; /* Only have one, in this example... */
  // If the connect is successful, a tun%d device will be created, where "%d"
  // is our unit number -1
  if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
    perror("connect(AF_SYS_CONTROL)");
    close(fd);
    return -1;
  }
  std::stringstream s2;
  s2 << "utun" << sc.sc_unit - 1;
  dev = s2.str();
  return fd;
}

#ifdef UNUSED
int tun_alloc(std::string &dev) {
  int fd = 0;
  for (int i = 0; i < 255; ++i) {
    char buf[128];
    sprintf(buf, "/dev/tun%d", i);
    printf("try:%s\n", buf);
    if ((fd = open(buf, O_RDWR)) > 0) {
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
#endif
#else
#include <linux/if_tun.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int tun_alloc(std::string &dev) {
  int fd = open("/dev/net/tun", O_RDWR);
  if (fd < 0) {
    return -1;
  }
  for (int i = 0; i < 255; ++i) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    char buf[IFNAMSIZ];
    snprintf(buf, sizeof(buf), "tun%d", i);
    LOG(INFO) << "try:" << buf;
    strncpy(ifr.ifr_name, buf, IFNAMSIZ);
    int err = ioctl(fd, TUNSETIFF, &ifr);
    if (err >= 0) {
      dev = ifr.ifr_name;
      return fd;
    }
  }
  close(fd);
  return -1;
}
#endif
