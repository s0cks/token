#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include "token.h"

static inline bool
InitializeLogging(char* arg0){
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

int
main(int argc, char** argv){
    if(!InitializeLogging(argv[0])){
        std::cerr << "couldn't initialize logging";
        return EXIT_FAILURE;
    }

    testing::InitGoogleTest(&argc, argv);

    std::cout << Token::GetVersion() << std::endl;
    return RUN_ALL_TESTS();
}