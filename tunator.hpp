
#ifndef __TunaTor__
#define __TunaTor__

#include <getopt.h>

#include <string>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

constexpr unsigned long long int
HashStringToInt(const char *str, unsigned long long int hash = 0) {
  return (*str == 0) ? hash : 101 * HashStringToInt(str + 1) + *str;
}

class TunaTor {
private:
  std::string address;
  short port;
  size_t threads;
  char *cert;
  char *key;

public:
  TunaTor() : address("127.0.0.1"), port(4711), threads(4), cert(0), key(0) {}

  const std::string &getAddress() const { return address; }
  short getPort() const { return port; }
  size_t getThreads() const { return threads; }
  const char *getCert() const { return cert; }
  const char *getKey() const { return key; }
  bool isSsl() const { return getCert() && getKey(); }

  void dump() {
    LOG(INFO) <<"address=" << address << " port=" << port << " threads=" << threads
              << " cert=" << (cert ? cert : "") << " key=" << (key ? key : "");
  }

  static TunaTor args(int argc, char **argv) {
    TunaTor ret;
    while (1) {
      // int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static const struct option long_options[] = {
          {"address", required_argument, 0, 0},
          {"port", required_argument, 0, 0},
          {"threads", required_argument, 0, 0},
          {"cert", required_argument, 0, 0},
          {"key", required_argument, 0, 0},
          {0, 0, 0, 0}};
      auto c = getopt_long(argc, argv, "", long_options, &option_index);
      if (c == -1) {
        break;
      }
      if (c != 0) {
        // only long_options
        continue;
      }
      switch (HashStringToInt(long_options[option_index].name)) {
      case HashStringToInt("address"):
        ret.address = optarg;
        break;
      case HashStringToInt("port"):
        ret.port = std::stoi(optarg);
        break;
      case HashStringToInt("threads"):
        ret.threads = std::stoi(optarg);
        break;
      case HashStringToInt("cert"):
        ret.cert = optarg;
        break;
      case HashStringToInt("key"):
        ret.key = optarg;
        break;
      default:
        std::cout << "option " << long_options[option_index].name;
        if (optarg) {
          std::cout << " with arg " << optarg;
        }
        std::cout << std::endl;
      }
    }
    return ret;
  }

  void start() const;

};

#endif
