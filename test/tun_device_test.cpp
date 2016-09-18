
#include "../tun_device.hpp"

#include <chrono>
#include <iostream>

#include <boost/thread.hpp>

#include "chai.hpp"
#include "mocha.hpp"

using std::cerr;
using std::endl;

struct S_pkt {
  bool end;
  size_t batch;
  bool endBatch;
  size_t pkt;
};

INITIALIZE_EASYLOGGINGPP
int main() {
  describe("TunDevice", []() {
    TunDevice tun(IfAddrs(), 1500, 100);

    it("try starting", [&tun]() {
      for (int i = 0; i < 3; ++i) {
        std::thread t([&tun]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
        Chai::assert.isTrue(tun.start());
        t.join();
        tun.stop();
      }
      Chai::assert.isTrue(tun.start());
    });

    boost::mutex mutex;
    mutex.lock();
    auto sendThread = std::thread([&mutex, &tun]() {
      for (size_t j = 0; j < 100; ++j) {
        // LOG(INFO) << "SendBatch:start:" << j;
        for (size_t i = 0; i < 47; ++i) {
          if (!tun.getToTun().push([i, j](Packet *pkt) {
                // cerr << "+";
                struct S_pkt buf = {j == 99, j, i == 46, i};
                if (buf.end && buf.endBatch) {
                  cerr << "SendBatch:END:" << buf.end << ":" << buf.endBatch
                       << endl;
                }
                *((struct S_pkt *)pkt->buf) = buf;
                return sizeof(buf);
              })) {
            cerr << "SendBatch:push:failed" << j << i << endl;
            return;
          }
        }
        // cerr << "SendBatch:lock:" << j << endl;
        mutex.lock();
      }
    });

    auto recvThread = std::thread([&mutex, &tun]() {
      struct S_pkt buf;
      do {
        // LOG(INFO) << "RecvBatch:start";
        do {
          bool ret = tun.getFromTun().pop([&buf](Packet *pkt) {
            // cerr << "-";
            buf = *((struct S_pkt *)pkt->buf);
            return 1;
          });
          if (!ret) {
            cerr << "recvThread:" << ret << ":" << buf.endBatch << ":"
                 << buf.end << endl;
          }
        } while (!buf.endBatch);
        // cerr << "RecvBatch:unlock" << endl;;
        mutex.unlock();
      } while (!buf.end);
    });

    // cerr << "-1-2" << endl;
    recvThread.join();
    sendThread.join();
    tun.stop();
    mutex.unlock();
    it("compare results", [&tun]() {
      auto fromTunPs = tun.getFromTun().getStatistic();
      // cerr << fromTunPs.asString();
      Chai::assert.equal(fromTunPs.getCurrent().pushOk, fromTunPs.getCurrent().popOk);
      auto toTunPs = tun.getToTun().getStatistic();
      // cerr << toTunPs.asString();
      Chai::assert.equal(toTunPs.getCurrent().pushOk, toTunPs.getCurrent().popOk);
    });
  });
  exit();
}
