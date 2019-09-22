#include <glog/logging.h>
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
PrintUnclaimedTransaction(Token::Messages::UnclaimedTransaction* utxo){
    std::cout << "Unclaimed Transaction from " << utxo->tx_hash() << "[" << utxo->index() << "] => " << utxo->user() << "(" << utxo->token() << ")" << std::endl;
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
    int port = atoi(argv[2]);

    std::cout << "Connecting to: " << host << ":" << port << std::endl;
    TokenServiceClient client(host, port);

    std::string input;
    do{
        std::cout << "?> ";
        input.clear();
        std::cin >> input;
        if(input == "discon"){
            return EXIT_FAILURE;
        } else if(input == "append"){
            Messages::BlockHeader head;
            client.GetHead(&head);

            std::cout << "Previous Hash: " << head.hash() << std::endl;

            /*
             * TODO: Refactor
            Block* nblock = new Block(head.height() + 1, head.hash());
            std::cout << "Create Transactions (y/n)?:";
            std::string prompt;
            std::cin >> prompt;
            if(prompt == "y" || prompt == "Y"){
                query:
                    std::string hash;
                    std::string user;
                    std::string token;
                    int index;

                    std::cout << "Create Transaction: " << std::endl;
                    std::cout << "Transaction Hash: ";
                    std::cin >> hash;
                    std::cout << "Output Index: ";
                    std::cin >> index;
                    std::cout << "User: ";
                    std::cin >> user;
                    std::cout << "Token: ";
                    std::cin >> token;

                    std::cout << "Creating Transaction...." << std::endl;

                    Transaction* tx = nblock->CreateTransaction();
                    tx->AddInput(hash, index);
                    tx->AddOutput(token, user);

                    std::string another;
                    std::cout << "Add Another Transaction (y/n)?:";
                    std::cin >> another;
                    if(another == "Y" || another == "y"){
                        goto query;
                    }
            }

            Token::Messages::BlockHeader nhead;
            if(!client.Append(nblock, &nhead)){
                std::cerr << "Cannot append new block" << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "Appended!" << std::endl;
            std::cout << "New <HEAD>:" << std::endl;
            PrintBlock(&nhead);
            */
        } else if(input == "gethead"){
            Messages::BlockHeader head;
            if(!client.GetHead(&head)){
                LOG(ERROR) << "couldn't get <HEAD> from service";
                return EXIT_FAILURE;
            }
            PrintBlock(&head);
        } else if(input == "lspeers"){
            std::vector<std::string> peers;
            if(!client.GetPeers(peers)){
                LOG(ERROR) << "couldn't get peers from service";
                return EXIT_FAILURE;
            }
            LOG(INFO) << "Peers:";
            for(auto& it : peers){
                LOG(INFO) << "  - " << it;
            }
        } else if(input == "getblock"){
            std::string target;
            std::cout << "target := ";
            std::cin >> target;
            if(target.length() == 64){
                LOG(INFO) << "fetching block from hash: " << target;
                Messages::BlockHeader blk;
                if(!client.GetBlock(target, &blk)){
                    LOG(ERROR) << "couldn't fetch block: " << target;
                    return EXIT_FAILURE;
                }
                PrintBlock(&blk);
            } else{
                LOG(INFO) << "fetching block @" << target;
                Messages::BlockHeader blk;
                if(!client.GetBlockAt(atoi(target.c_str()), &blk)){
                    LOG(ERROR) << "couldn't fetch block: " << target;
                    return EXIT_FAILURE;
                }
                PrintBlock(&blk);
            }
        }
    } while(true);
}

/*
int main(int argc, char** argv){
    if(argc < 2) return EXIT_FAILURE;
    std::string address(argv[1]);
    int port = atoi(argv[2]);
    uv_loop_t* loop = uv_default_loop();
    Token::PeerSession* peer = new Token::PeerSession(loop, address, port);
    Token::BlockChainServerShell shell(loop, peer);
    uv_run(loop, UV_RUN_DEFAULT);
    return EXIT_SUCCESS;
}
*/