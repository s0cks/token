#ifndef TOKEN_JOB_H
#define TOKEN_JOB_H

#include <leveldb/write_batch.h>

#include <utility>

#include "pool.h"
#include "block.h"
#include "wallet.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

namespace token{
#define FOR_EACH_JOB_RESULT_STATUS(V) \
    V(Unscheduled)                    \
    V(Successful)                     \
    V(Failed)                         \
    V(Cancelled)                      \
    V(TimedOut)

  class JobResult{
   public:
    enum Status{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_JOB_RESULT_STATUS(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_JOB_RESULT_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    Status status_;
    std::string message_;
   public:
    JobResult(const Status& status, std::string  msg):
      status_(status),
      message_(std::move(msg)){}
    JobResult(const JobResult& result) = default;
    ~JobResult() = default;

    Status GetStatus() const{
      return status_;
    }

    std::string GetMessage() const{
      return message_;
    }

    bool IsUnscheduled() const{
      return GetStatus() == JobResult::kUnscheduled;
    }

    bool IsSuccessful() const{
      return GetStatus() == JobResult::kSuccessful;
    }

    bool IsFailed() const{
      return GetStatus() == JobResult::kFailed;
    }

    bool IsTimedOut() const{
      return GetStatus() == JobResult::kTimedOut;
    }

    bool IsCancelled() const{
      return GetStatus() == JobResult::kCancelled;
    }

    JobResult& operator=(const JobResult& result) = default;

    friend std::ostream& operator<<(std::ostream& stream, const JobResult& result){
      stream << result.GetMessage() << " [" << result.GetStatus() << "]";
      return stream;
    }
  };

  class Job : public Object{
   public:
    static inline JobResult
    Success(const std::string& message){
      return JobResult(JobResult::kSuccessful, message);
    }

    static inline JobResult
    Success(const std::stringstream& ss){
      return Success(ss.str());
    }

    static inline JobResult
    Failed(const std::string& message){
      return JobResult(JobResult::kFailed, message);
    }

    static inline JobResult
    Failed(const std::stringstream& ss){
      return Failed(ss.str());
    }

    static inline JobResult
    TimedOut(const std::string& message){
      return JobResult(JobResult::kTimedOut, message);
    }

    static inline JobResult
    Cancelled(const std::string& message){
      return JobResult(JobResult::kCancelled, message);
    }
   protected:
    Job* parent_;
    std::string name_;
    JobResult result_;
    std::atomic<int32_t> unfinished_;

    Job(Job* parent, std::string  name):
      parent_(parent),
      name_(std::move(name)),
      result_(JobResult::kUnscheduled, "Unscheduled"),
      unfinished_(){
      unfinished_.store(1, std::memory_order_relaxed);
      if(parent != nullptr){
        parent->IncrementUnfinishedJobs();
      }
    }

    virtual JobResult DoWork() = 0;

    int32_t GetUnfinishedJobs() const{
      return unfinished_.load(std::memory_order_relaxed);
    }

    bool IncrementUnfinishedJobs(){
      return unfinished_.fetch_add(1, std::memory_order_relaxed) == 1;
    }

    bool DecrementUnfinishedJobs(){
      return unfinished_.fetch_sub(1, std::memory_order_relaxed) == 1;
    }

    bool Finish(){
      if(!DecrementUnfinishedJobs()){
        return false;
      }
      if(HasParent()){
        return GetParent()->Finish();
      }
      return true;
    }
   public:
    ~Job() override = default;

    Job* GetParent() const{
      return parent_;
    }

    bool HasParent() const{
      return parent_ != nullptr;
    }

    bool IsFinished() const{
      return unfinished_.load(std::memory_order_seq_cst) == 0;
    }

    std::string GetName() const{
      return name_;
    }

    JobResult& GetResult(){
      return result_;
    }

    JobResult GetResult() const{
      return result_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << GetName() << "Job()";
      return ss.str();
    }

    bool Run(){
      if(IsFinished()){
        return false;
      }

      result_ = DoWork();
      Finish();
      return true;
    }
  };

  class BatchWriteJob : public Job{
   public:
    static const int64_t kMinimumBatchWriteSize = 0;
    static const int64_t kMaximumBatchWriteSize = 128 * token::internal::kMegabytes;
   protected:
    int64_t min_bsize_;
    int64_t max_bsize_;
    leveldb::WriteBatch batch_;

    BatchWriteJob(Job* parent, const std::string& name, int64_t min_bsize, int64_t max_bsize):
      Job(parent, name),
      min_bsize_(min_bsize),
      max_bsize_(max_bsize),
      batch_(){}
    BatchWriteJob(Job* parent, const std::string& name):
      BatchWriteJob(parent, name, kMinimumBatchWriteSize, kMaximumBatchWriteSize){}

    void SetMinimumBatchSize(const int64_t& val){
      min_bsize_ = val;
    }

    void SetMaximumBatchSize(const int64_t& val){
      max_bsize_ = val;
    }
   public:
    ~BatchWriteJob() override = default;

    int64_t GetMinimumBatchSize() const{
      return min_bsize_;
    }

    int64_t GetCurrentBatchSize() const{
      return batch_.ApproximateSize();
    }

    int64_t GetMaximumBatchSize() const{
      return max_bsize_;
    }

    virtual bool Commit() const = 0;
  };

  class WalletManagerJob : public Job{
   protected:
    WalletManagerJob(Job* parent, const std::string& name):
      Job(parent, name){}
   public:
    ~WalletManagerJob() override = default;
  };

  class WalletManagerBatchWriteJob : public BatchWriteJob{
   protected:
    WalletManagerBatchWriteJob(Job* parent, const std::string& name, int64_t min_bsize, int64_t max_bsize):
      BatchWriteJob(parent, name, min_bsize, max_bsize){}
    WalletManagerBatchWriteJob(Job* parent, const std::string& name):
      BatchWriteJob(parent, name){}

    //TODO: refactor?
    void PutWallet(const User& user, const Wallet& wallet){
      UserKey key(user);
      BufferPtr buffer = Buffer::NewInstance(GetBufferSize(wallet));
      if(!Encode(wallet, buffer)){
        LOG(WARNING) << "cannot encode wallet.";
        return;
      }
      batch_.Put((leveldb::Slice&)key, buffer->operator leveldb::Slice());
    }
   public:
    ~WalletManagerBatchWriteJob() override = default;

    bool Commit() const override;
  };

  class ObjectPoolJob : public Job{
   protected:
    ObjectPoolJob(Job* parent, const std::string& name):
      Job(parent, name){}
   public:
    ~ObjectPoolJob() override = default;
  };

  class ObjectPoolBatchWriteJob : public BatchWriteJob{
   protected:
    ObjectPoolBatchWriteJob(Job* parent, const std::string& name, int64_t min_bsize, int64_t max_bsize):
      BatchWriteJob(parent, name, min_bsize, max_bsize){}
    ObjectPoolBatchWriteJob(Job* parent, const std::string& name):
      BatchWriteJob(parent, name){}

    inline bool
    PutTransaction(const Hash& hash, const TransactionPtr& val){
      ObjectPool::PoolKey key(Type::kTransaction, val->GetBufferSize(), hash);
      BufferPtr value = val->ToBuffer();
      batch_.Put((leveldb::Slice&)key, value->AsSlice());
      return true;
    }

    inline bool
    PutBlock(const Hash& hash, const BlockPtr& val){
      ObjectPool::PoolKey key(Type::kBlock, val->GetBufferSize(), hash);
      BufferPtr value = val->ToBuffer();
      batch_.Put((leveldb::Slice&)key, value->AsSlice());
      return true;
    }

    inline bool
    PutUnclaimedTransaction(const Hash& hash, const UnclaimedTransactionPtr& val){
      ObjectPool::PoolKey key(Type::kUnclaimedTransaction, val->GetBufferSize(), hash);
      BufferPtr value = val->ToBuffer();
      batch_.Put((leveldb::Slice&)key, value->AsSlice());
      return true;
    }
   public:
    ~ObjectPoolBatchWriteJob() override = default;

    bool Commit() const override;
  };

#define JOB_LOG(LevelName, Job) \
  LOG(LevelName) << "[" << (Job)->GetName() << "] "

#define DLOG_JOB(LevelName, Job) \
  DLOG(LevelName) << "[" << (Job)->GetName() << "] "

}

#endif //TOKEN_JOB_H