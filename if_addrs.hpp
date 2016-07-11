
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>

#include <arpa/inet.h>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include <json/json.h>

class IfAddrs {
public:
  class RouteVia {
  public:
    std::string dest;
    std::string via;
    RouteVia() {
    }
    RouteVia(const std::string &dest, const std::string &via) : dest(dest), via(via) {
    }
    bool isValid() const {
      return IfAddrs::isValidWithPrefix(dest) && IfAddrs::isValidWithoutPrefix(via);
    }
    boost::property_tree::ptree asPtree() const {
      boost::property_tree::ptree pt;
      pt.put("dest", dest);
      pt.put("via", via);
      return pt;
    }
    static bool fromPtree(boost::property_tree::ptree const &pt, RouteVia& rv) {
      rv.dest = pt.get<std::string>("dest");
      rv.via = pt.get<std::string>("via");
      return rv.isValid();
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

  template <typename T>
  static std::vector<T> asVector(boost::property_tree::ptree const &pt,
    boost::property_tree::ptree::key_type const &key) {
    std::vector<T> r;
    for (auto &item : pt.get_child(key)) {
      r.push_back(item.second.get_value<T>());
    }
    return r;
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
  const std::vector<std::string> &getAddrs() const {
    return addrs;
  }
  const std::vector<RouteVia> &getRoutes() const {
    return routes;
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

  boost::property_tree::ptree asPtree() const {
    boost::property_tree::ptree pt;
    boost::property_tree::ptree ips;
    for (auto &addr : addrs) {
      boost::property_tree::ptree ip;
      ip.put("", addr);
      ips.push_back(std::make_pair("", ip));
    }
    if (!addrs.empty()) {
      pt.add_child("ips", ips);
    }

    boost::property_tree::ptree ptRoutes;
    for (auto &route : routes) {
      // boost::property_tree::ptree ptRoute;
      // ptRoute.add_child("", route.asPtree());
      ptRoutes.push_back(std::make_pair("", route.asPtree()));
    }
    if (!addrs.empty()) {
      pt.add_child("routes", ptRoutes);
    }
    return pt;
  }

  static bool fromPtree(boost::property_tree::ptree const &pt, IfAddrs &ifAddrs) {
    for (auto &v : asVector<std::string>(pt, "ips")) {
        if (!ifAddrs.addAddr(v)) {
          LOG(ERROR) << "can not addAddr:" << v;
          return false;
        }
    }
    for (auto &item : pt.get_child("routes")) {
      RouteVia rv;
      if (!RouteVia::fromPtree(item.second, rv)) {
        LOG(ERROR) << "can not fromPtree";
        return false;
      }
      if (!ifAddrs.addRoute(rv)) {
        LOG(ERROR) << "can not addRoute";
        return false;
      }
    }
    return true;
  }
};
