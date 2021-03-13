#include <glog/logging.h>

#include "pool.h"
#include "wallet.h"
#include "job/job.h"

namespace token{
#define CHECK_BATCH_SIZE(Size) \
  if((Size) <= GetMinimumBatchSize() || (Size) >= GetMaximumBatchSize()){\
    JOB_LOG(ERROR, this) << "cannot write batch of ~" << (Size) << "b, batch size should be ~" << GetMinimumBatchSize() << "-" << GetMaximumBatchSize() << "b"; \
    return false;                   \
  }

  bool WalletManagerBatchWriteJob::Commit() const{
    int64_t size = GetCurrentBatchSize();
    CHECK_BATCH_SIZE(size)

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO, this) << "committing ~" << size << "b of changes to the wallet db.";
#endif//TOKEN_DEBUG

    leveldb::Status status;
    if(!(status = WalletManager::GetInstance()->Commit(batch_)).ok()){
      JOB_LOG(ERROR, this) << "cannot commit ~" << size << "b of changes to wallet db: " << status.ToString();
      return false;
    }
    return true;
  }

  bool ObjectPoolBatchWriteJob::Commit() const{
    int64_t size = GetCurrentBatchSize();
    CHECK_BATCH_SIZE(size)

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO, this) << "committing ~" << size << "b of changes to pool.";
#endif//TOKEN_DEBUG

    leveldb::Status status;
    if(!(status = ObjectPool::Write(batch_)).ok()){
      JOB_LOG(ERROR, this) << "cannot commit batch of ~" << size << "b to pool: " << status.ToString();
      return false;
    }
    return true;
  }
}