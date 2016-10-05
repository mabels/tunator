#include "../if_addrs.hpp"

#include <iostream>
#include <map>

#include <ipaddress/ipaddress.hpp>

#include "chai.hpp"
#include "mocha.hpp"

//#include <boost/property_tree/json_parser.hpp>
//#include <boost/property_tree/ptree.hpp>

using std::cerr;
using std::endl;

INITIALIZE_EASYLOGGINGPP
int main() {
  describe("testIsValidWithPrefix", []() {
    std::map<std::string, bool> testIsValidWithPrefix = {
        {"", false},
        {"a.b.c.d", false},
        {"300.200.200.200", false},
        {"200.200.200.200", true},
        {"200::200:200:200", true},
        {"/", false},
        {"/17", false},
        {"/a7", false},
        {"200.200.200.200/", false},
        {"200.200.200.200/a7", false},
        {"200.200.200.200/7", true},
        {"200.200.200.200/-1", false},
        {"200.200.200.200/33", false},
        {"zoo::200/", false},
        {"200:200:200::200/a7", false},
        {"200:200:200::200/77", true},
        {"200:200:200::200/-1", false},
        {"200:200:200::200/129", false}};
    for (const auto &kv : testIsValidWithPrefix) {
      auto ipa = IPAddress::parse(kv.first);
      it(kv.first, [kv, ipa]() { Chai::assert.equal(kv.second, ipa.isOk()); });
      // if (kv.second != ipa.isOk()) {
      //   cerr << "IfAddrs::isValidWithPrefix failed on|" << kv.first << "|"
      //        << kv.second << endl;
      //   return 1;
      // }
    }
  });
  describe("testIsValidWithoutPrefix", []() {
    std::map<std::string, bool> testIsValidWithoutPrefix = {
        {"", false},          {"/", false},          {"/a7", false},
        {"/17", false},       {"1.2.3.4/17", true},  {"1:2:3::4/17", true},
        {"300.2.3.4", false}, {"1:zoo:3::4", false}, {"1.2.3.4", true},
        {"1:2:3::4", true}};
    for (const auto &kv : testIsValidWithoutPrefix) {
      auto ipa = IPAddress::parse(kv.first);
      it(kv.first, [kv, ipa]() { Chai::assert.equal(kv.second, ipa.isOk()); });
    }
    // if (kv.second != ipa.isOk()) {
    //   // if (kv.second != IfAddrs::isValidWithoutPrefix(kv.first)) {
    //   cerr << "IfAddrs::isValidWithPrefix failed on:" << kv.first << ":"
    //        << kv.second << endl;
    //   return 1;
    // }
  });
  describe("IfAddrs", []() {
    IfAddrs ia;
    it("addDest", [&ia]() {
      ia.addDest("10.1.0.254/24");
      ia.addDest("fd00::1/112");
    });

    it("addAddr", [&ia]() {
      ia.addAddr("10.1.0.1/24");
      ia.addAddr("fd00::1000/112");
      Chai::assert.isTrue(ia.addAddr("10.2.0.1/24"));
      Chai::assert.isFalse(ia.addAddr("256.2.0.1/24"));
    });

    it("addRoute", [&ia]() {
      ia.addRoute(
          IfAddrs::RouteVia::create("172.16.0.1/24", "172.16.0.254").unwrap());
      Chai::assert.isTrue(ia.addRoute(
          IfAddrs::RouteVia::create("172.17.0.1/24", "172.17.0.254").unwrap()), "-1-");
      Chai::assert.isTrue(
          IfAddrs::RouteVia::create("300.17.0.1/24", "172.17.0.254").isErr(), "-2-");
      Chai::assert.isTrue(
          IfAddrs::RouteVia::create("129.17.0.1/24", "172.17.0.254/23")
              .isErr(), "-3-");
      Chai::assert.isTrue(
          IfAddrs::RouteVia::create("129.17.0.1/24", "172.17.0.354").isErr(), "-4-");
    });

    it("asCommands", [&ia]() {
      auto cmds = ia.asCommands("DEV");
      extern char *IfAddrsRef[];
      int idx = 0;
      for (auto cmd : cmds) {
        Chai::assert.equal(cmd.dump(), IfAddrsRef[idx]);
        ++idx;
      }
    });
    it("asJson-1", [&ia]() {
      Json::StyledWriter styledWriter;
      Json::Value iaFrom;
      ia.asJson(iaFrom);
      auto from = styledWriter.write(iaFrom);
      Json::Reader reader;
      std::stringstream fromStream;
      fromStream << from;
      Json::Value iaTo;
      reader.parse(fromStream, iaTo, false);
      IfAddrs my;
      IfAddrs::fromJson(iaTo, my);
      Json::Value out;
      my.asJson(out);
      auto to = styledWriter.write(out);
      Chai::assert.equal(from, to);
    });

    it("asJson-2", []() {
      IfAddrs ia;
      Json::Value iaFrom;
      ia.asJson(iaFrom);
      Json::StyledWriter styledWriter;
      auto from = styledWriter.write(iaFrom);
      Json::Reader reader;
      std::stringstream fromStream;
      fromStream << from;
      Json::Value iaTo;
      reader.parse(fromStream, iaTo, false);
      IfAddrs my;
      IfAddrs::fromJson(iaTo, my);
      Json::Value out;
      my.asJson(out);
      auto to = styledWriter.write(out);
      Chai::assert.equal(from, to);
    });
  });
  exit();
}
