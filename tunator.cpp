#include <server_ws.hpp>
#include <server_wss.hpp>

#include <getopt.h>

#include <boost/lockfree/policies.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

constexpr unsigned long long int
HashStringToInt(const char *str, unsigned long long int hash = 0) {
  return (*str == 0) ? hash : 101 * HashStringToInt(str + 1) + *str;
}
// using namespace std;

class TunaTor {
private:
  short port;
  size_t threads;
  char *cert;
  char *key;

public:
  TunaTor() : port(4711), threads(4), cert(0), key(0) {}

  short getPort() const { return port; }
  size_t getThreads() const { return threads; }
  const char *getCert() const { return cert; }
  const char *getKey() const { return key; }
  bool isSsl() const { return getCert() && getKey(); }

  void dump() {
    LOG(INFO) << "port=" << port << " threads=" << threads
              << " cert=" << (cert ? cert : "") << " key=" << (key ? key : "");
  }

  static TunaTor args(int argc, char **argv) {
    TunaTor ret;
    while (1) {
      // int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static const struct option long_options[] = {
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
};

class TunDevice {
public:
  TunDevice() {}
};

template <class socketType> class TunaServer {
  const TunaTor &tunaTor;
  typedef SimpleWeb::SocketServer<socketType> WsServer;
  std::unique_ptr<WsServer> wsServer;
  std::map<std::string, TunDevice> ip;

public:
  TunaServer(TunaTor &tunaTor, WsServer *wsServer)
      : tunaTor(tunaTor), wsServer(std::unique_ptr<WsServer>(wsServer)) {}

  void start() {
    auto &init = wsServer->endpoint["^/init$"];
    init.onmessage = [this](
        std::shared_ptr<typename WsServer::Connection> connection,
        std::shared_ptr<typename WsServer::Message> message) {
      auto message_str = message->string();
      boost::property_tree::ptree pt;
      std::stringstream s2;
      s2 << message_str;
      LOG(INFO) << (size_t)connection.get() << ":" << message_str;
      // init-message
      //   -> [ip]
      //   -> routing[{dst,gw,dev}]
      //   <- tunDev
      try {
        boost::property_tree::read_json(s2, pt);
        // Create TunDevice
        auto send_stream = std::make_shared<typename WsServer::SendStream>();
        *send_stream << message_str;
        // server.send is an asynchronous function
        wsServer->send(connection, send_stream, [](const boost::system::
                                                       error_code &ec) {
          if (ec) {
            std::cout << "Server: Error sending message. " <<
                // See
                // http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html,
                // Error Codes for error code meanings
                "Error: " << ec << ", error message: " << ec.message()
                      << std::endl;
          }
        });

      } catch (boost::property_tree::json_parser::json_parser_error &je) {
        LOG(ERROR) << je.message();
      }
    };
    init.onopen = [](
        std::shared_ptr<typename WsServer::Connection> connection) {
      LOG(DEBUG) << "Server: Opened connection " << (size_t)connection.get();
    };
    init.onclose = [](std::shared_ptr<typename WsServer::Connection> connection,
                      int status, const std::string & /*reason*/) {
      LOG(DEBUG) << "Server: Closed connection " << (size_t)connection.get()
                 << " with status code " << status;
    };
    init.onerror = [](std::shared_ptr<typename WsServer::Connection> connection,
                      const boost::system::error_code &ec) {
      LOG(DEBUG) << "Server: Error in connection " << (size_t)connection.get()
                 << ". "
                 << "Error: " << ec << ", error message: " << ec.message();
    };
  }
};

static boost::lockfree::spsc_queue<int, boost::lockfree::capacity<100>> q;
static int sum;
class PacketBuffer {
private:
public:
public:
  static void produce() {
    LOG(INFO) << "produce entered";
    size_t drops = 0;
    int i = 1;
    while (1) {
      for (; i%100; ++i) {
        if (!q.push(i)) {
          LOG(INFO) << "drop" << i << ":" << ++drops;
        }
      }
      LOG(INFO) << "produced " << i++;
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
  }
  static void consume() {
    LOG(INFO) << "consume entered";
    int i = 0;
    while (1) {
      int wait = 100;
      while (q.pop(i)) {
        sum += i;
        wait = 0;
      }
      if (wait == 0) {
        LOG(INFO) << i;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
    LOG(INFO) << "exited";
  }

  static void start() {
    std::thread t2{consume};
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    std::thread t1{produce};
    t1.join();
    t2.join();
    LOG(INFO) << "post join";
    q.consume_all([](int i) { sum += i; });
    LOG(INFO) << "sum=" << sum;
  }
};

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();
  PacketBuffer::start();

  typedef SimpleWeb::SocketServer<SimpleWeb::WSS> WssServer;
  typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
  if (tunaTor.isSsl()) {
    LOG(INFO) << "Starting SSL on port " << tunaTor.getPort();
    auto tuna = TunaServer<SimpleWeb::WSS>(
        tunaTor, new WssServer(tunaTor.getPort(), tunaTor.getThreads(),
                               tunaTor.getCert(), tunaTor.getKey()));
    tuna.start();
  } else {
    LOG(INFO) << "Starting on port " << tunaTor.getPort();
    auto tuna = TunaServer<SimpleWeb::WS>(
        tunaTor, new WsServer(tunaTor.getPort(), tunaTor.getThreads()));
    tuna.start();
  }
  // server_thread.join();
  return 0;
}
