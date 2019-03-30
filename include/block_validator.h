#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include "block.h"
#include "utxo.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    private:
        UnclaimedTransactionPool utxo_pool_;
        std::vector<Transaction*> valid_txs_;
        std::vector<Transaction*> invalid_txs_;
    public:
        BlockValidator(UnclaimedTransactionPool utxo_pool_):
            valid_txs_(),
            invalid_txs_(),
            utxo_pool_(utxo_pool_){}
        ~BlockValidator(){}

        bool IsValid(Transaction* tx){
            if(tx->GetNumberOfInputs() == 0 || tx->GetNumberOfOutputs() == 0){
                return false;
            }

            int i;
            for(i = 0; i < tx->GetNumberOfInputs(); i++){
                Input* in = tx->GetInputAt(i);
                UnclaimedTransaction ut(in->GetPreviousHash(), in->GetIndex());
                if(!utxo_pool_.Contains(ut)){
                    return false;
                }
                Output* out = utxo_pool_.Get(ut);
                if(out == nullptr) {
                    return false;
                }
            }

            for(i = 0; i < tx->GetNumberOfInputs() - 1; i++){
                Input* in = tx->GetInputAt(i);
                UnclaimedTransaction ut(in->GetPreviousHash(), in->GetIndex());
                for(int j = i + 1; j < tx->GetNumberOfInputs(); j++){
                    Input* next = tx->GetInputAt(j);
                    UnclaimedTransaction utx(next->GetPreviousHash(), next->GetIndex());
                    if(ut == utx){
                        return false;
                    }
                }
            }
            return true;
        }

        void VisitTransaction(Transaction* tx){
            if(IsValid(tx)){
                int i;
                for(i = 0; i < tx->GetNumberOfOutputs(); i++){
                    UnclaimedTransaction utx(tx->GetHash(), static_cast<uint32_t>(i));
                    utxo_pool_.Insert(utx, tx->GetOutputAt(i));
                }
                for(i = 0; i < tx->GetNumberOfInputs(); i++){
                    Input* in = tx->GetInputAt(i);
                    UnclaimedTransaction ut(in->GetPreviousHash(), in->GetIndex());
                    utxo_pool_.Remove(ut);
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

        UnclaimedTransactionPool GetUnclaimedTransactionPool(){
            return utxo_pool_;
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H