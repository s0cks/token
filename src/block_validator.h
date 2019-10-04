#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
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
            if(tx->GetNumberOfInputs() == 0){
                LOG(WARNING) << "inputs == 0";
                return false;
            }
            if(tx->GetNumberOfOutputs() == 0){
                LOG(WARNING) << "outputs == 0";
                return false;
            }
            int i;
            for(i = 0; i < tx->GetNumberOfInputs(); i++){
                Input* in = tx->GetInputAt(i);
                UnclaimedTransaction ut;
                if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(in->GetPreviousHash(), in->GetIndex(), &ut)){
                    LOG(WARNING) << "no unclaimed transaction for: " << in->GetPreviousHash() << "[" << in->GetIndex() << "]";
                    return false;
                }
            }
            return true;
        }

        void VisitTransaction(Transaction* tx){
            LOG(INFO) << "validating transaction: " << tx->GetHash();
            if(IsValid(tx)){
                int i;
                for(i = 0; i < tx->GetNumberOfOutputs(); i++){
                    UnclaimedTransaction utxo(tx->GetHash(), i, tx->GetOutputAt(i));
                    if(!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(&utxo)){
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return;
                    }
                    LOG(INFO) << "added new unclaimed transaction: " << utxo.GetHash();
                }
                for(i = 0; i < tx->GetNumberOfInputs(); i++){
                    Input* in = tx->GetInputAt(i);
                    UnclaimedTransaction utxo(in->GetPreviousHash(), in->GetIndex());
                    if(!UnclaimedTransactionPool::GetInstance()->RemoveUnclaimedTransaction(&utxo)){
                        LOG(WARNING) << "couldn't remove unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return;
                    }
                    LOG(INFO) << "removed spent unclaimed transaction: " << utxo.GetHash();
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