#ifndef TOKEN_VERIFIER_H
#define TOKEN_VERIFIER_H

#include "transaction.h"

namespace Token{
    class TransactionVerifier{
    private:
        Transaction* tx_;
    public:
        TransactionVerifier(Transaction* tx):
            tx_(tx){}
        ~TransactionVerifier(){}

        Transaction* GetTransaction(){
            return tx_;
        }

        bool VerifySignature();
    };
}

#endif //TOKEN_VERIFIER_H