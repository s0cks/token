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
    using namespace Token;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if(!InitializeLogging(argv[0])){
        std::cerr << "couldn't initialize logging";
        return EXIT_FAILURE;
    }

    if(!TransactionPool::Initialize()){
        LOG(ERROR) << "couldn't initialize transaction pool";
        return EXIT_FAILURE;
    }

    Token::Transaction* tx1 = new Token::Transaction();
    if(!TransactionPool::PutTransaction(tx1)){
        LOG(ERROR) << "couldn't put transaction into transaction pool";
        return EXIT_FAILURE;
    }

    Token::Transaction* tx2;
    if(!(tx2 = TransactionPool::GetTransaction(tx1->GetHash()))){
        LOG(ERROR) << "couldn't retrieve transaction from transaction pool";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Tx1 Hash := " << tx1->GetHash();
    LOG(INFO) << "Tx2 Hash := " << tx2->GetHash();
    return EXIT_SUCCESS;
}