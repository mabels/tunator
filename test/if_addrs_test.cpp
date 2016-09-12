#include "../if_addrs.hpp"

#include <iostream>
#include <map>

//#include <boost/property_tree/json_parser.hpp>
//#include <boost/property_tree/ptree.hpp>

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
  ia.addDest("10.2.1.0");
  ia.addDest("fd00::1");
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

  auto cmds = ia.asCommands("DEV");
  int idx = 0;
  extern char *IfAddrsRef[];
  bool needExit = false;
  for (auto cmd : cmds) {
      if (cmd.dump() != IfAddrsRef[idx]) {
        cerr << "wrong string " << cmd.dump() << "!=" << IfAddrsRef[idx] << endl;
        needExit = true;
      }
      ++idx;
  }
  if (needExit) { return 1; }
  {
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
    if (from != to) {
       cerr << from << "!=" << to;
    }
  }

  {
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
    if (from != to) {
       cerr << from << "!=" << to;
    }
  }
  return 0;
}
