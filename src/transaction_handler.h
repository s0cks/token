#ifndef TOKEN_TRANSACTION_HANDLER_H
#define TOKEN_TRANSACTION_HANDLER_H

#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class TransactionHandler : public TransactionVisitor{
    private:
        Transaction* transaction_;
        uint32_t out_idx_;

        inline UnclaimedTransaction*
        CreateUnclaimedTransaction(const std::string& user){
            uint256_t tx_hash = transaction_->GetHash();
            return UnclaimedTransaction::NewInstance(tx_hash, out_idx_++, user);
        }
    public:
        TransactionHandler(Transaction* tx):
            TransactionVisitor(),
            transaction_(tx),
            out_idx_(0){}
        ~TransactionHandler(){}

        bool VisitInput(Input* input){
            UnclaimedTransaction* utxo = input->GetUnclaimedTransaction();
            if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(utxo->GetHash())){
                LOG(WARNING) << "couldn't remove unclaimed transaction from pool: " << utxo->ToString();
                return false;
            }
            return true;
        }

        bool VisitOutput(Output* output){
            UnclaimedTransaction* utxo = CreateUnclaimedTransaction(output->GetUser());
            if(!UnclaimedTransactionPool::PutUnclaimedTransaction(utxo)){
                LOG(WARNING) << "couldn't add unclaimed transaction to pool: " << utxo->ToString();
                return false;
            }
            return true;
        }

        static bool ProcessTransaction(Transaction* tx){
            TransactionHandler handler(tx);
            return tx->Accept(&handler);
        }
    };
}

#endif //TOKEN_TRANSACTION_HANDLER_H