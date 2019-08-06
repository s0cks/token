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
    TokenServiceClient client(host, 8080);

    std::string input;
    do{
        std::cin >> input;
        if(input == "discon"){
            return EXIT_FAILURE;
        } else if(input == "append"){
            Messages::BlockHeader head;
            client.GetHead(&head);

            Block* nblock = new Block(head.previous_hash(), head.height() + 1);
            std::cout << "Create Transactions (y/n)?:";
            std::cin >> input;
            if(input == "y" || input == "Y"){
                do{
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
                    tx->AddOutput(user, token);

                    bool another;
                    std::cout << "Add Another Transaction (y/n)?:";
                    std::cin >> another;
                    if(!another){
                        break;
                    }
                } while(true);
            }

            if(!client.Append(nblock, &head)){
                std::cerr << "Cannot append new block" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << "Appended!" << std::endl;
            std::cout << "New <HEAD>:" << std::endl;
            PrintBlock(&head);
        } else if(input == "gethead"){
            Messages::BlockHeader head;
            client.GetHead(&head);
            PrintBlock(&head);
        } else if(input == "lsutxo"){
            std::string target;
            std::cout << "User [Default is empty]?:";
            std::cin >> target;

            if(target == "None"){
                /*
                UnclaimedTransactionPool utxos;
                if(!client.GetUnclaimedTransactions(&utxos)){
                    std::cerr << "Couldn't get unclaimed transactions" << std::endl;
                    return EXIT_FAILURE;
                }
                std::cout << "Unclaimed Transactions: " << std::endl;
                std::cout << (utxos);
                 */
            } else{
                /*
                UnclaimedTransactionPool utxos;
                if(!client.GetUnclaimedTransactions(target, &utxos)){
                    std::cerr << "Couldn't get unclaimed transactions" << std::endl;
                    return EXIT_FAILURE;
                }
                std::cout << "Unclaimed Transactions for " << target << ": " << std::endl;
                std::cout << (utxos);
                 */
            }
        } else if(input == "save"){
            if(!client.Save()){
                std::cerr << "Couldn't shutdown" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << "Shutting down" << std::endl;
        }
    } while(true);
}