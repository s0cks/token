#include <glog/logging.h>
#include <gtest/gtest.h>
#include <pool.h>
#include "keychain.h"
#include "block_chain.h"

static inline bool
InitializeLogging(char* arg0){
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

int
main(int argc, char** argv){
    return EXIT_SUCCESS;
}