#ifndef TOKEN_TX_VALIDATOR_H
#define TOKEN_TX_VALIDATOR_H

#include "transaction.h"
#include "utxo.h"

namespace Token{
    class TransactionValidator{
    public:
        bool IsValid(Transaction* tx){
            if((!tx->IsCoinbase() && tx->GetNumberOfInputs() == 0) || tx->GetNumberOfOutputs() == 0) return false;
            for(int i = 0; i < tx->GetNumberOfInputs(); i++){
                Input* in = tx->GetInputAt(i);
                UnclaimedTransaction utxo;
                if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(in->GetPreviousHash(), in->GetIndex(), &utxo)){
                    return false;
                }

            }
        }

        bool ValidateTransactions(std::vector<Transaction*> possible, std::vector<Transaction*>& valid, std::vector<Transaction*>& invalid){
            std::vector<Transaction*> work;
            int num_valid = 0;
            for(auto& it : possible){
                if(IsValid(it)){
                    for(int i = 0; i < it->GetNumberOfOutputs(); i++){
                        UnclaimedTransaction* utxo = new UnclaimedTransaction(it->GetHash(), i);
                        UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(utxo);
                    }
                    for(int i = 0; i < it->GetNumberOfInputs(); i++){
                        Input* in = it->GetInputAt(i);
                        UnclaimedTransaction* utxo = new UnclaimedTransaction(in->GetPreviousHash(), in->GetIndex());
                    }
                    valid.push_back(it);
                } else{
                    work.push_back(it);
                }
            }
            return true;
        }
    };
}

#endif //TOKEN_TX_VALIDATOR_H