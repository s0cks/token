#ifndef TKN_VERIFIER_BLOCK_H
#define TKN_VERIFIER_BLOCK_H

#include "object_pool.h"
#include "verifier/verifier_transaction.h"

namespace token{
  namespace internal{
    class BlockVerifierBase : public IndexedTransactionVisitor{
    protected:
      UnclaimedTransactionPool& pool_;
      Hash blk_hash_;
      BlockPtr blk_;

      BlockVerifierBase(UnclaimedTransactionPool& pool, const BlockPtr& blk):
        IndexedTransactionVisitor(),
        pool_(pool),
        blk_hash_(blk->hash()),
        blk_(blk){
      }

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      ~BlockVerifierBase() override = default;

      Hash GetBlockHash() const{
        return blk_hash_;
      }

      BlockPtr GetBlock() const{
        return blk_;
      }

      virtual bool Verify() = 0;
    };
  }

  namespace sync{
    class BlockVerifier : public internal::BlockVerifierBase{
    public:
      BlockVerifier(UnclaimedTransactionPool& pool, const BlockPtr& blk):
        internal::BlockVerifierBase(pool, blk){
      }
      ~BlockVerifier() override = default;

      bool Visit(const IndexedTransactionPtr& val) override{
        NOT_IMPLEMENTED(FATAL);//TODO: implement?
        return false;
      }

      bool Verify() override{
        DLOG(INFO) << "(sync) verifying block " << hash() << "....";
        auto start = Clock::now();

        uint64_t idx = 0;
        for(auto iter = block_->transactions_begin(); iter != block_->transactions_end(); iter++){
          auto tx = (*iter);
          TransactionVerifier verifier(utxos(), tx);
          if(!verifier.Verify()){
            LOG(ERROR) << "transaction " << verifier.hash() << " is invalid: ";
            LOG(ERROR) << "  - inputs valid=" << verifier.input_verifier().GetPercentageValid() << "%, invalid=" << verifier.input_verifier().GetPercentageInvalid() << "%";
            LOG(ERROR) << "  - outputs valid=" << verifier.output_verifier().GetPercentageValid() << "%, invalid=" << verifier.output_verifier().GetPercentageInvalid() << "%";
            return true;
          }
          DVLOG(1) << "transaction " << verifier.hash() << " is valid.";
        }

        DLOG(INFO) << "(sync) verify block " << hash() << " has finished, took " << GetElapsedTimeMilliseconds(start) << "ms.";
        return true;
      }
    };
  }

  namespace parallel{
    class BlockVerifier : public internal::BlockVerifierBase{
    protected:
      task::TaskEngine& engine_;
      task::TaskQueue& queue_;
      std::vector<std::shared_ptr<VerifyTransactionTask>> tasks_;

      inline std::shared_ptr<VerifyTransactionTask>
      CreateVerifyTransactionTask(const IndexedTransactionPtr& val){
        auto task = std::make_shared<VerifyTransactionTask>(&engine_, pool(), val);
        tasks_.push_back(task);
        return task;
      }
    public:
      BlockVerifier(UnclaimedTransactionPool& pool, const BlockPtr& blk, task::TaskEngine& engine, task::TaskQueue& queue):
        internal::BlockVerifierBase(pool, blk),
        engine_(engine),
        queue_(queue),
        tasks_(){
        tasks_.reserve(blk->GetNumberOfTransactions());
      }
      ~BlockVerifier() override = default;

      task::TaskEngine& GetTaskEngine() const{
        return engine_;
      }

      task::TaskQueue& GetTaskQueue() const{
        return queue_;
      }

      bool Visit(const IndexedTransactionPtr& val) override{
        auto start = Clock::now();

        auto tx_hash = val->hash();
        DVLOG(2) << "visiting transaction " << tx_hash << "....";

        auto& queue = GetTaskQueue();
        auto task = CreateVerifyTransactionTask(val);
        if(!task->Submit(queue)){
          LOG(ERROR) << "cannot verify transaction " << tx_hash << ".";
          return false;
        }

        DVLOG(1) << "waiting for task to finish.";
        while(!task->IsFinished());//spin
        DVLOG(1) << "task has finished, took=" << GetElapsedTimeMilliseconds(start) << "ms.";
        return true;
      }

      bool Verify() const override{
        return GetBlock()->VisitTransactions(this);
      }
    };
  }
}

#endif//TKN_VERIFIER_BLOCK_H