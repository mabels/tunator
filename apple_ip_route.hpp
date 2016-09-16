
#include <istream>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <boost/tokenizer.hpp>

#include "iproute.hpp"
#include "system_cmd.hpp"

class AppleIpRoute {
  public:
    static std::string serialize(const IPRoute &ipr) {
      std::stringstream s2;
        auto dest = ipr.dest.to_string();
        if (dest == "0.0.0.0/0") {
          s2 << "default";
        } else {
          s2 << dest;
        }
        if (ipr.via.isSome()) {
          s2 << " via " << ipr.via.unwrap().to_s();
        }
        if (ipr.dev.isSome()) {
          s2 << " dev " << ipr.dev.unwrap();
        }
        if (ipr.proto.isSome()) {
          s2 << " proto " << ipr.proto.unwrap();
        }
        if (ipr.scope.isSome()) {
          s2 << " scope " << ipr.scope.unwrap();
        }
        if (ipr.src.isSome()) {
          s2 << " src " << ipr.src.unwrap().to_s();
        }
        if (ipr.metric.isSome()) {
          s2 << " metric " << ipr.metric.unwrap();
        }
        return s2.str();
    }
    static std::string add(const IPRoute &ipr) { return AppleIpRoute::command(ipr, "add"); }
    static std::string del(const IPRoute &ipr) { return AppleIpRoute::command(ipr, "del"); }
    static std::vector<std::string> command(const std::vector<IPRoute> &iprs, const char *mod) {
      std::vector<std::string> ret;
      for (auto ipr : iprs) {
        ret.push_back(AppleIpRoute::command(ipr, mod));
      }
      return ret;
    }
    static std::vector<std::string> add(const std::vector<IPRoute> &iprs) {
      return AppleIpRoute::command(iprs, "add");
    }
    static std::vector<std::string> del(const std::vector<IPRoute> &iprs) {
      return AppleIpRoute::command(iprs, "del");
    }
    static std::string command(const IPRoute &ipr, const char *mod) {
      std::stringstream ipcmd;
      ipcmd << "/sbin/route " << mod << " " << AppleIpRoute::serialize(ipr);
      return ipcmd.str();
    }
    static Result<std::vector<IPRoute>> read() {
      auto srv4 = SystemCmd("/usr/sbin/netstat").arg("-rn").arg("-f").arg("inet").run();
      auto srv6 = SystemCmd("/usr/sbin/netstat").arg("-rn").arg("-f").arg("inet6").run();
      if (!srv4.ok || !srv6.ok || srv4.exitCode != 0 || srv6.exitCode != 0) {
        return Err<std::vector<IPRoute>>("failure while running ip command");
      }
      std::stringstream s2;
      s2 << srv4.sout.str() << srv6.sout.str();
      return AppleIpRoute::parse(s2.str());
    }
    static Result<std::vector<IPRoute>> parse(const std::string &_lines) {
      std::vector<IPRoute> ret;
      std::string line;
      const boost::char_separator<char> inlineSep("\t ");
      const boost::char_separator<char> inlineEol("\n\r");
      //std::istream_iterator<char> itLines(_lines);
      bool skipHeader = true;
      boost::tokenizer<boost::char_separator<char>> lines(_lines, inlineEol);
//std::cout << "parse-1" << std::endl;
      for(auto line : lines) {
//std::cout << "parse-2" << line << std::endl;
         IPRoute ipr;
         bool first = true;
         bool name = true;
         std::string last;
         bool isErr = false;
         boost::tokenizer<boost::char_separator<char>> tokens(line, inlineSep);
         for (const auto& _ : tokens) {
           std::string t(_);
           if (skipHeader) {
             skipHeader = t != "Destination";
             break;
           }
//std::cout << "parse-3" << _ << std::endl;
           if (first) {
             auto ipa = IPAddress::parse(t == "default" ? "0.0.0.0/0" : t);
             if (ipa.isErr()) {
              //std::cout << "parse-3-x:" << ipa.text() << "[" << t << "]" << std::endl;
               isErr = true;
               break;
             }
             //std::cout << "parse-3-3:" << t << std::endl;
             //std::cout << "parse-3-2:" << t << ":" << ipa.unwrap() << std::endl;
             ipr.dest = ipa.unwrap();
             first = false;
           } else {
             if (name) {
               last = t;
               name = false;
             } else {
               name = true;
               if (last == "via") {
                 auto via = IPAddress::parse(t);
                 if (via.isErr()) {
                   isErr |= true;
                   break;
                 }
                 ipr.via = Some(via.unwrap());
               } else if (last == "dev") {
                 ipr.dev = Some(t);
               } else if (last == "proto") {
                 ipr.proto = Some(t);
               } else if (last == "scope") {
                 ipr.scope = Some(t);
               } else if (last == "src") {
                 auto src = IPAddress::parse(t);
                 if (src.isErr()) {
                   isErr |= true;
                   break;
                 }
                 ipr.src = Some(src.unwrap());
               } else if (last == "metric") {
                 size_t metric = stoi(t, 0, 10);
                 ipr.metric = Some(metric);
               }
             }
           }
         }
         if (isErr) {
           return Err<std::vector<IPRoute>>("can't parse");
         }
         //std::cout << "IPR.dest=" << ipr.dest.to_string() << std::endl;
         ret.push_back(ipr);
      }
      return Ok(ret);
    }
    static std::string serialize(const std::vector<IPRoute> &iprs) {
      std::stringstream s2;
      for (auto &ipr : iprs) {
//std::cout << "serial-3-1" << ipr.dest << std::endl;
//std::cout << "parse-3-2" << std::endl;
        s2 << AppleIpRoute::serialize(ipr);
        s2 << std::endl;
      }
//std::cout << "parse-4" << std::endl;
      return s2.str();
    }
};
