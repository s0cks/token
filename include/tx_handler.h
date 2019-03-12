#ifndef TOKEN_TX_HANDLER_H
#define TOKEN_TX_HANDLER_H

#include "common.h"
#include "array.h"
#include "block.h"
#include "utx_pool.h"

namespace Token{
    class TransactionHandler{
    private:
        UnclaimedTransactionPool* utxpool_;
    public:
        TransactionHandler(UnclaimedTransactionPool* utxpool):
            utxpool_(utxpool){}
        ~TransactionHandler(){}

        UnclaimedTransactionPool* GetUnclaimedTransactionPool() const{
            return utxpool_;
        }

        bool IsValid(Transaction* tx);
        bool HandleTransactions(Array<Transaction*> input, Array<Transaction*> output);
    };
}

#endif //TOKEN_TX_HANDLER_H