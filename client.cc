#include <glog/logging.h>
#include <gflags/gflags.h>
#include "block_chain.h"
#include "rpc/client.h"

static inline void
PrintBlock(Token::BlockHeader* block){
    std::cout << "-- Block # " << block->GetHeight() << " ---" << std::endl;
    std::cout << "\tPrevious Hash: " << block->GetPreviousHash() << std::endl;
    std::cout << "\tHash: " << block->GetHash() << std::endl;
    std::cout << "\tMerkle Root: " << block->GetMerkleRoot() << std::endl;
    std::cout << "-----------" << std::endl;
}

static inline void
PrintUnclaimedTransaction(Token::Proto::BlockChain::UnclaimedTransaction* utxo){
    std::cout << "Unclaimed Transaction from " << utxo->tx_hash() << "[" << utxo->tx_index() << "]" << std::endl;
}

static inline void
PrintTransaction(Token::Transaction* tx){
    std::cout << "\t+ #" << tx->GetIndex() << "(" << tx->GetHash() << ")" << std::endl;
    int idx;
    std::cout << "\t\tInputs:" << std::endl;
    for(idx = 0; idx < tx->GetNumberOfInputs(); idx++){
        Token::Input input;
        if(!tx->GetInput(idx, &input)) return;
        std::cout << "\t\t  * #" << idx << ": " << input.GetHash() << std::endl;
    }
    std::cout << "\t\tOutputs:" << std::endl;
    for(idx = 0; idx < tx->GetNumberOfOutputs(); idx++){
        Token::Output output;
        if(!tx->GetOutput(idx, &output)) return;
        std::cout << "\t\t  * #" << idx << ": " << output.GetHash() << std::endl;
    }
}

DEFINE_string(address, "", "The address of the node to connect to");

int main(int argc, char** argv){
    using namespace Token;
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    TokenServiceClient* client;
    if(FLAGS_port > 0){
        client = new TokenServiceClient(FLAGS_address, FLAGS_port);
    } else{
        client = new TokenServiceClient(FLAGS_address);
    }

    std::string input;
    do{
        std::cout << "?> ";
        input.clear();
        std::cin >> input;
        if(input == "discon"){
            return EXIT_FAILURE;
        } else if(input == "gethead"){
            BlockHeader head;
            if(!client->GetHead(&head)){
                LOG(ERROR) << "couldn't get <HEAD> from rpc";
                return EXIT_FAILURE;
            }
            PrintBlock(&head);
        } else if(input == "getblock"){
            std::string target;
            std::cout << "target := ";
            std::cin >> target;
            if(target.length() == 64){
                LOG(INFO) << "fetching block from hash: " << target;
                BlockHeader blk;
                if(!client->GetBlock(target, &blk)){
                    LOG(ERROR) << "couldn't fetch block: " << target;
                    return EXIT_FAILURE;
                }
                PrintBlock(&blk);
            }
            return EXIT_FAILURE;
        } else if(input == "getutxos"){
            std::string user;
            std::cout << "User? := ";
            std::cin >> user;

            Token::Proto::BlockChainService::UnclaimedTransactionList utxos;
            if(user == "None"){
                LOG(INFO) << "getting all unclaimed transactions";
                if(!client->GetUnclaimedTransactions(&utxos)){
                    LOG(ERROR) << "couldn't get all unclaimed transactions";
                    return EXIT_FAILURE;
                }
            } else{
                LOG(INFO) << "getting unclaimed transactions for: " << user;
                if(!client->GetUnclaimedTransactions(user, &utxos)){
                    LOG(ERROR) << "couldn't get unclaimed transactions for: " << user;
                    return EXIT_FAILURE;
                }
            }

            LOG(INFO) << "unclaimed transactions: ";
            size_t idx = 1;
            for(auto& it : utxos.utxos()){
                LOG(INFO) << " - #" << (idx++) << " " << it;
            }
        } else if(input == "spend"){
            std::string token;
            std::string user;

            std::cout << "User? := ";
            std::cin >> user;

            std::cout << "Token? := ";
            std::cin >> token;

            Transaction tx;
            uint256_t hash = HashFromHexString(token);
            if(!client->Spend(hash, user, &tx)){
                LOG(ERROR) << "couldn't spend: " << hash;
                return EXIT_FAILURE;
            }

            LOG(INFO) << "spent: " << tx.GetHash();
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