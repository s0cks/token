#include <gtest/gtest.h>
#include <glog/logging.h>

int main(int argc, char** argv){
  using namespace ::testing;
  using namespace google;

  InitGoogleTest(&argc, argv);
  InitGoogleLogging(argv[0]);

#ifdef TOKEN_DEBUG
  LogToStderr();
#endif//TOKEN_DEBUG

  ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}