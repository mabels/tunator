#include <chrono>

#include "chai.hpp"
#include "mocha.hpp"

#include "../system_cmd.hpp"

#ifdef __APPLE_CC__
#define EXEC_TRUE "/usr/bin/true"
#define EXEC_FALSE "/usr/bin/false"
#define EXEC_ECHO "/bin/echo"
#define EXEC_SLEEP "/bin/sleep"
#define EXEC_GREP "/usr/bin/grep"
#else
#define EXEC_TRUE "/bin/true"
#define EXEC_FALSE "/bin/false"
#define EXEC_ECHO "/bin/echo"
#define EXEC_SLEEP "/bin/sleep"
#define EXEC_GREP "/bin/grep"
#endif

INITIALIZE_EASYLOGGINGPP
int main() {
  describe("SystemCmd", []() {
    // it("simple echo", []() {
    //     Chai::assert.equal(0, SystemCmd("/bin/sleep").arg("60").run().exitCode);
    // });
    it("not launched", []() {
        Chai::assert.isFalse(SystemCmd("WTF").run().ok);
    });
    it("syncron sleep", []() {
      auto start = std::chrono::high_resolution_clock::now(); //measure time starting here
      Chai::assert.isTrue(SystemCmd(EXEC_SLEEP).arg("1").run().ok, "sleep should be ok");
      auto end = std::chrono::high_resolution_clock::now(); //end measurement here
      auto elapsed = end - start;

      Chai::assert.isTrue(std::chrono::microseconds{1000000} <= elapsed, "time is not ok");
    });
    it("launched", []() {
        Chai::assert.isTrue(SystemCmd(EXEC_TRUE).run().ok);
    });
    it("return ok", []() {
        Chai::assert.equal(0, SystemCmd(EXEC_TRUE).run().exitCode);
    });
    it("return false", []() {
        Chai::assert.equal(1, SystemCmd(EXEC_FALSE).run().exitCode);
    });
    it("stdout empty", []() {
        Chai::assert.isTrue(SystemCmd(EXEC_TRUE).run().getSout().str().empty());
    });
    it("stderr empty", []() {
        Chai::assert.isTrue(SystemCmd(EXEC_TRUE).run().getSerr().str().empty());
    });

    it("stdout hello world", []() {
        Chai::assert.equal(SystemCmd(EXEC_ECHO).arg("hello world").run().getSout().str(), "hello world\n");
        Chai::assert.equal(SystemCmd(EXEC_ECHO).arg("hello world").run().getSerr().str(), "");
    });
    it("stderr output", []() {
        Chai::assert.isTrue(SystemCmd(EXEC_GREP).arg("---Fehler").run().getSout().str().empty());
        Chai::assert.isFalse(SystemCmd(EXEC_GREP).arg("---Fehler").run().getSerr().str().empty());
    });
  });
  exit();
}
