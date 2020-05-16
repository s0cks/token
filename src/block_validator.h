#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
#include "object.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    public:
        typedef std::vector<Transaction> TransactionList;
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

        bool IsValid(const Transaction& tx);
        bool Visit(const Transaction& tx);

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