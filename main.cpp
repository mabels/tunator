


#include "tunator.hpp"

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  auto tunaTor = TunaTor::args(argc, argv);
  tunaTor.dump();

  tunaTor.start();
  return 0;
}
