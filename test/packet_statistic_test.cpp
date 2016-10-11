#include "../packet_statistic.hpp"

#include <cmath>
#include <iostream>
#include <thread>

#include <ctime>
#include <chrono>
#include <iomanip>

using std::cerr;
using std::endl;
using std::chrono::system_clock;

#include "chai.hpp"
#include "mocha.hpp"

int main() {
  describe("PacketStatistic", []() {
    PacketStatistic ps;
    auto started = ps.getStarted();
    // std::chrono::duration<float> difference = started - system_clock::now();
    // it("started should take less than 50msec", [difference]() {
    //   auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(difference);
    //   Chai::assert.isTrue(milli.count() < 50);
    // });
    ps.incAllocOk();
    it("1-incAllocOk current", [&ps]() {
      Chai::assert.isTrue(ps.getCurrent().allocOk == 1);
    });
    it("1-incAllocOk total", [&ps]() {
      Chai::assert.equal(ps.getTotal().allocOk, 0u);
    });
    auto collected = ps.collect();
    started = collected.getStarted();
    //std::cout << "0-Pre=> ret=" <<
     // std::chrono::duration_cast<std::chrono::milliseconds>(started.time_since_epoch()).count()
      //        << std::endl;


    //std::cout << "1-started:" << started.rep << std::endl;
    it("1-collected started", [&ps, started, collected]() {
      Chai::assert.isTrue(started == collected.getStarted());
    });
    it("collected current", [&ps]() {
      Chai::assert.equal(ps.getCurrent().allocOk, 0u);
    });
    it("collected total", [&ps]() {
      Chai::assert.equal(ps.getTotal().allocOk, 1u);
    });
    ps.incAllocOk();
    it("2-incAllocOk current", [&ps]() {
      Chai::assert.equal(ps.getCurrent().allocOk, 1u);
    });
    it("2-incAllocOk total", [&ps]() {
      Chai::assert.equal(ps.getTotal().allocOk, 1u);
    });
    //std::cout << "Pre=> ret=" <<
     // std::chrono::duration_cast<std::chrono::milliseconds>(collected.getStarted().time_since_epoch()).count()
      //        << std::endl;

    //started = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    collected = ps.collect();
    //std::cout << "Post=> ret=" <<
     // std::chrono::duration_cast<std::chrono::milliseconds>(collected.getStarted().time_since_epoch()).count()
      //        << std::endl;
    //std::cout << "2-started:" << started.rep << std::endl;
    // auto difference =  collected.getStarted() - started;
    auto difference = std::chrono::system_clock::now() - started;
    it("2-collected started", [difference]() {
      auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(difference);
      //std::cout << "difference:" << milli.count() << std::endl;
      Chai::assert.isTrue(milli.count() >= 100);
    });
    it("3-incAllocOk current", [&ps]() {
      Chai::assert.equal(ps.getCurrent().allocOk, 0u);
    });
    it("3-incAllocOk total", [&ps]() {
      Chai::assert.isTrue(ps.getTotal().allocOk != 0);
    });
  });
  exit();
}
