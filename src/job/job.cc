#include <glog/logging.h>

#include "pool.h"
#include "job/job.h"

namespace token{
#define JOB_LOG(LevelName) \
  LOG(LevelName) << "[" << GetName() << "] "

  bool ObjectPoolBatchWriteJob::Commit() const{
    int64_t size = GetCurrentBatchSize();
    if(size <= GetMinimumBatchSize()){
      JOB_LOG(ERROR) << "cannot write batch of ~" << size << "b, batch cannot be under size of " << GetMinimumBatchSize();
      return false;
    }

    if(size >= GetMaximumBatchSize()){
      JOB_LOG(ERROR) << "cannot write batch of ~" << size << "b, batch cannot exceed size of " << GetMaximumBatchSize();
      return false;
    }

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO) << "committing ~" << size << "b of changes to object pool.";
#endif//TOKEN_DEBUG

    leveldb::Status status;
    if(!(status = ObjectPool::Write(batch_)).ok()){
      JOB_LOG(ERROR) << "cannot commit batch of ~" << size << "b: " << status.ToString();
      return false;
    }
    return true;
  }
}