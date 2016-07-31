
#include <client_ws.hpp>

#include <tunator.hpp>
#include <message.hpp>
#include <if_addrs.hpp>

#include <iostream>
#include <string>
#include <thread>

#include "../tun_device.hpp"
#include "../tuna_server.hpp"
#include "../message.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;


typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;

void test_not_startable(TunaTor& tunator) {
  WsClient client("127.0.0.1:4711/init");
  client.onmessage = [&client]( std::shared_ptr<WsClient::Message> message) {
    auto wsPacket = WsPacket<WsClient>::from(*message);
    if (wsPacket.header != "JSON") {
      LOG(ERROR) << "Reading HEADER failed";
      return;
    }
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(wsPacket.message, json, false)) {
      LOG(ERROR) << "Reading JSON failed";
      return;
    }
    Message<TunDevice> tunMsg("doof");
    Message<TunDevice>::fromJson(json, tunMsg);
    if (tunMsg.getAction() != "init-res") {
        LOG(ERROR) << "action missmatch: init-res!=" << tunMsg.getAction();
        exit(-1);
    }
    if (tunMsg.getData().getRunning()) {
        LOG(ERROR) << "tun should not be started";
        exit(-1);
    }
    client.send_close(100);
    //client.close();
  };
  client.onopen = [&client]() {
    Message<IfAddrs> ifAddrs("init");
    ifAddrs.getData().setMtu(40000);
    auto send_stream = std::make_shared<WsClient::SendStream>();
    Json::Value jsVal;
    ifAddrs.asJson(jsVal);
    Json::StyledWriter styledWriter;
    auto from = styledWriter.write(jsVal);
    *send_stream << "JSON";
    *send_stream << from;
    client.send(send_stream);
  };
  client.onclose = [&tunator](int status, const string & /*reason*/) {
    LOG(INFO) << "Client: Closed connection with status code " << status;

  };
  client.onerror = [](const boost::system::error_code &ec) {
    LOG(INFO) << "Client: Error: " << ec << ", error message: " << ec.message();
  };
  LOG(INFO) << "client.start";
  client.start();
  LOG(INFO) << "done client.start";
}


void test_startable(TunaTor& tunator) {
  WsClient client("127.0.0.1:4711/init");
  client.onmessage = [&client]( std::shared_ptr<WsClient::Message> message) {
    auto wsPacket = WsPacket<WsClient>::from(*message);
    if (wsPacket.header != "JSON") {
      LOG(ERROR) << "Reading HEADER failed";
      return;
    }
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(wsPacket.message, json, false)) {
      LOG(ERROR) << "Reading JSON failed";
      return;
    }
    Message<TunDevice> tunMsg("doof");
    Message<TunDevice>::fromJson(json, tunMsg);
    if (tunMsg.getAction() != "init-res") {
        LOG(ERROR) << "action missmatch: init-res!=" << tunMsg.getAction();
        exit(-1);
    }
    if (!tunMsg.getData().getRunning()) {
        LOG(ERROR) << "tun should not be started";
        exit(-1);
    }
    client.send_close(100);
    //client.close();
  };
  client.onopen = [&client]() {
    Message<IfAddrs> ifAddrs("init");
    auto send_stream = std::make_shared<WsClient::SendStream>();
    Json::Value jsVal;
    ifAddrs.asJson(jsVal);
    Json::StyledWriter styledWriter;
    auto from = styledWriter.write(jsVal);
    *send_stream << "JSON";
    *send_stream << from;
    client.send(send_stream);
  };
  client.onclose = [&tunator](int status, const string & /*reason*/) {
    LOG(INFO) << "Client: Closed connection with status code " << status;

  };
  client.onerror = [](const boost::system::error_code &ec) {
    LOG(INFO) << "Client: Error: " << ec << ", error message: " << ec.message();
  };
  LOG(INFO) << "client.start";
  client.start();
  LOG(INFO) << "done client.start";
}


void test_wait_status(TunaTor& tunator) {
  WsClient client("127.0.0.1:4711/init");
  size_t wait_count = 0;
  client.onmessage = [&client, &wait_count]( std::shared_ptr<WsClient::Message> message) {
    auto wsPacket = WsPacket<WsClient>::from(*message);
    if (wsPacket.header != "JSON") {
      LOG(ERROR) << "Reading HEADER failed";
      return;
    }
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(*message, json, false)) {
      LOG(ERROR) << "Reading JSON failed";
      return;
    }
    Message<TunDevice> tunMsg("doof");
    Message<TunDevice>::fromJson(json, tunMsg);
    if (!tunMsg.getData().getRunning()) {
        LOG(ERROR) << "tun should not be started";
        exit(-1);
    }
    if (wait_count + 1 != tunMsg.getData().getSeq()) {
        LOG(ERROR) << "seq mismatch:" << wait_count << "!=" << tunMsg.getData().getSeq();
        exit(-1);
    }
    if (wait_count == 0) {
      if (tunMsg.getAction() != "init-res") {
          LOG(ERROR) << "action missmatch: init-res!=" << tunMsg.getAction();
          exit(-1);
      }
      ++wait_count;
    } else if (wait_count < 4) {
      if (tunMsg.getAction() != "stat-res") {
          LOG(ERROR) << "action missmatch: stat-res!=" << tunMsg.getAction();
          exit(-1);
      }
      ++wait_count;
    } else {
      client.send_close(4711);
    }
  };
  client.onopen = [&client]() {
    Message<IfAddrs> ifAddrs("init");
    auto send_stream = std::make_shared<WsClient::SendStream>();
    Json::Value jsVal;
    ifAddrs.asJson(jsVal);
    Json::StyledWriter styledWriter;
    auto from = styledWriter.write(jsVal);
    *send_stream << "JSON";
    *send_stream << from;
    client.send(send_stream);
  };
  client.onclose = [&tunator](int status, const string & /*reason*/) {
    LOG(INFO) << "Client: Closed connection with status code " << status;

  };
  client.onerror = [](const boost::system::error_code &ec) {
    LOG(INFO) << "Client: Error: " << ec << ", error message: " << ec.message();
  };
  LOG(INFO) << "client.start";
  client.start();
  LOG(INFO) << "done client.start";
}

void sendPacket(WsClient &client, size_t &packetId) {
  auto send_stream = std::make_shared<WsClient::SendStream>();
  *send_stream << "PAKT";
  *send_stream << packetId;
  for (int i = 0; i < 40; ++i) {
    *send_stream << ":" << "TestPacket" << ":";
  }
  if (packetId % 93 == 0) {
    LOG(INFO) << "sendPacket:" << packetId;
  }
  *send_stream << packetId;
  packetId++;
  client.send(send_stream);
}

void test_packets(TunaTor& tunator) {
  WsClient client("127.0.0.1:4711/init");
  size_t packetId = 0;
  client.onmessage = [&client, &packetId]( std::shared_ptr<WsClient::Message> message) {
    auto wsPacket = WsPacket<WsClient>::from(*message);
    if (wsPacket.header == "PAKT") {
      std::stringstream packetIdStr;
      packetIdStr << packetId - 1;
      char buffer[wsPacket.packetLen+1];
      wsPacket.copyOut(buffer,wsPacket.packetLen);
      buffer[wsPacket.packetLen] = 0;
      std::string packet(buffer);
      if (packet.substr(0,packetIdStr.str().length()) != packetIdStr.str() ||
          packet.substr(packet.length()-packetIdStr.str().length()) != packetIdStr.str()) {

        LOG(ERROR) << "packetId missmatch:" << message->size() << ":" << packet.length()
          << ":" << packetIdStr.str() << "|" << packet;
        exit(-4);
      }
      sendPacket(client, packetId);
      if (packetId >= 10000) {
        LOG(INFO) << "transmitted packets:" << packetId;
        client.send_close(4711);
      }
      return;
    }
    if (wsPacket.header != "JSON") {
      LOG(ERROR) << "Reading HEADER failed:" << wsPacket.header;
      return;
    }
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(wsPacket.message, json, false)) {
      LOG(ERROR) << "Reading JSON failed";
      return;
    }
    Message<TunDevice> tunMsg("doof");
    Message<TunDevice>::fromJson(json, tunMsg);
    if (!tunMsg.getData().getRunning()) {
        LOG(ERROR) << "tun should not be started";
        exit(-1);
    }
    if (tunMsg.getAction() == "init-res") {
      auto send_stream = std::make_shared<WsClient::SendStream>();
      sendPacket(client, packetId);
    }
    //
  };
  client.onopen = [&client]() {
    Message<IfAddrs> ifAddrs("init");
    auto send_stream = std::make_shared<WsClient::SendStream>();
    Json::Value jsVal;
    ifAddrs.asJson(jsVal);
    Json::StyledWriter styledWriter;
    auto from = styledWriter.write(jsVal);
    *send_stream << "JSON";
    *send_stream << from;
    client.send(send_stream);
  };
  client.onclose = [&tunator](int status, const string & /*reason*/) {
    LOG(INFO) << "Client: Closed connection with status code " << status;

  };
  client.onerror = [](const boost::system::error_code &ec) {
    LOG(INFO) << "Client: Error: " << ec << ", error message: " << ec.message();
  };
  LOG(INFO) << "client.start";
  client.start();
  LOG(INFO) << "done client.start";
}

typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;
void simpleEchoServer(WsServer &server) {
  //Example 1: echo WebSocket endpoint
  //  Added debug messages for example use of the callbacks
  //  Test with the following JavaScript:
  //    var ws=new WebSocket("ws://localhost:8080/echo");
  //    ws.onmessage=function(evt){console.log(evt.data);};
  //    ws.send("test");
  auto& echo=server.endpoint["^/echo/?$"];

  echo.onmessage=[&server](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::Message> message) {
      //WsServer::Message::string() is a convenience function for:
      //stringstream data_ss;
      //data_ss << message->rdbuf();
      //auto message_str = data_ss.str();
      auto message_str=message->string();

      // cout << "Server: Message received: \"" << message_str << "\" from " << (size_t)connection.get() << endl;
      //
      // cout << "Server: Sending message \"" << message_str <<  "\" to " << (size_t)connection.get() << endl;

      auto send_stream=std::make_shared<WsServer::SendStream>();
      *send_stream << message_str;
      //server.send is an asynchronous function
      server.send(connection, send_stream, [](const boost::system::error_code& ec){
          if(ec) {
              cout << "Server: Error sending message. " <<
              //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                      "Error: " << ec << ", error message: " << ec.message() << endl;
          }
      });
  };

  echo.onopen=[](std::shared_ptr<WsServer::Connection> connection) {
      cout << "Server: Opened connection " << (size_t)connection.get() << endl;
  };

  //See RFC 6455 7.4.1. for status codes
  echo.onclose=[&server](std::shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
      cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
  };

  //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  echo.onerror=[](std::shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
      cout << "Server: Error in connection " << (size_t)connection.get() << ". " <<
              "Error: " << ec << ", error message: " << ec.message() << endl;
  };
}

void simpleEchoClient(WsServer &server) {
  WsClient client("localhost:8080/echo");
  ssize_t count = 30000;
  auto start = std::chrono::steady_clock::now();
  client.onmessage=[&client, &server, &count, &start](std::shared_ptr<WsClient::Message> message) {
      auto message_str=message->string();
      //cout << "Client: Message received: \"" << message_str << "\"" << endl;
      if (--count < 0) {
        cout << "Client: Sending close connection" << endl;
        client.send_close(1000);
        auto took = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start);
        cout << "1000 took " << took.count() << endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //server.stop();
      } else {
          string message="Hello";
          //cout << "Client: Sending message: \"" << message << "\"" << endl;
          auto send_stream=std::make_shared<WsClient::SendStream>();
          *send_stream << message;
          client.send(send_stream);
      }
  };

  client.onopen=[&client]() {
      cout << "Client: Opened connection" << endl;
      string message="Hello";
      //cout << "Client: Sending message: \"" << message << "\"" << endl;
      auto send_stream=std::make_shared<WsClient::SendStream>();
      *send_stream << message;
      client.send(send_stream);
  };

  client.onclose=[](int status, const string& /*reason*/) {
      cout << "Client: Closed connection with status code " << status << endl;
  };

  //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  client.onerror=[](const boost::system::error_code& ec) {
      cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
  };

  client.start();
}

void echo_test_tcp_nodelay() {
  WsServer server(8080, 4);
  simpleEchoServer(server);
  std::thread server_thread([&server](){
     //Start WS-server
     server.start();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  simpleEchoClient(server);
  cout << "Client done" << endl;
  server.stop();
  server_thread.join();
}

INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);

  echo_test_tcp_nodelay();

  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();

  auto server = std::thread([&tunaTor]() { tunaTor.start(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  test_not_startable(tunaTor);
  test_wait_status(tunaTor);
  for (int i = 0; i < 100; ++i) {
    test_startable(tunaTor);
  }
  test_packets(tunaTor);

  tunaTor.stop();
  server.join();

  return 0;
}
