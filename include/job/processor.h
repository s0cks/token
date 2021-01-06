#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "pool.h"
#include "block.h"
#include "job/job.h"

#include "utils/kvstore.h"

namespace Token{
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
    leveldb::WriteBatch batch_pool_;
    leveldb::WriteBatch batch_wallet_;
    UserWallets wallets_;
    bool clean_; //TODO: remove clean flag?

    JobResult DoWork();
   public:
    ProcessBlockJob(const BlockPtr& blk, bool clean = false):
      Job(nullptr, "ProcessBlock"),
      block_(blk),
      mutex_(),
      batch_pool_(),
      batch_wallet_(),
      clean_(clean){}
    ~ProcessBlockJob(){}

    BlockPtr GetBlock() const{
      return block_;
    }

    bool ShouldClean() const{
      return clean_;
    }

    bool Visit(const TransactionPtr& tx);

    void Append(const leveldb::WriteBatch& batch){
      std::lock_guard<std::mutex> guard(mutex_);
      batch_pool_.Append(batch);
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
      batch_pool_.Append(batch);
    }

    leveldb::Status Write(){
      std::lock_guard<std::mutex> guard(mutex_);
      if(batch_pool_.ApproximateSize() == 0){
        return leveldb::Status::IOError("the object pool write batch  is ~0b in size.");
      }

      if(batch_wallet_.ApproximateSize() == 0){
        return leveldb::Status::IOError("the wallet write batch is ~0b in size.");
      }

      leveldb::Status status;
      if(!(status = ObjectPool::Write(&batch_pool_)).ok()){
        LOG(WARNING) << "error occurred when writing the object pool batch: " << status.ToString();
        return leveldb::Status::IOError("cannot write the object pool batch.");
      }

      if(!(status = WalletManager::Write(&batch_wallet_)).ok()){
        LOG(WARNING) << "error occurred when writing the wallet batch: " << status.ToString();
        return leveldb::Status::IOError("cannot write the wallet batch.");
      }
      return leveldb::Status::OK();
    }
  };

  class ProcessTransactionJob : public BatchWriteJob{
   protected:
    TransactionPtr transaction_;

    JobResult DoWork();
   public:
    ProcessTransactionJob(ProcessBlockJob* parent, const TransactionPtr& tx):
      BatchWriteJob(parent, "ProcessTransaction"),
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

    void Append(const leveldb::WriteBatch& batch, const UserWallets& hashes){
      return ((ProcessBlockJob*) GetParent())->Append(batch, hashes);
    }
  };

  class ProcessTransactionInputsJob : public BatchWriteJob{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionInputsJob(ProcessTransactionJob* parent):
      BatchWriteJob(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionInputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }
  };

  class ProcessInputListJob : public InputListJob{
   public:
    static const int64_t kMaxNumberOfInputs = 128;
   protected:
    JobResult DoWork(){
      for(auto& it : inputs_){
        UnclaimedTransactionPtr utxo = ObjectPool::FindUnclaimedTransaction(it);
        Hash hash = utxo->GetHash();
        //TODO: remove input
      }

      ((ProcessTransactionInputsJob*) GetParent())->Append(GetBatch());
      return Success("done.");
    }
   public:
    ProcessInputListJob(ProcessTransactionInputsJob* parent, const InputList& inputs):
      InputListJob(parent, "ProcessInputList", inputs){}
    ~ProcessInputListJob() = default;
  };

  class ProcessTransactionOutputsJob : public BatchWriteJob{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionOutputsJob(ProcessTransactionJob* parent):
      BatchWriteJob(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionOutputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }

    void Append(const leveldb::WriteBatch& batch, const UserWallets& hashes){
      return ((ProcessTransactionJob*) GetParent())->Append(batch, hashes);
    }
  };

  class ProcessOutputListJob : public OutputListJob{
   public:
    static const int64_t kMaxNumberOfOutputs = 128;
   private:
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
    JobResult DoWork(){
      for(auto& it : outputs()){
        UnclaimedTransactionPtr val = CreateUnclaimedTransaction(it);
        Hash hash = val->GetHash();
        PutObject(batch_, hash, val);
        Track(val->GetUser(), hash);
      }

      ((ProcessTransactionOutputsJob*) GetParent())->Append(GetBatch(), wallets_);
      return Success("done.");
    }
   public:
    ProcessOutputListJob(ProcessTransactionOutputsJob* parent, int64_t wid, const OutputList& outputs):
      OutputListJob(parent, "ProcessOutputListJob", outputs),
      offset_(wid * ProcessOutputListJob::kMaxNumberOfOutputs){}
    ~ProcessOutputListJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionOutputsJob*) GetParent())->GetTransaction();
    }
  };
}

#endif //TOKEN_PROCESS_BLOCK_H