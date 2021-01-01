#ifndef TOKEN_JOB_H
#define TOKEN_JOB_H

#include <leveldb/write_batch.h>
#include "object.h"

namespace Token{
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

    friend std::ostream &operator<<(std::ostream &stream, const Status &status){
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
    JobResult(const Status &status, const std::string &msg):
      status_(status),
      message_(msg){}
    JobResult(const JobResult &result):
      status_(result.status_),
      message_(result.message_){}
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

    void operator=(const JobResult &result){
      status_ = result.status_;
      message_ = result.message_;
    }

    friend std::ostream &operator<<(std::ostream &stream, const JobResult &result){
      stream << result.GetMessage() << " [" << result.GetStatus() << "]";
      return stream;
    }
  };

  class Job : public Object{
   public:
    static inline JobResult
    Success(const std::string &message){
      return JobResult(JobResult::kSuccessful, message);
    }

    static inline JobResult
    Success(const std::stringstream &ss){
      return Success(ss.str());
    }

    static inline JobResult
    Failed(const std::string &message){
      return JobResult(JobResult::kFailed, message);
    }

    static inline JobResult
    Failed(const std::stringstream &ss){
      return Failed(ss.str());
    }

    static inline JobResult
    TimedOut(const std::string &message){
      return JobResult(JobResult::kTimedOut, message);
    }

    static inline JobResult
    Cancelled(const std::string &message){
      return JobResult(JobResult::kCancelled, message);
    }
   protected:
    Job *parent_;
    std::string name_;
    JobResult result_;
    std::atomic<int32_t> unfinished_;

    Job(Job *parent, const std::string &name):
      parent_(parent),
      name_(name),
      result_(JobResult::kUnscheduled, "Unscheduled"),
      unfinished_(){
      unfinished_.store(1, std::memory_order_seq_cst);
      if(parent != nullptr)
        parent->IncrementUnfinishedJobs();
    }

    virtual JobResult DoWork() = 0;

    int32_t GetUnfinishedJobs() const{
      return unfinished_.load(std::memory_order_seq_cst);
    }

    bool IncrementUnfinishedJobs(){
      return unfinished_.fetch_add(1, std::memory_order_seq_cst) == 1;
    }

    bool DecrementUnfinishedJobs(){
      return unfinished_.fetch_sub(1, std::memory_order_seq_cst) == 1;
    }

    bool Finish(){
      if(!DecrementUnfinishedJobs())
        return false;
      if(HasParent())
        return GetParent()->Finish();
      return true;
    }
   public:
    virtual ~Job() = default;

    Job *GetParent() const{
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

    JobResult GetResult() const{
      return result_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << GetName() << "Job()";
      return ss.str();
    }

    bool Run(){
      if(IsFinished())
        return false;
      LOG(INFO) << "running " << GetName() << " job....";
      result_ = DoWork();
      Finish();
      return true;
    }
  };

  class WriteBatchJob : public Job{
   protected:
    WriteBatchJob(Job *parent, const std::string &name):
      Job(parent, name){}
   public:
    virtual ~WriteBatchJob() = default;
    virtual void Append(leveldb::WriteBatch *batch) = 0;
  };
}

#endif //TOKEN_JOB_H