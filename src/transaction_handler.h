#ifndef TOKEN_TRANSACTION_HANDLER_H
#define TOKEN_TRANSACTION_HANDLER_H

#include "transaction.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    class TransactionHandler : public TransactionVisitor{
    private:
        Transaction* transaction_;
        uint32_t out_idx_;

        inline Handle<UnclaimedTransaction>
        CreateUnclaimedTransaction(const User& user, const Product& product){
            Hash tx_hash = transaction_->GetHash();
            return UnclaimedTransaction::NewInstance(tx_hash, out_idx_++, user, product);
        }
    public:
        TransactionHandler(Transaction* tx):
            TransactionVisitor(),
            transaction_(tx),
            out_idx_(0){}
        ~TransactionHandler(){}

        bool VisitInput(Input* input){
            UnclaimedTransaction* utxo = input->GetUnclaimedTransaction();
            UnclaimedTransactionPool::RemoveUnclaimedTransaction(utxo->GetHash());
            return true;
        }

        bool VisitOutput(Output* output){
            UnclaimedTransactionPool::PutUnclaimedTransaction(CreateUnclaimedTransaction(output->GetUser(), output->GetProduct()));
            return true;
        }

        static bool ProcessTransaction(Transaction* tx){
            TransactionHandler handler(tx);
            return tx->Accept(&handler);
        }
    };
}

#endif //TOKEN_TRANSACTION_HANDLER_H