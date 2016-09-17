
#include <vector>

#include "system_cmd.hpp"
#include "if_addrs.hpp"

#if defined(__APPLE_CC__) || defined(TEST)
std::vector<SystemCmd> IfAddrs::asCommands(const std::string &dev) const {
  LOG(INFO) << "asCommands:1";
  std::vector<SystemCmd> ret;
  for (auto &addr : addrs) {
    LOG(INFO) << "asCommands:1a";
    // missing ipv4 ipv6 sorting
    for (auto &dst : getDests()) {
      if (dst.ip_same_kind(addr)) {
        LOG(INFO) << "asCommands:2";
        ret.push_back(SystemCmd("/sbin/ifconfig").arg(dev).arg(addr.to_string()).arg(dst.to_s()));
      }
    }
  }
  LOG(INFO) << "asCommands:3";
  for (auto &route : routes) {
    LOG(INFO) << "asCommands:4";
    ret.push_back(SystemCmd("/sbin/route").arg("add")
      .arg("-ifscope").arg(dev)
      .arg(route.dest.to_string()).arg(route.via.to_s())
      .arg("dev").arg(dev));
  }
  LOG(INFO) << "asCommands:5";
  ret.push_back(SystemCmd("/sbin/ifconfig").arg(dev)
      .arg("mtu").arg(mtu).arg("up"));
  LOG(INFO) << "asCommands:6";
  return ret;
}

#ifdef TEST
const char *IfAddrsRef[] = {
      "/sbin/ifconfig DEV 10.1.0.1/24 10.2.0.1/24",
      "/sbin/ifconfig DEV 10.1.0.1/24 fd00::1/112",
      "/sbin/ifconfig DEV 10.2.0.1/24 10.2.0.1/24",
      "/sbin/ifconfig DEV 10.2.0.1/24 fd00::1/112",
      "/sbin/route add -ifscope DEV 172.16.0.1/24 172.16.0.254 dev DEV",
      "/sbin/route add -ifscope DEV 172.17.0.1/24 172.17.0.254 dev DEV",
      "/sbin/ifconfig DEV mtu 1360 up",
      0
};
#endif

#endif
