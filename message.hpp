#ifndef __Message__
#define __Message__

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <arpa/inet.h>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <json/json.h>


template<typename T>
class MessageRef {
private:
  std::string action;
  T &data;

public:
  MessageRef(std::string const &action, T& data) : action(action), data(data) {
  }

  const std::string& getAction() const {
    return action;
  }

  T& getData() {
    return data;
  }

  void asJson(Json::Value &val) const {
    val["action"] = action;
    Json::Value jsData;
    data.asJson(jsData);
    val["data"] = jsData;
  }

  static bool fromJson(Json::Value &val, MessageRef<T> &msg) {
    msg.action = val.get("action", "").asString();
    T::fromJson(val["data"], msg.data);
    return true;
  }
};

template<typename T>
class Message : public MessageRef<T> {
  private:
    T realData;
  public:
    Message(std::string const &action) : MessageRef<T>(action, realData) {}
};

#endif
