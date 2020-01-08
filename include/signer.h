#ifndef TOKEN_SIGNER_H
#define TOKEN_SIGNER_H

#include "transaction.h"

namespace Token{
    class TransactionSigner{
    private:
        Transaction* tx_;
    public:
        TransactionSigner(Transaction* tx):
            tx_(tx){}
        ~TransactionSigner(){}

        Transaction* GetTransaction(){
            return tx_;
        }

        bool GetSignature(std::string* signature);
        bool Sign();
    };
}

#endif //TOKEN_SIGNER_H
