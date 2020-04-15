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

        uint64_t idx;
        for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
            Input input;
            if(!tx.GetInput(idx, &input)){
                LOG(WARNING) << "couldn't get input #" << idx;
                return false;
            }
            /*
             *TODO:
             * UnclaimedTransaction utxo(hash, idx,
             * if(!UnclaimedTransactionPool::HasUnclaimedTransaction(utxo.GetHash())){
             *   LOG(WARNING) << "no unclaimed transaction found for input #" << idx << " of " << hash;
             *   return false;
             * }
             */
        }
        return true;
    }

    bool BlockValidator::Visit(const Transaction& tx){
        uint256_t hash = tx.GetHash();
        if(!IsValid(tx)){
            invalid_txs_.push_back(tx);
            return false;
        }
        uint64_t idx;
        for(idx = 0; idx < tx.GetNumberOfOutputs(); idx++){
            Output out;
            if(!tx.GetOutput(idx, &out)){
                LOG(WARNING) << "couldn't get output #" << idx << " of " << hash;
                return false;
            }

            UnclaimedTransaction utxo(&out);
            if(!UnclaimedTransactionPool::PutUnclaimedTransaction(&utxo)){
                LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                LOG(WARNING) << "*** Unclaimed Transaction: ";
                LOG(WARNING) << "***   + Input: " << utxo.GetTransaction() << "[" << utxo.GetIndex() << "]";
                LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                return false;
            }
        }
        for(idx = 0; idx < tx.GetNumberOfInputs(); idx++){
            Input input;
            if(!tx.GetInput(idx, &input)){
                LOG(WARNING) << "couldn't get input #" << idx;
                return false;
            }

            /*
             *TODO:
             * UnclaimedTransaction utxo(input.GetTransactionHash(), input.GetOutputIndex(),
             * uint256_t uhash = utxo.GetHash();
             * if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(uhash)){
             *      LOG(WARNING) << "couldn't remove unclaimed transaction: " << uhash;
             *      LOG(WARNING) << "*** Unclaimed Transaction: ";
             *      LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
             *      LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
             *      return false;
             * }
             */
        }
        valid_txs_.push_back(tx);
        return true;
    }
}