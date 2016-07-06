#include "../packet_statistic.hpp"

#include <iostream>
#include <cmath>
#include <thread>

using std::cerr;
using std::endl;
using std::chrono::system_clock;

int main() {
  PacketStatistic ps;
  auto started = ps.getStarted();
  std::chrono::duration<float> difference = started - system_clock::now();
  if (std::abs(difference.count()) > 50) {
    cerr << "getStarted is not correct" << endl;
    return 1;
  }
  ps.incAllocOk();
  if (ps.getCurrent().allocOk != 1) {
    cerr << "1-incAllocOk current failed" << endl;
    return 1;
  }
  if (ps.getTotal().allocOk != 0) {
    cerr << "1-incAllocOk total failed" << endl;
    return 1;
  }
  auto collected = ps.collect();
  if (started != collected.getStarted()) {
    cerr << "1-collected started failed" << endl;
    return 1;
  }
  started = collected.getStarted();
  if (ps.getCurrent().allocOk != 0) {
    cerr << "collected current failed" << endl;
    return 1;
  }
  if (ps.getTotal().allocOk != 1) {
    cerr << "collected total failed" << endl;
    return 1;
  }
  ps.incAllocOk();
  if (ps.getCurrent().allocOk != 1) {
    cerr << "2-incAllocOk current failed" << endl;
    return 1;
  }
  if (ps.getTotal().allocOk != 1) {
    cerr << "2-incAllocOk total failed" << endl;
    return 1;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  collected = ps.collect();
  difference = started - collected.getStarted();
  if (difference.count() > 50) {
    cerr << "2-collected started failed" << endl;
    return 1;
  }
  if (ps.getCurrent().allocOk != 0) {
    cerr << "3-incAllocOk current failed" << endl;
    return 1;
  }
  if (ps.getTotal().allocOk != 2) {
    cerr << "3-incAllocOk total failed" << endl;
    return 1;
  }
  return 0;
}
