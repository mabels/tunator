
#include <stdlib.h>

class Packet {
public:
  bool used;
  size_t idx;
  size_t size;
  size_t maxSize;
  void *buf;
  void free() {
    used = false;
  }
};
