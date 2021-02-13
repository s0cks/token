#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "pool.h"
#include "block.h"
#include "job/job.h"

#include "utils/kvstore.h"

namespace token{
  class ProcessInputListJob;
  class ProcessTransactionInputsJob;
  class ProcessOutputListJob;
  class ProcessTransactionOutputsJob;
  class ProcessTransactionJob;
  class ProcessBlockJob;

  //TODO: fixme
  typedef std::map<User, Wallet> UserWallets;

  class ProcessBlockJob : public Job, BlockVisitor{
    friend class ProcessTransactionJob;
   protected:
    BlockPtr block_;
    std::mutex mutex_; //TODO: remove mutex
    leveldb::WriteBatch* batch_;

    leveldb::WriteBatch batch_wallet_;
    UserWallets wallets_;
    bool clean_; //TODO: remove clean flag?

    JobResult DoWork();
   public:
    ProcessBlockJob(const BlockPtr& blk, bool clean = false):
      Job(nullptr, "ProcessBlock"),
      block_(blk),
      mutex_(),
      batch_(new leveldb::WriteBatch()),
      batch_wallet_(),
      clean_(clean){}
    ~ProcessBlockJob(){
      if(batch_)
        delete batch_;
    }

    BlockPtr GetBlock() const{
      return block_;
    }

    bool ShouldClean() const{
      return clean_;
    }

    bool Visit(const TransactionPtr& tx);

    void Append(const leveldb::WriteBatch& batch){
      std::lock_guard<std::mutex> guard(mutex_);
      batch_->Append(batch);
    }

    void Append(const leveldb::WriteBatch& batch, const UserWallets& hashes){
      std::lock_guard<std::mutex> guard(mutex_);
      for(auto& it : hashes){
        auto pos = wallets_.find(it.first);
        if(pos == wallets_.end()){
          LOG(INFO) << "creating new wallet for " << it.first;
          wallets_.insert({it.first, it.second});
          continue;
        }

        const Wallet& src = it.second;
        Wallet& dst = pos->second;
        dst.insert(src.begin(), src.end());
        wallets_.insert({it.first, dst});
      }
      batch_->Append(batch);
    }

    int64_t GetWalletWriteSize() const{
      return batch_wallet_.ApproximateSize();
    }

    int64_t GetPoolWriteSize() const{
      return batch_wallet_.ApproximateSize();
    }

    int64_t GetTotalWriteSize() const{
      return GetWalletWriteSize() + GetPoolWriteSize();
    }

    leveldb::Status CommitWalletChanges(){
      if(batch_wallet_.ApproximateSize() == 0)
        return leveldb::Status::IOError("the wallet batch write is ~0b in size");
      for(auto& it : wallets_){
        const User& user = it.first;
        Wallet& wallet = it.second;
        WalletManager::GetWallet(user, wallet);
        LOG(INFO) << user << " now has " << wallet.size() << " unclaimed transactions.";
        RemoveObject(batch_wallet_, user);
        PutObject(batch_wallet_, user, wallet);
      }
      return WalletManager::Write(&batch_wallet_);
    }

    bool CommitAllChanges(){
      leveldb::Status status;
      if(!(status = CommitWalletChanges()).ok()){
        LOG(ERROR) << "cannot commit wallet changes: " << status.ToString();
        return false;
      }

      return true;
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

    BlockPtr GetBlock() const{
      return ((ProcessBlockJob*) GetParent())->GetBlock();
    }

    bool ShouldClean() const{
      return ((ProcessBlockJob*) GetParent())->ShouldClean();
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
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }
  };

  class ProcessInputListJob : public ObjectPoolBatchWriteJob{
   public:
    static const int64_t kMaxNumberOfInputs = 128;
   private:
    InputList inputs_;
   protected:
    JobResult DoWork(){
      for(auto& it : inputs_){
        UnclaimedTransactionPtr utxo = ObjectPool::FindUnclaimedTransaction(it);
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
      return ((ProcessTransactionOutputsJob*) GetParent())->GetTransaction();
    }
  };
}

#endif //TOKEN_PROCESS_BLOCK_H