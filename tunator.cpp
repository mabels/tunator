

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "tunator.hpp"
#include "tun_server.hpp"

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();
  // PacketBuffer::start();

  // typedef SimpleWeb::SocketServer<SimpleWeb::WSS> WssServer;
  // typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
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
