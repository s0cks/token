#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "pool.h"
#include "block.h"
#include "job/job.h"
#include "job/scheduler.h"

namespace token{
  class ProcessInputListJob;
  class ProcessTransactionInputsJob;
  class ProcessOutputListJob;
  class ProcessTransactionOutputsJob;
  class ProcessTransactionJob;
  class ProcessBlockJob;

  //TODO: fixme
  typedef std::map<User, Wallet> UserWallets;

  //TODO: refactor code
  class ProcessBlockJob : public WalletManagerBatchWriteJob, BlockVisitor{
    friend class ProcessTransactionJob;
   protected:
    BlockPtr block_;
    std::mutex mutex_; //TODO: remove mutex
    UserWallets wallets_;
    bool clean_; //TODO: remove clean flag?

    void Append(const UserWallets& wallets){
      std::lock_guard<std::mutex> guard(mutex_);
      for(auto& it : wallets){
        auto pos = wallets_.find(it.first);
        if(pos == wallets_.end()){
          wallets_.insert({ it.first, it.second });
          continue;
        }

        auto& existing = pos->second;
        auto& new_entries = it.second;
        existing.insert(new_entries.begin(), new_entries.end());
      }
    }

    JobResult DoWork();

    void BuildBatch(){
      for(auto& it : wallets_)
        PutWallet(it.first, it.second);
    }
   public:
    ProcessBlockJob(const BlockPtr& blk, bool clean = false):
      WalletManagerBatchWriteJob(nullptr, "ProcessBlock"),
      block_(blk),
      wallets_(),
      clean_(clean){}
    ~ProcessBlockJob() = default;

    BlockPtr GetBlock() const{
      return block_;
    }

    bool ShouldClean() const{
      return clean_;
    }

    bool Visit(const TransactionPtr& tx);

    static inline bool
    SubmitAndWait(const BlockPtr& blk){
      JobQueue* queue = JobScheduler::GetThreadQueue();
      ProcessBlockJob* job = new ProcessBlockJob(blk);
      if(!queue->Push(job))
        return false;

      while(!job->IsFinished()); //spin

      job->BuildBatch();
      return job->Commit();
    }
  };

  class ProcessTransactionJob : public Job{
   protected:
    Hash hash_;
    TransactionPtr transaction_;

    JobResult DoWork();
   public:
    ProcessTransactionJob(ProcessBlockJob* parent, const TransactionPtr& tx):
      Job(parent, "ProcessTransaction"),
      hash_(tx->GetHash()),
      transaction_(tx){}
    ~ProcessTransactionJob() = default;

    void Append(const UserWallets& wallets){
      return ((ProcessBlockJob*)GetParent())->Append(wallets);
    }

    BlockPtr GetBlock() const{
      return ((ProcessBlockJob*)GetParent())->GetBlock();
    }

    bool ShouldClean() const{
      return ((ProcessBlockJob*)GetParent())->ShouldClean();
    }

    TransactionPtr GetTransaction() const{
      return transaction_;
    }

    Hash GetTransactionHash() const{
      return hash_;
    }
  };

  class ProcessTransactionInputsJob : public Job{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionInputsJob(ProcessTransactionJob* parent):
      Job(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionInputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*)GetParent())->GetTransaction();
    }

    void Append(const UserWallets& wallets){
      return ((ProcessTransactionJob*)GetParent())->Append(wallets);
    }
  };

  class ProcessInputListJob : public ObjectPoolBatchWriteJob{
   public:
    static const int64_t kMaxNumberOfInputs = 128;
   private:
    InputList inputs_;
   protected:
    JobResult DoWork(){
      ObjectPoolPtr pool = ObjectPool::GetInstance();
      for(auto& it : inputs_){
        UnclaimedTransactionPtr utxo = pool->FindUnclaimedTransaction(it);
        Hash hash = utxo->GetHash();
        //TODO: remove input
      }

      if(!Commit())
        return Failed("Cannot commit changes.");
      return Success("done.");
    }
   public:
    ProcessInputListJob(ProcessTransactionInputsJob* parent, const InputList& inputs):
      ObjectPoolBatchWriteJob(parent, "ProcessInputList"),
      inputs_(inputs){}
    ~ProcessInputListJob() = default;
  };

  class ProcessTransactionOutputsJob : public Job{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionOutputsJob(ProcessTransactionJob* parent):
      Job(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionOutputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }

    void Append(const UserWallets& wallets){
      return ((ProcessTransactionJob*)GetParent())->Append(wallets);
    }
  };

  class ProcessOutputListJob : public ObjectPoolBatchWriteJob{
   public:
    static const int64_t kMaxNumberOfOutputs = 64;
   private:
    OutputList outputs_;

    int64_t offset_;
    UserWallets wallets_;

    inline UnclaimedTransactionPtr
    CreateUnclaimedTransaction(const Output& output){
      TransactionPtr tx = GetTransaction();
      int64_t index = GetNextOutputIndex();
      return std::make_shared<UnclaimedTransaction>(tx->GetHash(), index, output.GetUser(), output.GetProduct());
    }

    int64_t GetNextOutputIndex(){
      return offset_++;
    }

    void Track(const User& user, const Hash& hash){
      auto pos = wallets_.find(user.str());
      if(pos == wallets_.end()){
        wallets_.insert({user.str(), Wallet{hash}});
        return;
      }

      Wallet& list = pos->second;
      list.insert(hash);
    }
   protected:
    JobResult DoWork();
   public:
    ProcessOutputListJob(ProcessTransactionOutputsJob* parent, int64_t wid, const OutputList& outputs):
      ObjectPoolBatchWriteJob(parent, "ProcessOutputListJob"),
      outputs_(outputs),
      offset_(wid * ProcessOutputListJob::kMaxNumberOfOutputs){}
    ~ProcessOutputListJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionOutputsJob*)GetParent())->GetTransaction();
    }
  };
}

#endif //TOKEN_PROCESS_BLOCK_H