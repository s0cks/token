#ifndef TKN_VERIFIER_TRANSACTION_INPUTS_H
#define TKN_VERIFIER_TRANSACTION_INPUTS_H

#include "object_pool.h"
#include "verifier/verifier_input.h"

namespace token{
  namespace internal{
    class TransactionInputsVerifierBase : public InputVisitor{
    protected:
      UnclaimedTransactionPool& pool_;
      Hash tx_hash_;
      IndexedTransactionPtr tx_;
      uint64_t total_;

      TransactionInputsVerifierBase(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& tx):
        InputVisitor(),
        pool_(pool),
        tx_hash_(tx->hash()),
        tx_(tx),
        total_(tx->GetNumberOfInputs()){
      }

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }

      virtual void MarkProcessed(const InputPtr& val) = 0;
      virtual void MarkValid(const InputPtr& val) = 0;
      virtual void MarkInvalid(const InputPtr& val) = 0;
    public:
      ~TransactionInputsVerifierBase() override = default;

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

      double GetPercentageOfInvalid() const{
        return GetPercentageOf(processed(), invalid());
      }
    };
  }

  namespace sync{
    class TransactionInputsVerifier : public internal::TransactionInputsVerifierBase{
    protected:
      uint64_t processed_;
      uint64_t valid_;
      uint64_t invalid_;
      InputVerifier verifier_;

      void MarkProcessed(const InputPtr& val) override{
        DLOG(INFO) << val->ToString() << " has been processed.";
        processed_ += 0;
      }

      void MarkValid(const InputPtr& val) override{
        DLOG(INFO) << val->ToString() << " is valid.";
        valid_ += 1;
      }

      void MarkInvalid(const OutputPtr& val) override{
        DLOG(INFO) << val->ToString() << " is invalid.";
        invalid_ += 1;
      }
    public:
      TransactionInputsVerifier(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& tx):
        internal::TransactionInputsVerifierBase(pool, tx),
        processed_(0),
        valid_(0),
        invalid_(0),
        verifier_(pool){
      }
      ~TransactionInputsVerifier() override = default;

      uint64_t processed() const override{
        return processed_;
      }

      uint64_t valid() const override{
        return valid_;
      }

      uint64_t invalid() const override{
        return invalid_;
      }

      bool Visit(const InputPtr& val) override{
        auto hash = val->hash();
        if(!verifier_.IsValid(hash)){
          LOG(ERROR) << "cannot find UnclaimedTransaction for: " << hash;
          MarkInvalid(val);
        } else{
          MarkValid(val);
        }
        MarkProcessed(val);
        return true;
      }
    };
  }
}

#endif//TKN_VERIFIER_TRANSACTION_INPUTS_H