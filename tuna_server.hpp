
#ifndef __TunaServer__
#define __TunaServer__

#include "tun_device.hpp"
#include "tunator.hpp"

#include <map>
#include <memory>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <server_ws.hpp>
#include <server_wss.hpp>

class TunaTor;

template <class socketType> class TunaServer {
  const TunaTor &tunaTor;
  typedef SimpleWeb::SocketServer<socketType> WsServer;
  std::unique_ptr<WsServer> wsServer;
  std::map<std::shared_ptr<typename WsServer::Connection>, std::unique_ptr<TunDevice> >
      tunDevices;

public:
  TunaServer(const TunaTor &tunaTor, WsServer *wsServer)
      : tunaTor(tunaTor), wsServer(std::unique_ptr<WsServer>(wsServer)) {}

  WsServer &getWsServer() const { return *(wsServer.get()); }

  void start() {
    auto &init = wsServer->endpoint["^/init$"];
    init.onmessage = [this](
        std::shared_ptr<typename WsServer::Connection> connection,
        std::shared_ptr<typename WsServer::Message> message) {
      auto message_str = message->string();
      LOG(INFO) << (size_t)connection.get() << ":" << message_str;
      // init-message
      //   -> [ip]
      //   -> routing[{dst,gw,dev}]
      //   <- tunDev
      boost::property_tree::ptree pt;
      try {
        boost::property_tree::read_json(message_str, pt);
      } catch (boost::property_tree::json_parser::json_parser_error &ex) {
        LOG(ERROR) << "can not read_json:" << message_str << ":" << ex.message();
        return;
      }

      IfAddrs ifAddrs;
      if (!IfAddrs::fromPtree(pt, ifAddrs)) {
        LOG(ERROR) << "can't deserialize:" << message_str;
        return;
      }
      TunDevice *tun =
          new TunDevice(ifAddrs, tunaTor.getMtu(), tunaTor.getQsize());
      if (!tun->start()) {
        LOG(ERROR) << "can't start tun device";
        return;
      }
      tun->setRecvThread(new std::thread([tun, &connection, this]() {
        do {
          tun->getFromTun().pop([&connection, this](Packet *pkt) {
            auto send_stream =
                std::make_shared<typename WsServer::SendStream>();
            send_stream->write((const char*)pkt->buf, pkt->size);
            wsServer->send(connection, send_stream,
                           [](const boost::system::error_code &ec) {
                             LOG(ERROR)
                                 << "Server: Error sending message:" << ec
                                 << ", error message: " << ec.message();
                             return 0;
                           });
            return 1;
          });
        } while (tun->getRunning());
      }));

      tunDevices.insert(std::pair<std::shared_ptr<typename WsServer::Connection>,
        std::unique_ptr<TunDevice>>(connection, std::unique_ptr<TunDevice>(tun)));
      // Create TunDevice
      auto send_stream = std::make_shared<typename WsServer::SendStream>();
      boost::property_tree::write_json(*send_stream, tun->asPtree(), false);
      wsServer->send(connection, send_stream,
                     [](const boost::system::error_code &ec) {
                       if (ec) {
                         LOG(ERROR) << "Server: Error sending message:" << ec
                                    << ", error message: " << ec.message();
                       }
                     });
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
    wsServer->start();
  }
};

#endif
