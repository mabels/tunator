#include "mocha.hpp"
#include "chai.hpp"
#include "../linux_ip_route.hpp"

INITIALIZE_EASYLOGGINGPP
int main() {
  describe("linux-parse", []() {
      it("parse-linux-map", []() {
        std::stringstream s2;
        s2 <<
            "default via 172.31.1.1 dev eth0" << std::endl <<
            "169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1" << std::endl <<
            "169.254.66.0/30 dev gt4rtwlmgt proto kernel scope link src 169.254.66.1" << std::endl <<
            "169.254.66.4/30 dev gt4rtwlmgt proto kernel scope link src 169.254.66.5" << std::endl <<
            "169.254.70.0/30 dev gt4servicedehgw proto kernel scope link src 169.254.70.1" << std::endl <<
            "169.254.70.4/30 dev gt4servicedehgw proto kernel scope link src 169.254.70.5" << std::endl <<
            "169.254.193.0/30 dev gt4scable1 proto kernel scope link src 169.254.193.1" << std::endl <<
            "169.254.194.0/30 dev gt4scable2 proto kernel scope link src 169.254.194.1" << std::endl <<
            "169.254.207.0/30 dev gt4rtabde proto kernel scope link src 169.254.207.1" << std::endl <<
            "169.254.207.4/30 dev gt4rtabde proto kernel scope link src 169.254.207.5" << std::endl <<
            "172.31.1.0/24 dev eth0 proto kernel scope link src 172.31.1.100" << std::endl <<
            "192.168.66.0/24 via 169.254.66.6 dev gt4rtwlmgt" << std::endl <<
            "192.168.70.0/24 via 169.254.70.6 dev gt4servicedehgw" << std::endl <<
            "192.168.207.0/24 via 169.254.207.6 dev gt4rtabde" << std::endl;
        auto iproute = LinuxIpRoute::parse(s2.str());
        Chai::assert.isFalse(iproute.isErr());
        Chai::assert.equal(s2.str(), LinuxIpRoute::serialize(iproute.unwrap()));
    });
    it("ip route add", []() {
        std::stringstream s2;
        s2 <<
          "default via 172.31.1.1 dev eth0" << std::endl <<
          "169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1" << std::endl;
        auto iproute = LinuxIpRoute::parse(s2.str()).unwrap();
        Chai::assert.deepEqual(LinuxIpRoute::add(iproute),
            {"/sbin/ip route add default via 172.31.1.1 dev eth0",
             "/sbin/ip route add 169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1"});
    });
    it("ip route del", []() {
        std::stringstream s2;
        s2 <<
          "default via 172.31.1.1 dev eth0" << std::endl <<
          "169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1" << std::endl;
        auto iproute = LinuxIpRoute::parse(s2.str()).unwrap();
        Chai::assert.deepEqual(LinuxIpRoute::del(iproute),
            {"/sbin/ip route del default via 172.31.1.1 dev eth0",
             "/sbin/ip route del 169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1"});
    });
  });
  exit();
}
