
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
  std::map<std::shared_ptr<typename WsServer::Connection>, std::shared_ptr<TunDevice> >
      tunDevices;


  static constexpr unsigned long long int
  HashStringToInt(const char *str, unsigned long long int hash = 0) {
    return (*str == 0) ? hash : 101 * HashStringToInt(str + 1) + *str;
  }

  void respondTunDevice() {

  }

  void startStatThread(std::shared_ptr<TunDevice> tun,
    std::shared_ptr<typename WsServer::Connection> connection) {
      tun->setStatThread(new std::thread([tun, &connection, this]() {
        // while () {
        //   tun->statId;
        // }
      }));
  }

  void startRecvThread(std::shared_ptr<TunDevice> &tun,
                       std::shared_ptr<typename WsServer::Connection> &connection) {
    tun->setRecvThread(new std::thread([tun, &connection, this]() {
      do {
        tun->getFromTun().pop([&connection, this](Packet *pkt) {
          auto send_stream =
              std::make_shared<typename WsServer::SendStream>();
          *send_stream << "PAKT";
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
  }

  void actionInit(Json::Value &data,
                   std::shared_ptr<typename WsServer::Connection> &connection,
                   std::shared_ptr<typename WsServer::Message> &message) {
      LOG(INFO) << "running actionInit JSON";
      IfAddrs ifAddrs;
      if (!IfAddrs::fromJson(data, ifAddrs)) {
        LOG(ERROR) << "can't parse ifAddrs";
        return;
      }
      std::shared_ptr<TunDevice> tun(new TunDevice(ifAddrs, tunaTor.getMtu(), tunaTor.getQsize()));
      if (!tun->start()) {
        LOG(ERROR) << "can't start tun device";
        respondTunDevice(tun);
        return;
      }
      if (!startStatThread()) {
        tun->stop();
        respondTunDevice(tun);
        return;
      }
      if (!startRecvThread()) {
        tun->stop();
        respondTunDevice(tun);
        return;
      }
      tunDevices.insert(std::make_pair(connection, tun));
      // Create TunDevice
      auto send_stream = std::make_shared<typename WsServer::SendStream>();
      Json::Value out;
      tun->asJson(out);
      Json::StyledWriter styledWriter;
      *send_stream << styledWriter.write(out);
      wsServer->send(connection, send_stream,
                     [](const boost::system::error_code &ec) {
                       if (ec) {
                         LOG(ERROR) << "Server: Error sending message:" << ec
                                    << ", error message: " << ec.message();
                       }
                     });

  }
  void actionPong(Json::Value &data,
                   std::shared_ptr<TunDevice> &tun,
                   std::shared_ptr<typename WsServer::Connection> &connection,
                   std::shared_ptr<typename WsServer::Message> &message) {
    IfAddrs addrs;
    IfAddrs::fromJson(data, addrs);
    LOG(INFO) << "running actionPong JSON";
  }
  void processJson(std::shared_ptr<TunDevice> &tun,
                   std::shared_ptr<typename WsServer::Connection> &connection,
                   std::shared_ptr<typename WsServer::Message> &message) {
      LOG(INFO) << "Reading JSON";
      Json::Value json;
      Json::Reader reader;
      if (!reader.parse(*message, json, false)) {
        LOG(ERROR) << "Reading JSON failed";
        return;
      }
      auto action = json["action"].asString().c_str();
      LOG(INFO) << action;
      switch (HashStringToInt(action)) {
        case HashStringToInt("init"):
          actionInit(json["data"], tun, connection, message);
          break;
        case HashStringToInt("pong"):
          actionPong(json["data"], tun, connection, message);
          break;
        default:
          LOG(ERROR) << "unknown action processJson:" << action;
      }
      // auto message_str = message->string();
      // LOG(INFO) << (size_t)connection.get() << ":" << message_str;
      // // init-message
      // //   -> [ip]
      // //   -> routing[{dst,gw,dev}]
      // //   <- tunDev
      // try {
      //   //json << message_str;
      // } catch (boost::property_tree::json_parser::json_parser_error &ex) {
      //   LOG(ERROR) << "can not read_json:" << message_str << ":" << ex.message();
      //   return;
      // }
      //
      // IfAddrs ifAddrs;
      // if (!IfAddrs::fromJson(json, ifAddrs)) {
      //   LOG(ERROR) << "can't deserialize:" << message_str;
      //   return;
      // }
  }
  void processPakt(std::shared_ptr<typename WsServer::Connection> &connection,
                   std::shared_ptr<typename WsServer::Message> &message) {
    LOG(INFO) << "Reading PAKT:" << message.size();
    // if (message.size() >= )
    // tun.getToTun()
  }

public:
  TunaServer(const TunaTor &tunaTor, WsServer *wsServer)
      : tunaTor(tunaTor), wsServer(std::unique_ptr<WsServer>(wsServer)) {}

  WsServer &getWsServer() const { return *(wsServer.get()); }

  void start() {
    auto &init = wsServer->endpoint["^/init$"];
    init.onmessage = [this](
        std::shared_ptr<typename WsServer::Connection> connection,
        std::shared_ptr<typename WsServer::Message> message) {

      auto pbuf = message->rdbuf();
      const size_t sizeBuffer = sizeof("BUFF")-1;
      if (message->size() < sizeBuffer) {
        LOG(ERROR) << "Illegal Size:" << message->size();
        return;
      }
      char buffer[sizeBuffer];
      pbuf->sgetn(buffer,sizeBuffer);
      pbuf->pubseekpos(sizeBuffer);
      auto tunDev = tunDevices.find(connection);
      if (!memcmp("JSON", buffer, sizeBuffer)) {
        processJson(tunDev, connection, message);
      } else if (!memcmp("PAKT", buffer, sizeBuffer)) {
        processPakt(tunDev, connection, message);
      }
      return;
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
