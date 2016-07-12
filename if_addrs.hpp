
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

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
    RouteVia() {}
    RouteVia(const std::string &dest, const std::string &via)
        : dest(dest), via(via) {}
    bool isValid() const {
      return IfAddrs::isValidWithPrefix(dest) &&
             IfAddrs::isValidWithoutPrefix(via);
    }
    void asJson(Json::Value &value) const {
      value["dest"] = dest;
      value["via"] = via;
    }
    static bool fromJson(Json::Value &val, RouteVia &rv) {
      rv.dest = val["dest"].asString();
      rv.via = val["via"].asString();
      return true;
    }
  };

private:
  std::vector<std::string> addrs;
  std::vector<RouteVia> routes;

  static std::pair<std::string, std::string>
  splitPrefix(const std::string &addr) {
    auto slash = addr.find("/");
    if (slash == std::string::npos) {
      return std::pair<std::string, std::string>(addr, "");
    }
    return std::pair<std::string, std::string>(addr.substr(0, slash),
                                               addr.substr(slash + 1));
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
    } catch (std::exception &ex) {
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
  const std::vector<std::string> &getAddrs() const { return addrs; }
  const std::vector<RouteVia> &getRoutes() const { return routes; }

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
      s2 << "ip route add " << route.dest << " via " << route.via << " dev "
         << dev << std::endl;
    }
    s2 << "ip link set dev " << dev << " up" << std::endl;
    return s2.str();
  }

  void asJson(Json::Value &val) const {
    {
      Json::Value res(Json::arrayValue);
      for (auto &v : addrs) {
        res.append(v);
      }
      val["ips"] = res;
    }
    {
      Json::Value res(Json::arrayValue);
      for (auto &v : routes) {
        Json::Value route;
        v.asJson(route);
        res.append(route);
      }
      val["routes"] = res;
    }
  }

  static bool fromJson(Json::Value &val, IfAddrs &ifAddrs) {
      for (auto &v : val["ips"]) {
        if (!ifAddrs.addAddr(v.asString())) {
          LOG(ERROR) << "can not addAddr:" << v;
          return false;
        }
      }
      for (auto &item : val["routes"]) {
        RouteVia rv;
        if (!RouteVia::fromJson(item, rv)) {
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
