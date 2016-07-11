
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
  unsigned short mtu;
  unsigned short qSize;
  std::string address;
  unsigned short port;
  size_t threads;
  char *cert;
  char *key;

public:
  TunaTor() : mtu(1500), qSize(64), address("127.0.0.1"), port(4711), threads(4), cert(0), key(0) {}

  unsigned short getMtu() const { return mtu; }
  unsigned short getQsize() const { return qSize; }
  unsigned short getPort() const { return port; }

  const std::string &getAddress() const { return address; }
  size_t getThreads() const { return threads; }
  const char *getCert() const { return cert; }
  const char *getKey() const { return key; }
  bool isSsl() const { return getCert() && getKey(); }

  void dump() {
    LOG(INFO) << "mtu=" << mtu << " qsize=" << qSize
              << "address=" << address << " port=" << port << " threads=" << threads
              << " cert=" << (cert ? cert : "") << " key=" << (key ? key : "");
  }

  static TunaTor args(int argc, char **argv) {
    TunaTor ret;
    while (1) {
      // int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static const struct option long_options[] = {
          {"mtu", required_argument, 0, 0},
          {"qsize", required_argument, 0, 0},
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
      case HashStringToInt("mtu"):
        ret.mtu = std::stoi(optarg);
        break;
      case HashStringToInt("qsize"):
        ret.qSize = std::stoi(optarg);
        break;
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
