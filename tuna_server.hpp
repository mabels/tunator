
#ifndef __TunaServer__
#define __TunaServer__

#include "tunator.hpp"
#include "tun_device.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <server_ws.hpp>
#include <server_wss.hpp>

class TunaTor;

template <class socketType> class TunaServer {
  const TunaTor &tunaTor;
  typedef SimpleWeb::SocketServer<socketType> WsServer;
  std::unique_ptr<WsServer> wsServer;
  std::map<std::string, TunDevice> ip;

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
    wsServer->start();
  }

};

#endif
