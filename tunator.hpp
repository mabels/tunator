
#ifndef __TunaTor__
#define __TunaTor__

#include <getopt.h>

#include <string>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <server_ws.hpp>
#include <server_wss.hpp>


template<typename T> class TunaServer;

class TunaTor {
private:
  TunaServer<SimpleWeb::WSS> *tunaServerWSS;
  TunaServer<SimpleWeb::WS> *tunaServerWS;

  unsigned short mtu;
  unsigned short qSize;
  std::string address;
  unsigned short port;
  size_t threads;
  char *cert;
  char *key;
  size_t statFrequence;

  static constexpr unsigned long long int
  HashStringToInt(const char *str, unsigned long long int hash = 0) {
    return (*str == 0) ? hash : 101 * HashStringToInt(str + 1) + *str;
  }

public:
  TunaTor() : tunaServerWSS(0), tunaServerWS(0),
    mtu(1500), qSize(64), address("127.0.0.1"), port(4711),
    threads(4), cert(0), key(0), statFrequence(1000) {}

  ~TunaTor();

  unsigned short getMtu() const { return mtu; }
  unsigned short getQsize() const { return qSize; }
  unsigned short getPort() const { return port; }

  const std::string &getAddress() const { return address; }
  size_t getThreads() const { return threads; }
  const char *getCert() const { return cert; }
  const char *getKey() const { return key; }
  size_t getStatFrequence() const { return statFrequence; }
  bool isSsl() const { return getCert() && getKey(); }

  void dump() {
    LOG(INFO) << "mtu=" << mtu << " qsize=" << qSize
              << "address=" << address << " port=" << port << " threads=" << threads
              << " cert=" << (cert ? cert : "") << " key=" << (key ? key : "")
              << " statFrequence=" << statFrequence;
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
          {"statFrequence", required_argument, 0, 0},
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
      case HashStringToInt("statFrequence"):
        ret.statFrequence = std::stoi(optarg);
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

  void start();
  void stop();

};

#endif
