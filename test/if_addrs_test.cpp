#include "../if_addrs.hpp"

#include <iostream>
#include <map>

using std::cerr;
using std::endl;

INITIALIZE_EASYLOGGINGPP
int main() {
  std::map<std::string, bool> testIsValidWithPrefix = {
    {"", false},
    {"a.b.c.d", false },
    {"300.200.200.200", false },
    {"200.200.200.200", true },
    {"200::200:200:200", true },
    {"/", false },
    {"/17", false },
    {"/a7", false },
    {"200.200.200.200/", true },
    {"200.200.200.200/a7", false },
    {"200.200.200.200/7", true },
    {"200.200.200.200/-1", false },
    {"200.200.200.200/33", false },
    {"zoo::200/", false },
    {"200:200:200::200/a7", false },
    {"200:200:200::200/77", true },
    {"200:200:200::200/-1", false },
    {"200:200:200::200/129", false }
  };
  for (const auto &kv : testIsValidWithPrefix) {
    if (kv.second != IfAddrs::isValidWithPrefix(kv.first)) {
      cerr << "IfAddrs::isValidWithPrefix failed on:" << kv.first << ":"
           << kv.second << endl;
      return 1;
    }
  }
  std::map<std::string, bool> testIsValidWithoutPrefix = {
    {"", false },
    {"/", false },
    {"/a7", false },
    {"/17", false },
    {"1.2.3.4/17", false },
    {"1:2:3::4/17", false },
    {"300.2.3.4", false },
    {"1:zoo:3::4", false },
    {"1.2.3.4", true },
    {"1:2:3::4", true }
  };
  for (const auto &kv : testIsValidWithoutPrefix) {
    if (kv.second != IfAddrs::isValidWithoutPrefix(kv.first)) {
      cerr << "IfAddrs::isValidWithPrefix failed on:" << kv.first << ":"
           << kv.second << endl;
      return 1;
    }
  }

  IfAddrs ia;
  ia.addAddr("10.1.0.1/24");
  if (!ia.addAddr("10.2.0.1/24")) {
    cerr << "ia.addAddr failed" << endl;
    return 1;
  }
  if (ia.addAddr("256.2.0.1/24")) {
    cerr << "ia.addAddr failed" << endl;
    return 1;
  }
  ia.addRoute(IfAddrs::RouteVia("172.16.0.1/24", "172.16.0.254"));
  if (!ia.addRoute(IfAddrs::RouteVia("172.17.0.1/24", "172.17.0.254"))) {
    cerr << "ia.addRoute failed" << endl;
    return 1;
  }
  if (ia.addRoute(IfAddrs::RouteVia("300.17.0.1/24", "172.17.0.254"))) {
    cerr << "ia.addRoute failed" << endl;
    return 1;
  }
  if (ia.addRoute(IfAddrs::RouteVia("300.17.0.1/24", "172.17.0.254/23"))) {
    cerr << "ia.addRoute failed" << endl;
    return 1;
  }
  if (ia.addRoute(IfAddrs::RouteVia("300.17.0.1/24", "172.17.0.354"))) {
    cerr << "ia.addRoute failed" << endl;
    return 1;
  }
  auto ret = ia.asCommands("DEV");
  auto ref = std::string("ip addr add 10.1.0.1/24 dev DEV\n"
                         "ip addr add 10.2.0.1/24 dev DEV\n"
                         "ip route add 172.16.0.1/24 via 172.16.0.254 dev DEV\n"
                         "ip route add 172.17.0.1/24 via 172.17.0.254 dev DEV\n"
                         "ip link set dev DEV up\n");
  if (ret != ref) {
    cerr << "wrong string " << endl;
    cerr << ret.length() << "[" << ret << "]" << endl;
    cerr << ref.length() << "[" << ref << "]" << endl;
    return 1;
  }
  return 0;
}
