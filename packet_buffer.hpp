#include "packet.hpp"

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
      auto packet = packets.ptr()[i];
      packet->used = false;
      packets->idx = i;
      packets->size = 0;
      packets->max_size = mtu;
      packets->buf = buffer.ptr[i*packetSize];
    }
  }

  Packet* alloc() const {
    for(size_t i = 0; i < size; ++i) {
      auto packet = packets.ptr()[i];
      if (packet.used) {
        continue;
      }
      packet->used = true;
      return packet;
    }
    return 0;
  }

}
