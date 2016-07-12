
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <arpa/inet.h>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"

#include <json/json.h>

template<typename T>
class Message {
private:
  std::string action;
  T data;

public:
  Message(std::string const &action) : action(action) {
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

  static bool fromJson(Json::Value &val, Message<T> &msg) {
    msg.action = val["action"];
    T::fromJson(val["data"], msg.data);
    return true;
  }
};
