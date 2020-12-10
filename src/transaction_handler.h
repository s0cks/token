#ifndef TOKEN_TRANSACTION_HANDLER_H
#define TOKEN_TRANSACTION_HANDLER_H

#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class TransactionHandler : public TransactionVisitor{
    private:
        Transaction transaction_;
        int32_t out_idx_;

        inline UnclaimedTransactionPtr
        CreateUnclaimedTransaction(const User& user, const Product& product) const{
            Hash tx_hash = transaction_.GetHash();
            //TODO: out_idx_++
            return UnclaimedTransactionPtr(new UnclaimedTransaction(tx_hash, out_idx_, user, product));
        }
    public:
        TransactionHandler(const Transaction& tx):
            TransactionVisitor(),
            transaction_(tx),
            out_idx_(0){}
        ~TransactionHandler(){}

        bool VisitInput(const Input& input) const{
            UnclaimedTransactionPtr utxo = input.GetUnclaimedTransaction();
            UnclaimedTransactionPool::RemoveUnclaimedTransaction(utxo->GetHash());
            return true;
        }

        bool VisitOutput(const Output& output) const{
            UnclaimedTransactionPtr utxo = CreateUnclaimedTransaction(output.GetUser(), output.GetProduct());
            UnclaimedTransactionPool::PutUnclaimedTransaction(utxo->GetHash(), utxo);
            return true;
        }

        static bool ProcessTransaction(const Transaction& tx){
            TransactionHandler handler(tx);
            return tx.Accept(&handler);
        }
    };
}

#endif //TOKEN_TRANSACTION_HANDLER_H