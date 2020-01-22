#include <glog/logging.h>
#include <gtest/gtest.h>

#include "transaction.h"

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

    Token::Output out1("TestUser", "TestToken1");
    Token::Output out2("TestUser", "TestToken1");
    Token::Output out3("TestUser", "TestToken2");

    LOG(INFO) << "Output #1 Hash: " << out1.GetHash();
    LOG(INFO) << "Output #2 Hash: " << out2.GetHash();
    LOG(INFO) << "Output #3 Hash: " << out3.GetHash();

    if(out1 != out2){
        LOG(ERROR) << "outputs are not equal";
        return EXIT_FAILURE;
    }

    if(out1 == out3){
        LOG(ERROR) << "outputs have invalid hash";
        return EXIT_FAILURE;
    }


    Token::Transaction tx1(0);
    for(size_t idx = 0; idx < 128; idx++){
        std::stringstream token;
        token << "TestToken" << idx;
        tx1 << Token::Output("TestUser", token.str());
    }

    Token::Transaction tx2(1);
    for(size_t idx = 0; idx < 128; idx++){
        std::stringstream token;
        token << "TestToken" << idx;
        tx2 << Token::Output("TestUser", token.str());
    }

    Token::Transaction tx3(0);
    for(size_t idx = 0; idx < 128; idx++){
        std::stringstream token;
        token << "TestToken" << idx;
        tx3 << Token::Output("TestUser", token.str());
    }

    LOG(INFO) << "Transaction #1 Hash: " << tx1.GetHash();
    LOG(INFO) << "Transaction #2 Hash: " << tx2.GetHash();
    LOG(INFO) << "Transaction #3 Hash: " << tx3.GetHash();

    if(tx1 == tx2){
        LOG(ERROR) << "transactions have invalid hash";
        return EXIT_FAILURE;
    }

    if(tx1 != tx3){
        LOG(ERROR) << "transactions have invalid hash";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}