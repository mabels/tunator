
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
    auto header = TunaServerHelper::getHeader<WsClient>(message);
    if (header != "JSON") {
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
    auto header = TunaServerHelper::getHeader<WsClient>(message);
    if (header != "JSON") {
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
    auto header = TunaServerHelper::getHeader<WsClient>(message);
    if (header != "JSON") {
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
    auto header = TunaServerHelper::getHeader<WsClient>(message);
    if (header == "PAKT") {
      std::stringstream packetIdStr;
      packetIdStr << packetId - 1;
      char buffer[message->size()+1];
      message->rdbuf()->sgetn(buffer,message->size());
      buffer[message->size()] = 0;
      std::string packet(buffer);
      if (packet.substr(0,packetIdStr.str().length()) != packetIdStr.str() ||
          packet.substr(packet.length()-packetIdStr.str().length()) != packetIdStr.str()) {

        LOG(ERROR) << "packetId missmatch:" << packetIdStr.str() << "|" << packet;
        exit(-4);
      }
      sendPacket(client, packetId);
      if (packetId >= 1000) {
        LOG(INFO) << "transmitted packets:" << packetId;
        client.send_close(4711);
      }
      return;
    }
    if (header != "JSON") {
      LOG(ERROR) << "Reading HEADER failed:" << header;
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


INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);
  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();

  auto server = std::thread([&tunaTor]() { tunaTor.start(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  test_not_startable(tunaTor);
  test_wait_status(tunaTor);
  for (int i = 0; i < 13; ++i) {
    test_startable(tunaTor);
  }
  test_packets(tunaTor);

  tunaTor.stop();
  server.join();

  return 0;
}
