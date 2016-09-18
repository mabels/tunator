
#include <vector>

#include "system_cmd.hpp"
#include "if_addrs.hpp"

#if defined(__APPLE_CC__) || defined(TEST)
std::vector<SystemCmd> IfAddrs::asCommands(const std::string &dev) const {
 // LOG(INFO) << "asCommands:1";
  std::vector<SystemCmd> ret;
  for (auto &dst : getDests()) {
    //LOG(INFO) << "asCommands:1a";
    // missing ipv4 ipv6 sorting
    for (auto &addr : addrs) {
      if (!dst.ip_same_kind(addr)) {
        continue;
      }
      if (addr.includes(dst)) {
        //LOG(INFO) << "asCommands:2";
        ret.push_back(SystemCmd("/sbin/ifconfig").arg(dev).arg(addr.to_s()).arg(dst.to_s()));
      } else {
        ret.push_back(SystemCmd("/sbin/ifconfig").arg(dev).arg("alias").arg(addr.to_string()));
      }
    }
  }
  //LOG(INFO) << "asCommands:3";
  for (auto &route : routes) {
    //LOG(INFO) << "asCommands:4";
    ret.push_back(SystemCmd("/sbin/route").arg("add")
      .arg(route.dest.to_string()).arg(route.via.to_s())
      .arg("-ifscope").arg(dev));
  }
  //LOG(INFO) << "asCommands:5";
  ret.push_back(SystemCmd("/sbin/ifconfig").arg(dev)
      .arg("mtu").arg(mtu).arg("up"));
  //LOG(INFO) << "asCommands:6";
  return ret;
}

#ifdef TEST
const char *IfAddrsRef[] = {
      "/sbin/ifconfig DEV 10.1.0.1 10.1.0.254",
      "/sbin/ifconfig DEV alias 10.2.0.1/24",
      "/sbin/ifconfig DEV fd00::1000 fd00::1",
      "/sbin/route add 172.16.0.1/24 172.16.0.254 -ifscope DEV",
      "/sbin/route add 172.17.0.1/24 172.17.0.254 -ifscope DEV",
      "/sbin/ifconfig DEV mtu 1360 up",
      0
};
#endif

#endif
