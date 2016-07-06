#include "../packet_buffer.hpp"

#include <iostream>
#include <cmath>
#include <thread>

using std::cerr;
using std::endl;
using std::chrono::system_clock;

int main() {
  PacketBuffer pb(1313, 100);

  if (pb.getPacketSize() != 1320) {
    cerr << "getPacketSize total failed" << endl;
    return 1;
  }
  if (pb.getMtu() != 1313) {
    cerr << "getMtu total failed" << endl;
    return 1;
  }
  if (pb.getSize() != 100) {
    cerr << "getSize total failed" << endl;
    return 1;
  }
  Packet *packets[pb.getSize()];
  for (size_t i = 0; i < pb.getSize(); ++i) {
      packets[i] = pb.alloc();
      if (packets[i] == 0) {
        cerr << "alloc failed for " << i << endl;
        return 1;
      }
  }
  for (size_t i = 0; i < pb.getSize(); ++i) {
      if (pb.alloc() != 0) {
        cerr << "full alloc failed for " << i << endl;
        return 1;
      }
  }
  auto pkt = packets[pb.getSize()/2];
  pkt->free();
  if (pb.alloc() != pkt) {
        cerr << "new alloc failed" << endl;
  }

  for (size_t i = 0; i < pb.getSize(); ++i) {
    packets[i]->free();
    packets[i] = pb.alloc();
    if (packets[i] == 0) {
      cerr << "free alloc failed for " << i << endl;
      return 1;
    }
  }

  return 0;
}
