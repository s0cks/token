#include <sstream>
#include <string>
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

    std::stringstream genblk;
    genblk << path << "/blk0.dat";
    if(!FileExists(genblk.str())){
        Block* nblock = new Block(true);
        Transaction* ntx = nblock->CreateTransaction();
        for(int i = 0; i < 128; i++){
            std::stringstream tk_name;
            tk_name << "Token" << i;
            ntx->AddOutput("TestUser", tk_name.str());
        }
        if(!BlockChain::Initialize(path, nblock)){
            std::cerr << "Cannot initialize block chain @ " << path << std::endl;
            return EXIT_FAILURE;
        }
    } else{
        BlockChain::GetInstance()->Load(path);
    }

    std::cout << (*BlockChain::GetInstance()->GetHead()) << std::endl;

    std::vector<UnclaimedTransaction*> utxos;
    if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(utxos)){
        std::cerr << "Couldn't get unclaimed transactions" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Unclaimed Transactions:" << std::endl;
    for(auto& it : utxos){
        std::cout << (*it) << std::endl;
    }

    Block* nblock = BlockChain::GetInstance()->CreateBlock();
    Transaction* ntx = nblock->CreateTransaction();
    ntx->AddInput(utxos[10]->GetTransactionHash(), utxos[10]->GetIndex());
    ntx->AddOutput("TestUser2", utxos[10]->GetToken());

    if(!BlockChain::GetInstance()->Append(nblock)){
        std::cerr << "Couldn't append block: " << std::endl;
        std::cerr << (*nblock) << std::endl;
        return EXIT_FAILURE;
    }

    if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(utxos)){
        std::cerr << "Couldn't get unclaimed transactions" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Unclaimed Transactions:" << std::endl;
    for(auto& it : utxos){
        std::cout << (*it) << std::endl;
    }
    return EXIT_SUCCESS;
}