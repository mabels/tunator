#include "packet.hpp"

#include <stdlib.h>
#include <memory>

class PacketBuffer {
private:
  const size_t size;
  const size_t mtu;
  const size_t packetSize;
  const std::unique_ptr<Packet> packets;
  const std::unique_ptr<char> buffer;
public:
  PacketBuffer(size_t mtu, size_t size) :
    size(size),
    mtu(mtu),
    packetSize(((mtu/sizeof(size_t))+1)*sizeof(size_t)),
    packets(std::unique_ptr<Packet>(new Packet[size])),
    buffer(std::unique_ptr<char>(new char[packetSize * size])) {
    for(size_t i = 0; i < size; ++i) {
      auto &packet = packets.get()[i];
      packet.used = false;
      packet.idx = i;
      packet.size = 0;
      packet.maxSize = mtu;
      packet.buf = &(buffer.get()[i*packetSize]);
    }
  }

  size_t getPacketSize() const { return packetSize; }
  size_t getMtu() const { return mtu; }
  size_t getSize() const { return size; }

  Packet* alloc() const {
    for(size_t i = 0; i < size; ++i) {
      auto &packet = packets.get()[i];
      if (packet.used) {
        continue;
      }
      packet.used = true;
      return &packet;
    }
    return 0;
  }

};
