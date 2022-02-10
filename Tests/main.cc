#include <gtest/gtest.h>
#include <glog/logging.h>

int main(int argc, char** argv){
  using namespace ::testing;
  using namespace google;

  ParseCommandLineFlags(&argc, &argv, false);

#ifdef TOKEN_DEBUG
  InitGoogleLogging(argv[0]);
  LogToStderr();
#endif//TOKEN_DEBUG

  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}