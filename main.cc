#include <sstream>
#include <string>
#include <fstream>
#include "blockchain.h"
#include "service/service.h"
#include "node/node.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
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
    BlockChain::GetInstance()->Load(path);

    std::string genhash = BlockChain::GetInstance()->GetHead()->GetCoinbaseTransaction()->GetHash();

    Block* nblock = new Block(BlockChain::GetInstance()->GetHead());
    Transaction* ntx = nblock->CreateTransaction();
    ntx->AddInput(genhash, 0);
    ntx->AddOutput("TestUser2", "TestToken0");
    ntx->AddInput(genhash, 1);
    ntx->AddOutput("TestUser2", "TestToken1");

    if(!BlockChain::GetInstance()->Append(nblock)){
        std::cerr << "Couldn't append new block" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Unclaimed Transactions:" << std::endl << (*BlockChain::GetInstance()->GetHeadUnclaimedTransactionPool()) << std::endl;

    BlockChainService::Start("127.0.0.1", port);
    BlockChainServer::GetInstance()->Initialize(port + 1);
    if(argc > 3){
        int pport = atoi(argv[3]);
        BlockChainServer::GetInstance()->ConnectTo("127.0.0.1", pport);
    }
    BlockChainServer::GetInstance()->Start();
    BlockChainService::WaitForShutdown();
    return EXIT_SUCCESS;
}