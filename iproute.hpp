#ifndef __IPROUTE__
#define __IPROUTE__

#include <ipaddress/ipaddress.hpp>

using ipaddress::IPAddress;
using ipaddress::Option;

class IPRoute {
  public:
  IPAddress dest;
  Option<IPAddress> via;
  Option<std::string> dev;
  Option<std::string> proto;
  Option<std::string> scope;
  Option<IPAddress> src;
  Option<size_t> metric;
};

#endif
