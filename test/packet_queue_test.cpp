
#include "../packet_queue.hpp"

#include <iostream>

using std::cerr;
using std::endl;

INITIALIZE_EASYLOGGINGPP
int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);
  PacketQueue pb(1313, 100, 10);
  for (size_t j = 0; j < 10; ++j) {
    cerr << "Run:" << j << endl;
    for (size_t i = 0; i < 100; ++i) {
      // cerr << "push-:" << i << endl;
      if (!pb.push([i](Packet *pkt) {
        if (pkt == 0) {
          cerr << "push failed" << endl;
          return -1;
        }
        //cerr << "push:" << i << endl;
        *((size_t *)(pkt->buf)) = i;
        return (int)sizeof(size_t);
      })) {
        cerr << "push return failed" << endl;
        return -1;
      }
    }
    for (size_t i = 0; i < 10; ++i) {
      if (pb.push([i](Packet *pkt) {
        cerr << "should never called" << pkt << endl;
        return -1;
      })) {
        cerr << "push return ok" << endl;
        return -1;
      }
    }
    auto sc = pb.getStatistic().getCurrent();
    if (sc.pushOk != 100) {
      cerr << "pushOk failed:" << sc.pushOk << endl;
      return -1;
    }
    for (size_t i = 0; i < 100; ++i) {
      //cerr << "Pop-:" << i << endl;
      pb.pop([i](Packet *pkt) {
        //cerr << "pop:" << i << ":" << (size_t)pkt << endl;
        if (pkt == 0) {
          cerr << "pop failed" << endl;
          return -1;
        }
        if (pkt->size != sizeof(size_t)) {
          cerr << "pop size failed" << endl;
          return -1;
        }
        auto val = *((size_t *)(pkt->buf));
        if (val != i) {
          cerr << "pop buf failed:" << val << endl;
          return -1;
        }
        return (int)sizeof(size_t);
      });
    }
    //cerr << "START - over pop" << endl;
    for (size_t i = 0; i < 10; ++i) {
      //cerr << "over pop:" << i << endl;
      if (pb.pop([i](Packet *) {
        //cerr << "should never called" << pkt << endl;
        return -1;
      })) {
        //cerr << "pop return ok" << endl;
        return -1;
      }
    }
    //cerr << "DONE - over pop" << endl;
    auto s = pb.getStatistic().getCurrent();
    if (s.popEmpty != 10) {
      cerr << "popEmpty failed:" << s.popEmpty << endl;
      return -1;
    }
    if (s.popActionFailed != 0) {
      cerr << "popActionFailed failed:" << s.popActionFailed << endl;
      return -1;
    }
    if (s.popOk != 100) {
      cerr << "popOk failed:" << s.popOk << endl;
      return -1;
    }
  }
  auto s = pb.getStatistic().getTotal();
  if (s.pushOk != 1000) {
    cerr << "pushOk failed:" << s.pushOk << endl;
    return -1;
  }
  if (s.pushTimeout != 100) {
    cerr << "pushTimeout failed:" << s.pushTimeout << endl;
    return -1;
  }
  if (s.popTimeout != 100) {
    cerr << "popTimeout failed:" << s.popTimeout << endl;
    return -1;
  }
  if (s.popOk != 1000) {
    cerr << "popOk failed:" << s.popOk << endl;
    return -1;
  }

}
