#ifndef TOKEN_BLOCK_VALIDATOR_H
#define TOKEN_BLOCK_VALIDATOR_H

#include <glog/logging.h>
#include "object.h"
#include "transaction_validator.h"

namespace Token{
    class BlockVerifier : public BlockVisitor{
    private:
        bool strict_;
        std::vector<Hash>& valid_;
        std::vector<Hash>& invalid_;

        BlockVerifier(bool strict, std::vector<Hash>& valid, std::vector<Hash>& invalid):
            strict_(strict),
            valid_(valid),
            invalid_(invalid){}
    public:
        ~BlockVerifier() = default;

        bool IsStrict() const{
            return strict_;
        }

        bool Visit(const Handle<Transaction>& tx){
            Hash hash = tx->GetHash();
            if(tx->GetNumberOfInputs() <= 0){
                invalid_.push_back(hash);
                LOG(WARNING) << "transaction " << hash << " is invalid, no inputs.";
                return false;
            } else if(tx->GetNumberOfOutputs() <= 0){
                invalid_.push_back(hash);
                LOG(WARNING) << "transaction " << hash << " is invalid, no outputs.";
                return false;
            }

            if(IsStrict()){
                if(!TransactionValidator::IsValid(tx)){
                    invalid_.push_back(hash);
                    LOG(WARNING) << "transaction " << hash << " is invalid, invalid data";
                    return false;
                }
            }

            valid_.push_back(hash);
            LOG(INFO) << "transaction " << hash << " is valid";
            return true;
        }

        static bool IsValid(const Handle<Block>& blk, bool strict=false){
            LOG(INFO) << "verifying block " << blk->GetHash();
            std::vector<Hash> valid;
            std::vector<Hash> invalid;
            BlockVerifier verifier(strict, valid, invalid);
            if(!blk->Accept(&verifier))
                return false;

            if(valid.empty()){
                LOG(WARNING) << "block " << blk->GetHash() << " is invalid, no valid transactions.";
                return false;
            } else if(invalid.size() > valid.size()){
                LOG(WARNING) << "block " << blk->GetHash() << " is invalid, more invalid than valid transactions.";
                return false;
            } else if(invalid.empty()){
                LOG(INFO) << "block " << blk->GetHash() << " is valid.";
                return true;
            } else{
                LOG(INFO) << "block " << blk->GetHash() << " is partially invalid.";
                LOG(INFO) << blk << " valid transactions: " << valid.size();
                LOG(INFO) << blk << " invalid transactions: " << invalid.size();
                return true;
            }
        }
    };
}

#endif //TOKEN_BLOCK_VALIDATOR_H