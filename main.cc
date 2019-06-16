#include <sstream>
#include <string>
#include <fstream>
#include "blockchain.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

int main(int argc, char** argv){

    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string path(argv[1]);
    if(!FileExists(path)) {
        std::cout << "Root path '" << path << "' doesn't exist" << std::endl;
        return EXIT_FAILURE;
    }
    /**
    int port = atoi(argv[2]);
    if(argc > 3){
        int peer_port = atoi(argv[3]);
        BlockChain::GetInstance()->Load(path, "127.0.0.1", peer_port);
    } else{
        BlockChain::GetInstance()->Load(path);
        Block* head = BlockChain::GetInstance()->GetHead();
        Block* new_block = new Block(head);
        Transaction* new_tx_1 = new_block->CreateTransaction();
        new_tx_1->AddInput(head->GetCoinbaseTransaction()->GetHash(), 0);
        new_tx_1->AddOutput("TestUser", "TestToken");
        new_tx_1->AddOutput("TestUser", "TestToken");
        BlockChain::GetInstance()->Append(new_block);
    }
    BlockChain::GetServerInstance()->Initialize(port);
    **/
    Block* genesis = new Block();
    Transaction* ocbtx = genesis->CreateTransaction();
    ocbtx->AddOutput("TestUser", "TestToken");
    BlockChain::GetInstance()->SetHead(genesis);
    Block* nblock = new Block(genesis);
    Transaction* ncbtx = nblock->CreateTransaction();
    ncbtx->AddInput(ocbtx->GetHash(), 0);
    ncbtx->AddOutput("TestUser", "TestToken");
    if(!BlockChain::GetInstance()->Append(nblock)){
        std::cerr << "Cannot append new block" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}