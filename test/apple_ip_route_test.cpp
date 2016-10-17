#include <cascara/cascara.hpp>
using namespace cascara;

#include "../apple_ip_route.hpp"

int main() {
  describe("apple-parse", []() {
      it("parse-apple-map", []() {
        std::stringstream s2;
        s2 <<
        "Routing tables" << std::endl <<
        "" << std::endl << 
        "Internet:" << std::endl << 
        "Destination        Gateway            Flags        Refs      Use   Netif Expire" << std::endl << 
        "default            192.168.189.1      UGSc          239        9     en0" << std::endl << 
        "default            10.24.1.254        UGScI           2        0 bridge0" << std::endl << 
        "10.24.1/24         link#9             UC              6        0 bridge0" << std::endl << 
        "10.24.1.155        link#9             UHLWIi          1     2746 bridge0" << std::endl << 
        "10.24.1.176        8.0.27.0.bf.f5     UHLWIi          4     3890 bridge0    389" << std::endl << 
        "10.24.1.254        link#9             UHLWIir         3        0 bridge0" << std::endl << 
        "10.24.1.255        link#9             UHLWbI          1        8 bridge0" << std::endl << 
        "127                127.0.0.1          UCS             1        0     lo0" << std::endl << 
        "127.0.0.1          127.0.0.1          UH              4   712570     lo0" << std::endl << 
        "169.254            link#4             UCS             1        0     en0" << std::endl << 
        "192.168.189        link#4             UCS             3        0     en0" << std::endl << 
        "192.168.189.1/32   link#4             UCS             2        0     en0" << std::endl << 
        "192.168.189.1      3c:46:d8:2c:4a:c2  UHLWIir       240     1768     en0   1179" << std::endl << 
        "192.168.189.142    d4:40:f0:31:96:3e  UHLWIi          1        0     en0   1183" << std::endl << 
        "192.168.189.180/32 link#4             UCS             1        0     en0" << std::endl << 
        "192.168.189.255    link#4             UHLWbI          1       64     en0" << std::endl << 
        "224.0.0            link#4             UmCS            2        0     en0" << std::endl << 
        "224.0.0.251        1:0:5e:0:0:fb      UHmLWI          1        0     en0" << std::endl << 
        "255.255.255.255/32 link#4             UCS             1        0     en0" << std::endl;
        auto iproute = AppleIpRoute::parse(s2.str());
        assert.isFalse(iproute.isErr());
        assert.equal(s2.str(), AppleIpRoute::serialize(iproute.unwrap()));
    });
    it("ip route add", []() {
        std::stringstream s2;
        s2 <<
          "default via 172.31.1.1 dev eth0" << std::endl <<
          "169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1" << std::endl;
        auto iproute = AppleIpRoute::parse(s2.str()).unwrap();
        assert.deepEqual(AppleIpRoute::add(iproute),
            {"/sbin/ip route add default via 172.31.1.1 dev eth0",
             "/sbin/ip route add 169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1"});
    });
    it("ip route del", []() {
        std::stringstream s2;
        s2 <<
          "default via 172.31.1.1 dev eth0" << std::endl <<
          "169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1" << std::endl;
        auto iproute = AppleIpRoute::parse(s2.str()).unwrap();
        assert.deepEqual(AppleIpRoute::del(iproute),
            {"/sbin/ip route del default via 172.31.1.1 dev eth0",
             "/sbin/ip route del 169.254.12.0/24 dev br12 proto kernel scope link src 169.254.12.1"});
    });
  });
  exit();
}
