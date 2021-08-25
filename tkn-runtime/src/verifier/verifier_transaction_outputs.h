#ifndef TKN_VERIFIER_TRANSACTION_OUTPUTS_H
#define TKN_VERIFIER_TRANSACTION_OUTPUTS_H

#include "object_pool.h"

namespace token{
  namespace internal{
    class TransactionOutputsVerifierBase : public OutputVisitor{
    protected:
      UnclaimedTransactionPool& pool_;
      Hash tx_hash_;
      IndexedTransactionPtr tx_;
      uint64_t total_;

      TransactionOutputsVerifierBase(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& tx):
        OutputVisitor(),
        pool_(pool),
        tx_hash_(tx->hash()),
        tx_(tx),
        total_(tx->GetNumberOfOutputs()){
      }

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }

      virtual void MarkProcessed(const OutputPtr& val) = 0;
      virtual void MarkValid(const OutputPtr& val) = 0;
      virtual void MarkInvalid(const OutputPtr& val) = 0;
    public:
      ~TransactionOutputsVerifierBase() override = default;

      Hash GetTransactionHash() const{
        return tx_hash_;
      }

      IndexedTransactionPtr GetTransaction() const{
        return tx_;
      }

      uint64_t total() const{
        return total_;
      }

      virtual uint64_t processed() const = 0;
      virtual uint64_t valid() const = 0;
      virtual uint64_t invalid() const = 0;

      double GetPercentageProcessed() const{
        return GetPercentageOf(total(), processed());
      }

      double GetPercentageValid() const{
        return GetPercentageOf(processed(), valid());
      }

      double GetPercentageInvalid() const{
        return GetPercentageOf(processed(), invalid());
      }
    };
  }

  namespace sync{
    class TransactionOutputsVerifier : public internal::TransactionOutputsVerifierBase{
    protected:
      uint64_t processed_;
      uint64_t valid_;
      uint64_t invalid_;

      void MarkProcessed(const OutputPtr& val) override{
        DLOG(INFO) << val->ToString() << " has been processed.";
        processed_ += 1;
      }

      void MarkValid(const OutputPtr& val) override{
        DLOG(INFO) << val->ToString() << " is valid.";
        valid_ += 1;
      }

      void MarkInvalid(const OutputPtr& val) override{
        DLOG(INFO) << val->ToString() << " is invalid.";
        invalid_ += 1;
      }
    public:
      TransactionOutputsVerifier(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& tx):
        internal::TransactionOutputsVerifierBase(pool, tx),
        processed_(0),
        valid_(0),
        invalid_(0){
      }
      ~TransactionOutputsVerifier() override = default;

      uint64_t processed() const override{
        return processed_;
      }

      uint64_t valid() const override{
        return valid_;
      }

      uint64_t invalid() const override{
        return invalid_;
      }

      bool Visit(const OutputPtr& val) override{
        MarkValid(val);
        MarkProcessed(val);
        return true;
      }
    };
  }
}

#endif//TKN_VERIFIER_TRANSACTION_OUTPUTS_H