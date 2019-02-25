#include <sstream>
#include "token.h"
#include "block.h"
#include "common.h"
#include "merkle.h"

static inline void
PrintBlock(Token::Block* block, int block_idx){
    std::cout << "--- Block #" << block_idx << " --------" << std::endl;
    std::cout << "Genesis Transaction Hash := " << block->GetTransactionAt(0)->GetHash() << std::endl;
    std::cout << "Previous Block Hash := " << block->GetPreviousHash() << std::endl;
    std::cout << "Block Hash := " << block->GetHash() << std::endl;
    std::cout << std::endl;
}

static inline void
PrintHashes(std::vector<std::string> hashes){
    std::cout << "--- " << hashes.size() << " << Hashes: ----" << std::endl;
    int i;
    for(i = 0; i < hashes.size(); i++){
        std::cout << "\t" << (i + 1) << ": " << hashes[i] << std::endl;
    }
}

int
main(int argc, char** argv){
    std::cout << "Token := v" << Token::GetVersion() << std::endl;

    Token::Block block1("000000000000000000000000000000000000000000", "");
    Token::Transaction* b1tx1 = block1.CreateTransaction();
    b1tx1->AddOutput("Token#1");
    b1tx1->AddOutput("Token#2");
    b1tx1->AddOutput("Token#3");

    Token::Transaction* b1tx2 = block1.CreateTransaction();
    b1tx2->AddOutput("Token#4");
    b1tx2->AddOutput("Token#5");
    b1tx2->AddOutput("Token#6");

    Token::Transaction* b1tx3 = block1.CreateTransaction();
    b1tx3->AddOutput("Token#7");
    b1tx3->AddOutput("Token#8");
    b1tx3->AddOutput("Token#9");

    Token::Block block2("000000000000000000000000000000000000000000", "");
    Token::Transaction* b2tx1 = block2.CreateTransaction();
    b2tx1->AddOutput("Token#1");
    b2tx1->AddOutput("Token#2");
    b2tx1->AddOutput("Token#3");

    Token::Transaction* b2tx2 = block2.CreateTransaction();
    b2tx2->AddOutput("Token#4");
    b2tx2->AddOutput("Token#5");
    b2tx2->AddOutput("Token#6");

    Token::Transaction* b2tx3 = block2.CreateTransaction();
    b2tx3->AddOutput("Token#7");
    b2tx3->AddOutput("Token#8");
    b2tx3->AddOutput("Token#9");

    std::cout << "Block #1 - Merkle Root := " << block1.GetMerkleRoot() << std::endl;
    std::cout << "Block #1 - Hash := " << block1.GetHash() << std::endl;
    std::cout << "Block #2 - Merkle Root := " << block2.GetMerkleRoot() << std::endl;
    std::cout << "Block #2 - Hash := " << block2.GetHash() << std::endl;

    std::cout << "Writing Block #1" << std::endl;
    block1.Write(std::string(argv[1]));
    std::cout << "Writing Block #2" << std::endl;
    block2.Write(std::string(argv[2]));
    return 0;
}