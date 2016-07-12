
#include <client_ws.hpp>

#include <tunator.hpp>
#include <message.hpp>
#include <if_addrs.hpp>

#include <iostream>
#include <string>
#include <thread>



using std::cout;
using std::cerr;
using std::endl;
using std::string;

INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);
  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();

  auto server = std::thread([&tunaTor]() { tunaTor.start(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;
  WsClient client("127.0.0.1:4711/init");
  client.onmessage = [&client]( std::shared_ptr<WsClient::Message> message) {
    auto message_str = message->string();
    cout << "Client: Message received: \"" << message_str << "\"" << endl;
    cout << "Client: Sending close connection" << endl;
    client.send_close(1000);
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
  client.onclose = [](int status, const string & /*reason*/) {
    cout << "Client: Closed connection with status code " << status << endl;
  };
  client.onerror = [](const boost::system::error_code &ec) {
    cout << "Client: Error: " << ec << ", error message: " << ec.message()
         << endl;
  };
  cout << "client.start" << endl;
  client.start();
  cout << "done client.start" << endl;

  server.join();

  return 0;
}
