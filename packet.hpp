

class Packet {
public:
  bool used;
  size_t idx;
  size_t size;
  size_t max_size;
  void *buf;
  void free() {
    used = false;
  }
}
