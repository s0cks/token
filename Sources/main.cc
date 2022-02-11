#include <glog/logging.h>

int main(int argc, char** argv){


  google::InitGoogleLogging(argv[0]);
#ifdef TOKEN_DEBUG
  google::LogToStderr();
#endif//TOKEN_DEBUG

  DLOG(INFO) << "Hello World";
  return 0;
}