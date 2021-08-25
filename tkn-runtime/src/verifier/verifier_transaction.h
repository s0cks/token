#ifndef TKN_VERIFIER_TRANSACTION_H
#define TKN_VERIFIER_TRANSACTION_H

#include "object_pool.h"
#include "verifier/verifier_transaction_inputs.h"
#include "verifier/verifier_transaction_outputs.h"

namespace token{
  namespace internal{
    class TransactionVerifierBase{
    protected:
      UnclaimedTransactionPool& pool_;
      Hash tx_hash_;
      IndexedTransactionPtr tx_;

      TransactionVerifierBase(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
        pool_(pool),
        tx_hash_(val->hash()),
        tx_(val){
      }

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      virtual ~TransactionVerifierBase() = default;

      Hash GetTransactionHash() const{
        return tx_hash_;
      }

      IndexedTransactionPtr GetTransaction() const{
        return tx_;
      }

      virtual bool Verify() const = 0;
    };
  }

  namespace sync{
    class TransactionVerifier : public internal::TransactionVerifierBase{
    protected:
      sync::TransactionInputsVerifier verifier_inputs_;
      sync::TransactionOutputsVerifier verifier_outputs_;
    public:
      TransactionVerifier(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
        internal::TransactionVerifierBase(pool, val),
        verifier_inputs_(pool, val),
        verifier_outputs_(pool, val){
      }
      ~TransactionVerifier() override = default;

      bool Verify() const override{
        if(!tx_->VisitInputs(&verifier_inputs_)){
          LOG(ERROR) << "cannot visit Transaction inputs.";
          return false;
        }

        if(!tx_->VisitOutputs(&verifier_outputs_)){
          LOG(ERROR) << "cannot visit Transaction outputs.";
          return false;
        }
        return true;
      }
    };
  }
}

#endif//TKN_VERIFIER_TRANSACTION_H