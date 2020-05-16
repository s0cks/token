#include "block_chain.h"
#include "block_validator.h"

namespace Token{
    bool BlockValidator::IsValid(const Transaction& tx){
        uint256_t hash = tx.GetHash();
        if(tx.GetNumberOfInputs() <= 0){
            LOG(WARNING) << "transaction " << hash << " has no inputs";
            return false;
        } else if(tx.GetNumberOfOutputs() <= 0){
            LOG(WARNING) << "transaction " << hash << " has no outputs";
            return false;
        }

        for(auto it = tx.inputs_begin();
                it != tx.inputs_end();
                it++){
            Transaction in_tx;
            if(!it->GetTransaction(&in_tx)){
                LOG(WARNING) << "couldn't get transaction: " << it->GetTransactionHash();
                return false;
            }

            UnclaimedTransaction utxo(in_tx, it->GetOutputIndex());
            uint256_t utxo_hash = utxo.GetHash();
            if(!UnclaimedTransactionPool::HasUnclaimedTransaction(utxo_hash)) {
                LOG(WARNING) << "couldn't find unclaimed transaction: " << utxo_hash;
                return false;
            }
        }
        return true;
    }

    bool BlockValidator::Visit(const Transaction& tx){
        uint32_t index = 0;
        uint256_t hash = tx.GetHash();
        if(!IsValid(tx)){
            invalid_txs_.push_back(tx);
            return false;
        }

        index = 0;
        for(auto it = tx.outputs_begin(); it != tx.outputs_end(); it++){
            UnclaimedTransaction utxo(tx, index++);
            if(!UnclaimedTransactionPool::PutUnclaimedTransaction(&utxo)){
                LOG(WARNING) << "couldn't create unclaimed transaction for: " << hash << "[" << index << "]";
                return false;
            }
        }

        index = 0;
        for(auto it = tx.inputs_begin(); it != tx.inputs_end(); it++){
            UnclaimedTransaction utxo;
            if(!it->GetUnclaimedTransaction(&utxo)){
                LOG(WARNING) << "couldn't find unclaimed transaction for: " << it->GetTransactionHash() << "[" << it->GetOutputIndex() << "]";
                return false;
            }

            uint256_t uhash = utxo.GetHash();
            if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(uhash)){
                LOG(WARNING) << "couldn't remove unclaimed transaction: " << uhash;
                return false;
            }

            index++;
        }
        valid_txs_.push_back(tx);
        return true;
    }
}