#ifndef TOKEN_TRANSACTION_VERIFIER_H
#define TOKEN_TRANSACTION_VERIFIER_H

#include "verifier.h"
#include "block_chain.h"
#include "transaction.h"

namespace Token{
    class InputVerifier : public Verifier<Input>,
                          public TransactionInputVisitor{
        inline bool
        IsValidInput(const Input& input) const{
            Hash tx = input.GetTransactionHash();
            int64_t idx = input.GetOutputIndex();
            if(!UnclaimedTransactionPool::HasUnclaimedTransaction(tx, idx))
                return false;
            return true; // TODO: check user_ & product_
        }
    public:
        InputVerifier() = default;
        ~InputVerifier() = default;

        bool Visit(const Input& input){
            if(IsValidInput(input)){
                valid_.push_back(input);
            } else{
                invalid_.push_back(input);
            }
            return true;
        }
    };

    class OutputVerifier : public Verifier<Output>,
                           public TransactionOutputVisitor{
        inline bool
        IsValidOutput(const Output& output) const{
            //TODO: check for valid token?
            //TODO: check for valid user?
            return true;
        }
    public:
        OutputVerifier() = default;
        ~OutputVerifier() = default;

        bool Visit(const Output& output){
            if(IsValidOutput(output)){
                valid_.push_back(output);
            } else{
                invalid_.push_back(output);
            }
            return true;
        }
    };

    class TransactionVerifier{
    private:
        TransactionVerifier() = delete;

        static inline bool
        VerifyInputs(const Hash& hash, const TransactionPtr& tx){
            InputVerifier verifier;
            if(!tx->VisitInputs(&verifier)){
                LOG(ERROR) << "couldn't verify inputs for transaction: " << hash;
                return false;
            }

            if(!verifier.IsValid()){
                InputList& invalid = verifier.invalid();
                LOG(ERROR) << "transaction " << hash << " has " << invalid.size() << " invalid inputs:";
                for(auto& it : invalid)
                    LOG(ERROR) << " - " << it.ToString();
                return false;
            }
            return true;
        }

        static inline bool
        VerifyOutputs(const Hash& hash, const TransactionPtr& tx){
            OutputVerifier verifier;
            if(!tx->VisitOutputs(&verifier)){
                LOG(ERROR) << "couldn't verify outputs for transaction: " << hash;
                return false;
            }

            if(!verifier.IsValid()){
                OutputList& invalid = verifier.invalid();
                LOG(ERROR) << "transaction " << hash << " has " << invalid.size() << " invalid outputs:";
                for(auto& it : invalid)
                    LOG(ERROR) << " - " << it.ToString();
                return false;
            }
            return true;
        }
    public:
        ~TransactionVerifier() = delete;

        static bool IsValid(const TransactionPtr& tx){
            Hash hash = tx->GetHash();
            LOG(INFO) << "verifying transaction: " << hash;
            return VerifyInputs(hash, tx)
                && VerifyOutputs(hash, tx);
        }
    };
}

#endif //TOKEN_TRANSACTION_VERIFIER_H