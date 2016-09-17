#ifndef __IfAddrs__
#define __IfAddrs__

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <arpa/inet.h>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <json/json.h>

#include "system_cmd.hpp"
#include <ipaddress.hpp>
#include <result.hpp>

class IfAddrs {
public:
  class RouteVia {
  public:
    IPAddress dest;
    IPAddress via;
    RouteVia() {}
    static Result<RouteVia> create(const std::string& _dest, const std::string& _via) {
      auto dest = IPAddress::parse(_dest);
      if (dest.isErr()) {
	return Err<RouteVia>(dest.text());
      } 
      auto via = IPAddress::parse(_via);
      if (via.isErr()) {
	return Err<RouteVia>(via.text());
      } 
      if (!via.unwrap().ip_same_kind(dest.unwrap())) {
	return Err<RouteVia>("is not the same_kind");
      }
      if (via.unwrap().prefix.num != via.unwrap().ip_bits->bits) {
	return Err<RouteVia>("is not a hostaddress");
      }
      auto rv = RouteVia();
      rv.dest = dest.unwrap();
      rv.via = via.unwrap();
      return Ok(rv);
    }
    //RouteVia(const std::string &dest, const std::string &via)
    //   : dest(dest), via(via) {}
    bool isValid() const {
      return true;
    }
    void asJson(Json::Value &value) const {
      value["dest"] = dest.to_string();
      value["via"] = via.to_s();
    }
    static bool fromJson(Json::Value &val, RouteVia &rv) {
      auto dest = IPAddress::parse(val["dest"].asString());
      auto via = IPAddress::parse(val["via"].asString());
      if (dest.isErr() || via.isErr()) {
	return false;
      } 
      rv.dest = dest.unwrap();
      rv.via = via.unwrap();
      return true;
    }
  };

private:
  size_t mtu;
  std::vector<IPAddress> dests;
  std::vector<IPAddress> addrs;
  std::vector<RouteVia> routes;

public:
  IfAddrs() : mtu(1360) {
    // LOG(INFO) << addrs.size() << ":" << addrs.empty();
    // LOG(INFO) << asCommands("isEcho");
  }
  const std::vector<IPAddress> &getDests() const { return dests; }
  const std::vector<IPAddress> &getAddrs() const { return addrs; }
  const std::vector<RouteVia> &getRoutes() const { return routes; }

  void setMtu(size_t _mtu) {
     mtu = _mtu;
  }
  size_t getMtu() const {
    return mtu;
  }

  bool isEcho() const {
    // LOG(INFO) << addrs.size() << ":" << addrs.empty();
    // LOG(INFO) << asCommands("isEcho");
    return addrs.empty();
  }
  bool addDest(const std::string &dest) {
    auto ipa = IPAddress::parse(dest);
    if (ipa.isErr()) {
      return false;
    }
    dests.push_back(ipa.unwrap());
    return true;
  }
  bool addAddr(const std::string &addr) {
    auto ipa = IPAddress::parse(addr);
    if (ipa.isErr()) {
      return false;
    }
    addrs.push_back(ipa.unwrap());
    return true;
  }
  bool addRoute(const RouteVia &route) {
    if (!route.isValid()) {
      return false;
    }
    routes.push_back(route);
    return true;
  }
  std::vector<SystemCmd> asCommands(const std::string &dev) const;

  void asJson(Json::Value &val) const {
    val["mtu"] = Json::Value((Json::UInt64)mtu);
    {
      Json::Value res(Json::arrayValue);
      for (auto &v : dests) {
        res.append(v.to_string());
      }
      val["dests"] = res;
    }
    {
      Json::Value res(Json::arrayValue);
      for (auto &v : addrs) {
        res.append(v.to_string());
      }
      val["addrs"] = res;
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
    for (auto &v : val["dests"]) {
      if (!ifAddrs.addDest(v.asString())) {
        LOG(ERROR) << "can not addDest:" << v;
        return false;
      }
    }
    ifAddrs.setMtu(val.get("mtu", (Json::UInt64)ifAddrs.getMtu()).asInt());
      for (auto &v : val["addrs"]) {
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

#endif
