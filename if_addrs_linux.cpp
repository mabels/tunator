
#include <vector>

#include "if_addrs.hpp"
#include "system_cmd.hpp"

#if !defined(__APPLE_CC__) || defined(TEST)
std::vector<SystemCmd> IfAddrs::asCommands(const std::string &dev) const {
  std::vector<SystemCmd> ret;
  for (auto &addr : addrs) {
    ret.push_back(SystemCmd("/sbin/ip").arg("addr").arg("add").arg(addr).arg("dev").arg(dev));
  }
  for (auto &route : routes) {
    ret.push_back(SystemCmd("/sbin/ip").arg("route").arg("add")
      .arg(route.dest).arg("via").arg(route.via)
      .arg("dev").arg(dev));
  }
  ret.push_back(SystemCmd("/sbin/ip").arg("link").arg("set")
    .arg("dev").arg(dev).arg("mtu").arg(mtu).arg("up"));
  return ret;
}

#ifdef TEST
const char *IfAddrsRef[] = {
             "/sbin/ip addr add 10.1.0.1/24 dev DEV",
             "/sbin/ip addr add 10.2.0.1/24 dev DEV",
             "/sbin/ip route add 172.16.0.1/24 via 172.16.0.254 dev DEV",
             "/sbin/ip route add 172.17.0.1/24 via 172.17.0.254 dev DEV",
             "/sbin/ip link set dev DEV mtu 1360 up",
             0
};
#endif
#endif
