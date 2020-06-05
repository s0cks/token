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

            Output in_out;
            if(!in_tx.GetOutput(it->GetOutputIndex(), &in_out)){
                LOG(WARNING) << "couldn't get output #" << it->GetOutputIndex() << " from transaction: " << it->GetTransactionHash();
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
        if(!IsValid(tx)){
            invalid_txs_.push_back(tx);
            return false;
        }
        valid_txs_.push_back(tx);
        return true;
    }
}