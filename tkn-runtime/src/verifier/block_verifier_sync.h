#ifndef TKN_SYNC_BLOCK_VERIFIER_H
#define TKN_SYNC_BLOCK_VERIFIER_H

#include "object_pool.h"
#include "block_verifier.h"
#include "atomic/bit_vector.h"

namespace token{
  namespace sync{
    class TransactionInputVerifier : public InputVisitor{
    protected:
      UnclaimedTransactionPool& pool_;
      uint64_t total_;
      uint64_t processed_;
      uint64_t valid_;
      uint64_t invalid_;

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }

      bool IsValid(const Input& val);
    public:
      TransactionInputVerifier(UnclaimedTransactionPool& pool, const uint64_t& total):
        InputVisitor(),
        pool_(pool),
        total_(total),
        processed_(0),
        valid_(0),
        invalid_(0){}
      ~TransactionInputVerifier() override = default;

      uint64_t total() const{
        return total_;
      }

      uint64_t processed() const{
        return processed_;
      }

      uint64_t valid() const{
        return valid_;
      }

      uint64_t invalid() const{
        return invalid_;
      }

      double GetPercentageValid() const{
        return GetPercentageOf(total(), valid());
      }

      double GetPercentageInvalid() const{
        return GetPercentageOf(total(), invalid());
      }

      bool Visit(const Input& val) override{
        processed_++;
        if(IsValid(val)){
          valid_++;
        } else{
          invalid_++;
        }
        return true;
      }
    };

    class TransactionOutputVerifier : public OutputVisitor{
    private:
      uint64_t total_;
      uint64_t processed_;
      uint64_t valid_;
      uint64_t invalid_;
    public:
      explicit TransactionOutputVerifier(const uint64_t& total):
        OutputVisitor(),
        total_(total),
        processed_(0),
        valid_(0),
        invalid_(0){}
      ~TransactionOutputVerifier() override = default;

      uint64_t total() const{
        return total_;
      }

      uint64_t processed() const{
        return processed_;
      }

      uint64_t valid() const{
        return valid_;
      }

      uint64_t invalid() const{
        return invalid_;
      }

      double GetPercentageValid() const{
        return GetPercentageOf(total(), valid());
      }

      double GetPercentageInvalid() const{
        return GetPercentageOf(total(), invalid());
      }

      bool Visit(const Output& val) override;
    };

    class TransactionVerifier{
    private:
      IndexedTransactionPtr transaction_;
      Hash transaction_hash_;
      uint64_t transaction_idx_;

      UnclaimedTransactionPool& pool_;
      TransactionInputVerifier input_verifier_;
      TransactionOutputVerifier output_verifier_;

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      explicit TransactionVerifier(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
        transaction_(val),
        pool_(pool),
        input_verifier_(pool, val->GetNumberOfInputs()),
        output_verifier_(val->GetNumberOfOutputs()),
        transaction_hash_(val->hash()),
        transaction_idx_(val->index()){}
      ~TransactionVerifier() = default;

      TransactionInputVerifier& input_verifier(){
        return input_verifier_;
      }

      TransactionOutputVerifier& output_verifier(){
        return output_verifier_;
      }

      Hash hash() const{
        return transaction_hash_;
      }

      bool Verify();
      bool IsValid();
    };

    class BlockVerifier : public internal::BlockVerifierBase,
                          public IndexedTransactionVisitor{
    protected:
      BlockPtr block_;
      uint64_t num_transactions_;
    public:
      BlockVerifier(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockVerifierBase(blk, utxos),
        num_transactions_(blk->GetNumberOfTransactions()){}
      ~BlockVerifier() override = default;

      bool Visit(const IndexedTransactionPtr& val);
      bool Verify() override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_VERIFIER_H