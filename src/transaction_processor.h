#ifndef TOKEN_TRANSACTION_PROCESSOR_H
#define TOKEN_TRANSACTION_PROCESSOR_H

#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class TransactionHandler : public TransactionInputVisitor, public TransactionOutputVisitor{
    private:
        TransactionPtr transaction_;
        int32_t out_idx_;

        inline UnclaimedTransactionPtr
        CreateUnclaimedTransaction(const User& user, const Product& product) const{
            Hash tx_hash = transaction_->GetHash();
            //TODO: out_idx_++
            return UnclaimedTransactionPtr(new UnclaimedTransaction(tx_hash, out_idx_, user, product));
        }
    public:
        TransactionHandler(const TransactionPtr& tx):
            TransactionInputVisitor(),
            TransactionOutputVisitor(),
            transaction_(tx),
            out_idx_(0){}
        ~TransactionHandler(){}

        bool Visit(const Input& input){
            UnclaimedTransactionPtr utxo = input.GetUnclaimedTransaction();
            UnclaimedTransactionPool::RemoveUnclaimedTransaction(utxo->GetHash());
            return true;
        }

        bool Visit(const Output& output){
            UnclaimedTransactionPtr utxo = CreateUnclaimedTransaction(output.GetUser(), output.GetProduct());
            UnclaimedTransactionPool::PutUnclaimedTransaction(utxo->GetHash(), utxo);
            return true;
        }

        static bool ProcessTransaction(const TransactionPtr& tx){
            TransactionHandler handler(tx);
            return tx->VisitInputs(&handler)
                && tx->VisitOutputs(&handler);
        }
    };
}

#endif //TOKEN_TRANSACTION_PROCESSOR_H