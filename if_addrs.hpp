
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

class IfAddrs {
public:
  class RouteVia {
  public:
    const std::string dest;
    const std::string via;
    RouteVia(const std::string &dest, const std::string &via) : dest(dest), via(via) {
    }
  };
private:
  std::vector<std::string> addrs;
  std::vector<RouteVia> routes;
public:
  void addAddr(const std::string &addr) {
    addrs.push_back(addr);
  }
  void addRoute(const RouteVia &route) {
    routes.push_back(route);
  }
  std::string asCommands(const std::string &dev) const {
    std::stringstream s2;
    for (auto &addr : addrs) {
        s2 << "ip addr add " << addr << " dev " << dev << std::endl;
    }
    for (auto &route : routes) {
        s2 << "ip route add " << route.dest << " via " << route.via << " dev "<< dev << std::endl;
    }
    s2 << "ip link set dev << " << dev << " up" << std::endl;
    return s2.str();
  }
};
