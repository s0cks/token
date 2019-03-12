#ifndef TOKEN_TX_POOL_H
#define TOKEN_TX_POOL_H

#include "common.h"
#include "block.h"
#include <map>

namespace Token{
    class TransactionPool{
    private:
        std::map<std::string, Transaction*> transactions_;
    public:
        TransactionPool():
            transactions_(){}
        TransactionPool(const TransactionPool& other):
            transactions_(){
            transactions_.insert(other.transactions_.begin(), other.transactions_.end());
        }
        ~TransactionPool(){}

        void AddTransaction(Transaction* tx){
            transactions_.insert(std::make_pair(tx->GetHash(), tx));
        }

        void RemoveTransaction(std::string hash) {
            transactions_.erase(hash);
        }

        Transaction* GetTransaction(std::string hash) const{
            return transactions_.find(hash)->second;
        }
    };
}

#endif //TOKEN_TX_POOL_H