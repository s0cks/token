#include <gtest/gtest.h>
#include <glog/logging.h>

#include "token/token.h"

using namespace ::token;
using namespace ::google;
using namespace ::testing;

int main(int argc, char** argv){
  InitGoogleLogging(argv[0]);
#ifdef TOKEN_DEBUG
  LogToStderr();
#endif//TOKEN_DEBUG

  InitGoogleTest(&argc, argv);
  ParseCommandLineFlags(&argc, &argv, false);
  LOG(INFO) << "Running unit tests for token v" << GetVersion() << "....";
  return RUN_ALL_TESTS();
}