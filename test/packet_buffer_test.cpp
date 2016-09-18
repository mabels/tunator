#include "../packet_buffer.hpp"

#include <cmath>
#include <iostream>
#include <thread>

using std::cerr;
using std::endl;
using std::chrono::system_clock;

#include "chai.hpp"
#include "mocha.hpp"

int main() {
  describe("PacketBuffer", []() {
    PacketBuffer pb(1313, 100);

    it("packetSize 1320",
       [&pb]() { Chai::assert.equal(pb.getPacketSize(), 1320ul); });
    it("mtu 1313", [&pb]() { Chai::assert.equal(1313ul, pb.getMtu()); });
    it("size 100", [&pb]() { Chai::assert.equal(100ul, pb.getSize()); });

    Packet *packets[pb.getSize()];
    it("fill to 100", [&packets, &pb]() {
      for (size_t i = 0; i < pb.getSize(); ++i) {
        packets[i] = pb.alloc();
        Chai::assert.isFalse(packets[i] == 0);
      }
    });
    it("overfill to 100", [&packets, &pb]() {
      for (size_t i = 0; i < pb.getSize(); ++i) {
        Chai::assert.isTrue(pb.alloc() == 0);
      }
    });

    it("free to 50", [&packets, &pb]() {
      auto pkt = packets[pb.getSize() / 2];
      pkt->free();
      Chai::assert.isTrue(pb.alloc() == pkt);
    });

    it("free to all", [&packets, &pb]() {
      for (size_t i = 0; i < pb.getSize(); ++i) {
        packets[i]->free();
        packets[i] = pb.alloc();
        Chai::assert.isFalse(packets[i] == 0);
      }
    });
  });
  exit();
}
