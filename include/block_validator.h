#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include "block.h"
#include "utxo.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    private:
        std::vector<Transaction*> valid_txs_;
        std::vector<Transaction*> invalid_txs_;
    public:
        BlockValidator():
            valid_txs_(),
            invalid_txs_(){}
        ~BlockValidator(){}

        bool IsValid(Transaction* tx){
            if(tx->GetNumberOfInputs() == 0 || tx->GetNumberOfOutputs() == 0){
                std::cerr << "Inputs or outputs are 0" << std::endl;
                return false;
            }

            int i;
            for(i = 0; i < tx->GetNumberOfInputs(); i++){
                Input* in = tx->GetInputAt(i);
                UnclaimedTransaction* ut;
                if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(in->GetPreviousHash(), in->GetIndex(), &ut)){
                    std::cerr << "No unclaimed transaction for: " << in->GetPreviousHash();
                    return false;
                }
            }
            return true;
        }

        void VisitTransaction(Transaction* tx){
            if(IsValid(tx)){
                int i;
                for(i = 0; i < tx->GetNumberOfOutputs(); i++){
                    UnclaimedTransaction* utxo = new UnclaimedTransaction(tx->GetHash(), i, tx->GetOutputAt(i));
                    std::cout << "Inserting: " << (*utxo) << std::endl;
                    if(!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(utxo)){
                        std::cerr << "Cannot insert unclaimed transaction" << std::endl;
                    }
                }
                for(i = 0; i < tx->GetNumberOfInputs(); i++){
                    Input* in = tx->GetInputAt(i);
                    UnclaimedTransaction utxo(in->GetPreviousHash(), in->GetIndex());
                    std::cout << "removing " << (utxo) << std::endl;
                    if(!UnclaimedTransactionPool::GetInstance()->RemoveUnclaimedTransaction(&utxo)){
                        std::cerr << "Cannot remove unclaimed transaction" << std::endl;
                    }
                }
                valid_txs_.push_back(tx);
            } else{
                invalid_txs_.push_back(tx);
            }
        }

        std::vector<Transaction*> GetValidTransactions(){
            return valid_txs_;
        }

        std::vector<Transaction*> GetInvalidTransactions(){
            return invalid_txs_;
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H