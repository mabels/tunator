

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include "tunator.hpp"
#include "tuna_server.hpp"


TunaTor::~TunaTor() {
  if (tunaServerWS) {
      delete tunaServerWS;
  }
  if (tunaServerWSS) {
      delete tunaServerWSS;
  }
}

void TunaTor::stop() {
  LOG(INFO) << "TunaTor::+stop:" << this;
  if (tunaServerWS) {
    tunaServerWS->waitGraceFull();
    tunaServerWS->getWsServer().stop();
  }
  if (tunaServerWSS) {
    tunaServerWSS->waitGraceFull();
    tunaServerWSS->getWsServer().stop();
  }
  LOG(INFO) << "TunaTor::-stop:" << this;
}

void TunaTor::start() {
  typedef SimpleWeb::SocketServer<SimpleWeb::WSS> WssServer;
  typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
  if (isSsl()) {
    LOG(INFO) << "Starting SSL on port " << getPort();
    tunaServerWSS = new TunaServer<SimpleWeb::WSS>(
        *this, new WssServer(getPort(), getThreads(), getCert(), getKey()));
    tunaServerWSS->getWsServer().config.address = getAddress();
    tunaServerWSS->start();
  } else {
    LOG(INFO) << "Starting on port " << getPort();
    tunaServerWS = new TunaServer<SimpleWeb::WS>(
        *this, new WsServer(getPort(), getThreads()));
    tunaServerWS->getWsServer().config.address = getAddress();
    tunaServerWS->start();
  }
}
