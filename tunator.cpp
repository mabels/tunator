

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include "tunator.hpp"
#include "tuna_server.hpp"

void TunaTor::start() const {
  typedef SimpleWeb::SocketServer<SimpleWeb::WSS> WssServer;
  typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
  if (isSsl()) {
    LOG(INFO) << "Starting SSL on port " << getPort();
    auto tuna = TunaServer<SimpleWeb::WSS>(
        *this, new WssServer(getPort(), getThreads(), getCert(), getKey()));
    tuna.getWsServer().config.address = getAddress();
    tuna.start();
  } else {
    LOG(INFO) << "Starting on port " << getPort();
    auto tuna = TunaServer<SimpleWeb::WS>(
        *this, new WsServer(getPort(), getThreads()));
    tuna.getWsServer().config.address = getAddress();
    tuna.start();
  }
}
