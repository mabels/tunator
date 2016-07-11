
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>

#include <arpa/inet.h>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

class IfAddrs {
public:
  class RouteVia {
  public:
    const std::string dest;
    const std::string via;
    RouteVia(const std::string &dest, const std::string &via) : dest(dest), via(via) {
    }
    bool isValid() const {
      return IfAddrs::isValidWithPrefix(dest) && IfAddrs::isValidWithoutPrefix(via);
    }
  };
private:
  std::vector<std::string> addrs;
  std::vector<RouteVia> routes;

  static std::pair<std::string, std::string> splitPrefix(const std::string &addr) {
    auto slash = addr.find("/");
    if (slash == std::string::npos) {
      return std::pair<std::string, std::string>(addr, "");
    }
    return std::pair<std::string, std::string>(addr.substr(0, slash), addr.substr(slash + 1));
  }
  static bool isPrefixValid(int af, const std::string &sPrefix) {
    if (sPrefix.empty()) {
      return true;
    }
    try {
      auto prefix = std::stoi(sPrefix);
      if (af == AF_INET && 0 <= prefix && prefix <= 32) {
        return true;
      }
      if (af == AF_INET6 && 0 <= prefix && prefix <= 128) {
        return true;
      }
      return false;
    } catch (std::exception& ex) {
      return false;
    }
  }
public:
  static bool isValidWithoutPrefix(const std::string &addr) {
    auto slash = addr.find("/");
    if (slash != std::string::npos) {
        return false;
    }
    return isValidWithPrefix(addr);
  }
  static bool isValidWithPrefix(const std::string &addr) {
      auto sp = splitPrefix(addr);
      struct in_addr ipv4_dst;
      if (inet_pton(AF_INET, sp.first.c_str(), &ipv4_dst) == 1) {
        return isPrefixValid(AF_INET, sp.second);
      }
      struct in6_addr ipv6_dst;
      if (inet_pton(AF_INET6, sp.first.c_str(), &ipv6_dst) == 1) {
        return isPrefixValid(AF_INET6, sp.second);
      }
      return false;
  }

  IfAddrs() : addrs(), routes() {
    // LOG(INFO) << addrs.size() << ":" << addrs.empty();
    // LOG(INFO) << asCommands("isEcho");
  }
  bool isEcho() const {
    // LOG(INFO) << addrs.size() << ":" << addrs.empty();
    // LOG(INFO) << asCommands("isEcho");
    return addrs.empty();
  }
  bool addAddr(const std::string &addr) {
    if (!IfAddrs::isValidWithPrefix(addr)) {
      return false;
    }
    addrs.push_back(addr);
    return true;
  }
  bool addRoute(const RouteVia &route) {
    if (!route.isValid()) {
      return false;
    }
    routes.push_back(route);
    return true;
  }
  std::string asCommands(const std::string &dev) const {
    std::stringstream s2;
    for (auto &addr : addrs) {
        s2 << "ip addr add " << addr << " dev " << dev << std::endl;
    }
    for (auto &route : routes) {
        s2 << "ip route add " << route.dest << " via " << route.via << " dev "<< dev << std::endl;
    }
    s2 << "ip link set dev " << dev << " up" << std::endl;
    return s2.str();
  }
};
