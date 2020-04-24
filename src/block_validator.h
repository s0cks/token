#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
#include "object.h"

namespace Token{
    class BlockValidator : public BlockVisitor{
    private:
        Block* block_;
        std::vector<Transaction> valid_txs_;
        std::vector<Transaction> invalid_txs_;
    public:
        BlockValidator(Block* block):
            block_(block),
            invalid_txs_(),
            valid_txs_(){}
        ~BlockValidator(){}

        Block* GetBlock() const{
            return block_;
        }

        bool IsBlockValid() const{
            return GetNumberOfValidTransactions() == GetBlock()->GetNumberOfTransactions();
        }

        bool IsValid(const Transaction& tx);
        bool Visit(const Transaction& tx);

        uint64_t GetNumberOfInvalidTransactions() const{
            return invalid_txs_.size();
        }

        uint64_t GetNumberOfValidTransactions() const{
            return valid_txs_.size();
        }

        static bool IsValid(Block* block){
            BlockValidator validator(block);
            if(!block->Accept(&validator)) return false;
            return validator.IsBlockValid();
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H