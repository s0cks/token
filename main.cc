#include <sstream>
#include <string>
#include <service/service.h>
#include "blockchain.h"
#include <leveldb/db.h>
#include <glog/logging.h>
#include <sys/stat.h>

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

static inline bool
InitializeLogging(char* arg0, const std::string& path){
    std::string logdir = (path + "/logs/");
    if(!FileExists(logdir)){
        int rc;
        if((rc = mkdir(logdir.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) == -1){
            std::cerr << "Couldn't initialize logging directory '" << logdir << "'..." << std::endl;
            return false;
        }
    }
    google::SetLogDestination(google::INFO, logdir.c_str());
    google::SetLogDestination(google::WARNING, logdir.c_str());
    google::SetLogDestination(google::ERROR, logdir.c_str());
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

// <local file storage path>
// <local listening port>
// <peer port>
int main(int argc, char** argv){
    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string path(argv[1]);
    if(!FileExists(path)) {
        std::cout << "Root path '" << path << "' doesn't exist" << std::endl;
        return EXIT_FAILURE;
    }
    int port = atoi(argv[2]);

    int pport = -1;
    if(argc > 3){
        pport = atoi(argv[3]);
    }

    if(!InitializeLogging(argv[0], path)){
        return EXIT_FAILURE;
    }

    if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool((path + "/unclaimed.db"))){
        LOG(ERROR) << "Cannot load unclaimed transaction pool from path '" << (path + "/unclaimed.db") << "'";
        return EXIT_FAILURE;
    }
    UnclaimedTransactionPoolPrinter::Print();

    if(!BlockChain::Initialize(path)){
        return EXIT_FAILURE;
    }

    Block* block = BlockChain::GetInstance()->CreateBlock();
    Block* genesis = BlockChain::GetInstance()->GetGenesis();
    Transaction* tx = block->CreateTransaction();
    tx->AddInput(genesis->GetCoinbaseTransaction()->GetHash(), 12);
    tx->AddOutput("TestToken0", "TestUser2");
    if(!BlockChain::GetInstance()->Append(block)){
        LOG(WARNING) << "Couldn't append new block";
        return EXIT_FAILURE;
    }

    /*
    BlockChainService::GetInstance()->Start("127.0.0.1", port + 1);
    BlockChain::GetServerInstance()->AddPeer("127.0.0.1", pport);
    BlockChain::GetInstance()->StartServer(port);
    while(BlockChain::GetServerInstance()->IsRunning());
    BlockChain::GetInstance()->WaitForServerShutdown();
    BlockChainService::GetInstance()->WaitForShutdown();
    */
    return EXIT_SUCCESS;
}