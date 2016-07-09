#include "../if_addrs.hpp"

#include <iostream>

using std::cerr;
using std::endl;

INITIALIZE_EASYLOGGINGPP
int main() {
  IfAddrs ia;
  ia.addAddr("10.1.0.1/24");
  ia.addAddr("10.2.0.1/24");
  ia.addRoute(IfAddrs::RouteVia("172.16.0.1/24", "172.16.0.254"));
  ia.addRoute(IfAddrs::RouteVia("172.17.0.1/24", "172.17.0.254"));
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
