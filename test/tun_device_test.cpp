
#include "../tun_device.hpp"

#include <iostream>

using std::cerr;
using std::endl;

INITIALIZE_EASYLOGGINGPP
int main() {
  TunDevice tun(IfAddrs(), 1500, 100);

  for (int i = 0; i < 3; ++i) {
    std::thread t([&tun](){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      tun.stop();
    });

    if (tun.start() != true) {
      cerr << "tun didn't started" << endl;
      return 1;
    }
    tun.join();
    t.join();
  }

  /*
      if (pb.push([i](Packet *pkt) {
        cerr << "should never called" << pkt << endl;
        return -1;
      }
  */
  return 0;
}
