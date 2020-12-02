#ifndef TOKEN_TRANSACTION_VALIDATOR_H
#define TOKEN_TRANSACTION_VALIDATOR_H

#include "block_chain.h"
#include "transaction.h"

namespace Token{
    class TransactionValidator : public TransactionVisitor{
    private:
        const Transaction& transaction_;
    public:
        TransactionValidator(const Transaction& transaction):
            TransactionVisitor(),
            transaction_(transaction){}
        ~TransactionValidator(){}

        bool VisitInput(const Input& input) const{
            UnclaimedTransaction* utxo;
            if(!(utxo = input.GetUnclaimedTransaction())){
                LOG(WARNING) << "couldn't get unclaimed transaction for input: " << input.ToString();
                return false;
            }
            return input.GetUser() == utxo->GetUser();
        }

        bool VisitOutput(const Output& output) const{
            //TODO: check for valid token?
            //TODO: check for valid user?
            return true;
        }

        static bool IsValid(const Transaction& tx){
            TransactionValidator validator(tx);
            return tx.Accept(&validator);
        }
    };
}

#endif //TOKEN_TRANSACTION_VALIDATOR_H