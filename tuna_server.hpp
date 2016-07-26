
#ifndef __TunaServer__
#define __TunaServer__

#include "tun_device.hpp"
#include "tunator.hpp"

#include <map>
#include <memory>

#include <string.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <server_ws.hpp>
#include <server_wss.hpp>

#include "message.hpp"

class TunaTor;

class TunaServerHelper {
public:
  template<class T>
  static std::string getHeader(std::shared_ptr<typename T::Message> message) {
    auto pbuf = message->rdbuf();
    const size_t sizeBuffer = sizeof("BUFF")-1;
    if (message->size() < sizeBuffer) {
      LOG(ERROR) << "Illegal Size:" << message->size();
      return "";
    }
    char buffer[sizeBuffer+1];
    pbuf->sgetn(buffer,sizeBuffer);
    pbuf->pubseekpos(sizeBuffer);
    buffer[sizeBuffer] = 0;
    //LOG(INFO) << "getHeader:" << buffer;
    return buffer;
  }
};

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

  bool startStatThread(std::shared_ptr<TunDevice> tun,
    std::shared_ptr<typename WsServer::Connection> connection) {
      tun->setStatThread(new std::thread([tun, connection, this]() {
        Json::StyledWriter styledWriter;
        while (tun->getRunning()) {
          //LOG(INFO) << "StatThread:sleep:" << this;
          std::this_thread::sleep_for(std::chrono::milliseconds(tunaTor.getStatFrequence()));
          //LOG(INFO) << "StatThread:run:" << this;
          if (!tun->getRunning()) {
              return;
          }
          sendTun("stat-res", tun, connection);
        }
      }));
      return true;
  }

  bool startRecvThread(std::shared_ptr<TunDevice> &tun,
                       std::shared_ptr<typename WsServer::Connection> &connection) {
    tun->setRecvThread(new std::thread([tun, connection, this]() {
      do {
        tun->getFromTun().pop([&connection, this](Packet *pkt) {
          auto send_stream =
              std::make_shared<typename WsServer::SendStream>();
          *send_stream << "PAKT";
          send_stream->write((const char*)pkt->buf, pkt->size);
          wsServer->send(connection, send_stream,
                         [](const boost::system::error_code &ec) {
                           if (ec != 0) {
                             LOG(ERROR)
                                 << "Server: Error Recv-sending message:" << ec
                                 << ", error message: " << ec.message();
                           }
                           return 0;
                         });
          return 1;
        });
      } while (tun->getRunning());
    }));
    return true;
  }

  void sendTun(const char *_msg,
            std::shared_ptr<TunDevice> tun,
            std::shared_ptr<typename WsServer::Connection> connection) {
    auto send_stream = std::make_shared<typename WsServer::SendStream>();
    MessageRef<TunDevice> msg(_msg, *tun);
    Json::Value out;
    msg.asJson(out);
    Json::StyledWriter styledWriter;
    *send_stream << "JSON";
    *send_stream << styledWriter.write(out);
    wsServer->send(connection, send_stream,
                   [](const boost::system::error_code &ec) {
                     if (ec) {
                       LOG(ERROR) << "Server: Error sending tun-message:" << ec
                                  << ", error message: " << ec.message();
                     }
                   });

  }

  void actionInit(Json::Value &data,
                   std::shared_ptr<typename WsServer::Connection> &connection,
                   std::shared_ptr<typename WsServer::Message> &) {
      // LOG(INFO) << "running actionInit JSON";
      IfAddrs ifAddrs;
      if (!IfAddrs::fromJson(data, ifAddrs)) {
        LOG(ERROR) << "can't parse ifAddrs";
        return;
      }
      // LOG(INFO) << ifAddrs.asCommands(std::string());
      std::shared_ptr<TunDevice> tun(new TunDevice(ifAddrs, tunaTor.getMtu(), tunaTor.getQsize()));
      if (!tun->start()) {
        LOG(ERROR) << "can't start tun device";
        sendTun("init-res", tun, connection);
        return;
      }
      if (!startStatThread(tun, connection)) {
        tun->stop();
        sendTun("init-res", tun, connection);
        return;
      }
      if (!startRecvThread(tun, connection)) {
        tun->stop();
        sendTun("init-res", tun, connection);
        return;
      }
      tunDevices.insert(std::make_pair(connection, tun));
      // Create TunDevice
      sendTun("init-res", tun, connection);
  }

  void actionPong(Json::Value &data,
                   TunDevice *,
                   std::shared_ptr<typename WsServer::Connection> &,
                   std::shared_ptr<typename WsServer::Message> &) {
    IfAddrs addrs;
    IfAddrs::fromJson(data, addrs);
    LOG(INFO) << "running actionPong JSON";
  }

  void processJson(TunDevice *tun,
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
          actionInit(json["data"], connection, message);
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
  void processPacket(std::shared_ptr<TunDevice> &tun,
                  std::shared_ptr<typename WsServer::Connection> &,
                  std::shared_ptr<typename WsServer::Message> &message) {
    //LOG(INFO) << "Reading PAKT:" << message->size();
    // if (message.size() >= )
    tun->getToTun().push([message](Packet *pkt) {
      if (pkt == 0) {
        LOG(ERROR) << "push failed";
        return -1;
      }
      message->rdbuf()->sgetn((char*)(pkt->buf), message->size());
      return (int)message->size();
    });
  }

public:
  TunaServer(const TunaTor &tunaTor, WsServer *wsServer)
      : tunaTor(tunaTor), wsServer(std::unique_ptr<WsServer>(wsServer)) {
    LOG(INFO) << "TunServer:" << this;
  }
  ~TunaServer() {
    LOG(INFO) << "~TunServer:" << this;
  }


  WsServer &getWsServer() const { return *(wsServer.get()); }

  void waitGraceFull() const {
    bool first = true;
    while (tunDevices.size()) {
      if (first) {
        LOG(INFO) << "waitGraceFull";
        first = false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!first) {
      LOG(INFO) << "waitGraceFull:completed";
    }
  }


  void start() {
    auto &init = wsServer->endpoint["^/init$"];
    init.onmessage = [this](
        std::shared_ptr<typename WsServer::Connection> connection,
        std::shared_ptr<typename WsServer::Message> message) {

      auto buffer = TunaServerHelper::getHeader<WsServer>(message);
      auto tunDev = tunDevices.find(connection);
      if ("JSON" == buffer) {
        if (tunDev == tunDevices.end()) {
          processJson(0, connection, message);
        } else {
          processJson(tunDev->second.get(), connection, message);
        }
      } else {
        if (tunDev == tunDevices.end()) {
          LOG(ERROR) << "Can not find connection";
          return;
        }
        if ("PAKT" == buffer) {
          processPacket(tunDev->second, connection, message);
        }
      }
      return;
    };
    init.onopen = [](
        std::shared_ptr<typename WsServer::Connection> connection) {
      LOG(INFO) << "Server: Opened connection " << (size_t)connection.get();
    };
    init.onclose = [this](std::shared_ptr<typename WsServer::Connection> connection,
                      int status, const std::string & /*reason*/) {
      LOG(INFO) << "Server: Closed connection " << (size_t)connection.get()
                 << " with status code " << status;
      auto tunDev = tunDevices.find(connection);
      if (tunDev == tunDevices.end()) {
        LOG(ERROR) << "Server: Closed connection not found";
        return;
      }
      tunDev->second->stop();
      tunDevices.erase(tunDev);
    };
    init.onerror = [](std::shared_ptr<typename WsServer::Connection> connection,
                      const boost::system::error_code &ec) {
      LOG(INFO) << "Server: Error in connection " << (size_t)connection.get()
                 << ". "
                 << "Error: " << ec << ", error message: " << ec.message();
    };
    wsServer->start();
  }
};

#endif
