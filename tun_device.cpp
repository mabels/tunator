

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <string>

#include <fcntl.h>

bool is_v4(void *bytes) {
  return (((unsigned char*)bytes)[0]&0xf0) == 0x40;
}
bool is_v6(void *bytes) {
  return (((unsigned char*)bytes)[0]&0xf0) == 0x60;
}

#ifdef __APPLE_CC__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define TUNDEV_HEADER_SIZE 4

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

void tun_header(char *header, void *bytes, size_t nbytes) {
  if (nbytes < 1) {
    return;
  }
  if (is_v4(bytes)) {
    header[3] = 2;
  } else if (is_v6(bytes)) {
    header[3] = 30;
  }
}
#else
#include <linux/if_tun.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TUNDEV_HEADER_SIZE 4

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

void tun_header(char *header, void *bytes, size_t nbytes) {
  if (nbytes < 1) {
    return;
  }
  if (is_v4(bytes)) {
    header[2] = 8;
  } else if (is_v6(bytes)) {
    header[2] = -122;
    header[3] = -35;
  }
}
#endif

int tun_write(int fd, void *bytes, size_t nbytes) {
  char header[TUNDEV_HEADER_SIZE] = { 0, 0, 0, 0};
  tun_header(header, bytes, nbytes);
  struct iovec iov[] = {{ header, sizeof(header) },
                        { bytes, nbytes }};
  int ret = writev(fd, iov, sizeof(iov)/sizeof(iovec));
  if (ret <= 0) {
    return ret;
  }
  return ret - sizeof(header);
}

int tun_read(int fd, void *bytes, size_t nbytes) {
  char header[TUNDEV_HEADER_SIZE];
  struct iovec iov[] = {{ header, sizeof(header) },
                        { bytes, nbytes }};
  int ret = readv(fd, iov, sizeof(iov)/sizeof(iovec));
  if (ret <= 0) {
    return ret;
  }
//    for (int i = 0; i< ret; i++)
//    {
//       printf ("%02x ", (int)((char*)iov[1].iov_base)[i]);
//       if ( (i-4)%16 ==15) printf("\n");
//    }
//    printf ("\n");

  // LOG(INFO) << "tun_read:" << ret << ":"
  //    << (int)header[0] << " " << (int)header[1] << " "
  //    << (int)header[2] << " " << (int)header[3] << ":"
  //    << (int)((char*)iov[1].iov_base)[0];
  return ret - sizeof(header);
}
