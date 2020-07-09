#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
#include "object.h"
#include "transaction_validator.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    public:
        typedef std::vector<Transaction*> TransactionList;
    private:
        Block* block_;
        TransactionList valid_txs_;
        TransactionList invalid_txs_;
    public:
        BlockValidator(Block* block):
            block_(block),
            invalid_txs_(),
            valid_txs_(){}
        ~BlockValidator(){}

        Block* GetBlock() const{
            return block_;
        }

        bool IsValid(Transaction* tx){
            uint256_t hash = tx->GetSHA256Hash();
            if(tx->GetNumberOfInputs() <= 0){
                LOG(WARNING) << "transaction " << hash << " has no inputs";
                return false;
            } else if(tx->GetNumberOfOutputs() <= 0){
                LOG(WARNING) << "transaction " << hash << " has no outputs";
                return false;
            }
            return TransactionValidator::IsValid(tx);
        }

        bool Visit(Transaction* tx){
            if(!IsValid(tx)){
                invalid_txs_.push_back(tx);
                return false;
            }

            valid_txs_.push_back(tx);
            return true;
        }

        bool IsValid() const{
            if(!GetBlock()->Accept((BlockVisitor*)this)) return false;
            return GetNumberOfValidTransactions() == GetBlock()->GetNumberOfTransactions();
        }

        uint64_t GetNumberOfInvalidTransactions() const{
            return invalid_txs_.size();
        }

        uint64_t GetNumberOfValidTransactions() const{
            return valid_txs_.size();
        }

        TransactionList::const_iterator valid_begin() const{
            return valid_txs_.begin();
        }

        TransactionList::const_iterator valid_end() const{
            return valid_txs_.end();
        }

        TransactionList::const_iterator invalid_begin() const{
            return invalid_txs_.begin();
        }

        TransactionList::const_iterator invalid_end() const{
            return invalid_txs_.end();
        }

        static bool IsValid(Block* block){
            BlockValidator validator(block);
            return validator.IsValid();
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H