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
  typedef std::map<std::string, HashList> UserHashLists;

  class ProcessBlockJob : public Job, BlockVisitor{
    friend class ProcessTransactionJob;
   protected:
    BlockPtr block_;
    std::mutex mutex_; //TODO: remove mutex
    leveldb::WriteBatch batch_;
    UserHashLists hash_lists_;
    bool clean_; //TODO: remove clean flag?

    JobResult DoWork();
   public:
    ProcessBlockJob(const BlockPtr& blk, bool clean = false):
      Job(nullptr, "ProcessBlock"),
      block_(blk),
      mutex_(),
      batch_(),
      hash_lists_(),
      clean_(clean){}
    ~ProcessBlockJob(){}

    leveldb::WriteBatch& GetBatch(){
      return batch_;
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
      batch_.Append(batch);
    }

    void Append(const leveldb::WriteBatch& batch, const UserHashLists& hashes){
      std::lock_guard<std::mutex> guard(mutex_);
      for(auto& it : hashes){
        LOG(INFO) << "appending " << it.second.size() << " hashes for " << it.first;
        auto pos = hash_lists_.find(it.first);
        if(pos == hash_lists_.end()){
          LOG(INFO) << "creating new hash list";
          hash_lists_.insert({it.first, it.second});
          continue;
        }

        const HashList& src = it.second;
        HashList& dst = pos->second;
        dst.insert(src.begin(), src.end());
        hash_lists_.insert({it.first, dst});
      }
      batch_.Append(batch);
    }

    leveldb::Status Write(){
      std::lock_guard<std::mutex> guard(mutex_);
      if(batch_.ApproximateSize() == 0)
        LOG(WARNING) << "writing batch of size 0";
      return ObjectPool::Write(&batch_);
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

    void Append(const leveldb::WriteBatch& batch, const UserHashLists& hashes){
      return ((ProcessBlockJob*)GetParent())->Append(batch, hashes);
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
      return ((ProcessTransactionJob*)GetParent())->GetTransaction();
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

      ((ProcessTransactionInputsJob*)GetParent())->Append(GetBatch());
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
      return ((ProcessTransactionJob*)GetParent())->GetTransaction();
    }

    void Append(const leveldb::WriteBatch& batch, const UserHashLists& hashes){
      return ((ProcessTransactionJob*)GetParent())->Append(batch, hashes);
    }
  };

  class ProcessOutputListJob : public OutputListJob{
   public:
    static const int64_t kMaxNumberOfOutputs = 128;
   private:
    int64_t offset_;
    UserHashLists hashes_;

    inline UnclaimedTransactionPtr
    CreateUnclaimedTransaction(const Output& output){
      TransactionPtr tx = GetTransaction();
      int64_t index = GetNextOutputIndex();
      LOG(INFO) << "creating unclaimed transaction for " << tx->GetHash() << "[" << index << "]";
      return std::make_shared<UnclaimedTransaction>(tx->GetHash(), index, output.GetUser(), output.GetProduct());
    }

    int64_t GetNextOutputIndex(){
      return offset_++;
    }

    void Track(const User& user, const Hash& hash){
      auto pos = hashes_.find(user.str());
      if(pos == hashes_.end()){
        hashes_.insert({user.str(), HashList{hash}});
        return;
      }

      HashList& list = pos->second;
      list.insert(hash);
    }
   protected:
    JobResult DoWork(){
      for(auto& it : outputs()){
        UnclaimedTransactionPtr val = CreateUnclaimedTransaction(it);
        Hash hash = val->GetHash();
        PutObject(batch_, hash, val);
      }

      ((ProcessTransactionOutputsJob*)GetParent())->Append(GetBatch(), hashes_);
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