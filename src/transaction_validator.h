#ifndef TOKEN_TRANSACTION_VALIDATOR_H
#define TOKEN_TRANSACTION_VALIDATOR_H

#include "block_chain.h"
#include "transaction.h"

namespace Token{
    class TransactionValidator : public TransactionVisitor{
    private:
        Transaction* transaction_;
    public:
        TransactionValidator(Transaction* tx):
            TransactionVisitor(),
            transaction_(tx){}
        ~TransactionValidator(){}

        bool VisitInput(Input* input){
            Handle<UnclaimedTransaction> utxo = input->GetUnclaimedTransaction();
            if(utxo.IsNull()){
                LOG(WARNING) << "couldn't find unclaimed transaction for input: " << input->ToString();
                return false;
            }
            return input->GetUser() == utxo->GetUser();
        }

        bool VisitOutput(Output* output){
            //TODO: check for valid token?
            //TODO: check for valid user?
            return true;
        }

        static bool IsValid(Transaction* tx){
            TransactionValidator validator(tx);
            return tx->Accept(&validator);
        }
    };
}

#endif //TOKEN_TRANSACTION_VALIDATOR_H