#include "blockchain.h"
#include "service/client.h"

static inline void
PrintBlock(Token::Messages::BlockHeader* block){
    std::cout << "-- Block # " << block->height() << " ---" << std::endl;
    std::cout << "\tPrevious Hash: " << block->previous_hash() << std::endl;
    std::cout << "\tHash: " << block->hash() << std::endl;
    std::cout << "\tMerkle Root: " << block->merkle_root() << std::endl;
    std::cout << "-----------" << std::endl;
}

static inline void
PrintTransaction(Token::Transaction* tx){
    std::cout << "\t+ #" << tx->GetIndex() << "(" << tx->GetHash() << ")" << std::endl;
    int idx;
    std::cout << "\t\tInputs:" << std::endl;
    for(idx = 0; idx < tx->GetNumberOfInputs(); idx++){
        Token::Input* input = tx->GetInputAt(idx);
        std::cout << "\t\t  * #" << idx << ": " << (*input) << std::endl;
    }
    std::cout << "\t\tOutputs:" << std::endl;
    for(idx = 0; idx < tx->GetNumberOfOutputs(); idx++){
        Token::Output* output = tx->GetOutputAt(idx);
        std::cout << "\t\t  * #" << idx << ": " << (*output) << std::endl;
    }
}

int main(int argc, char** argv){
    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string host(argv[1]);

    std::cout << "Connecting to: " << host << ":" << 5051 << std::endl;
    TokenServiceClient client(host, 5051);

    Messages::BlockHeader block;
    if(!client.GetHead(&block)){
        std::cerr << "Cannot fetch <HEAD>!" << std::endl;
        return EXIT_FAILURE;
    }
    PrintBlock(&block);

    Transaction* ocbtx;
    if(!client.GetTransaction(block.hash(), 0, &ocbtx)){
        std::cerr << "Couldn't fetch genesis's coinbase transaction";
        return EXIT_FAILURE;
    }

    Block* nblock = new Block(block.hash(), block.height() + 1);
    Transaction* ncbtx = nblock->CreateTransaction();
    ncbtx->AddInput(ocbtx->GetHash(), 0);
    ncbtx->AddOutput("TestToken", "TestUser");
    if(!client.Append(nblock, &block)){
        std::cerr << "Couldn't append new block" << std::endl;
        return EXIT_FAILURE;
    }
    PrintBlock(&block);
    return EXIT_SUCCESS;
}