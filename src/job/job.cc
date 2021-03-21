#include <glog/logging.h>

#include "pool.h"
#include "wallet.h"
#include "job/job.h"

namespace token{
#define CHECK_BATCH_SIZE(Size) \
  if((Size) <= GetMinimumBatchSize() || (Size) >= GetMaximumBatchSize()){\
    LOG_JOB(ERROR, this) << "cannot write batch of ~" << (Size) << "b, batch size should be ~" << GetMinimumBatchSize() << "-" << GetMaximumBatchSize() << "b"; \
    return false;                   \
  }

  bool WalletManagerBatchWriteJob::Commit() const{
    int64_t size = GetCurrentBatchSize();
    CHECK_BATCH_SIZE(size)

    DLOG_JOB(INFO, this) << "committing ~" << size << "b of changes to the wallet db.";

    leveldb::Status status;
    if(!(status = WalletManager::GetInstance()->Commit(batch_)).ok()){
      LOG_JOB(ERROR, this) << "cannot commit ~" << size << "b of changes to wallet db: " << status.ToString();
      return false;
    }
    return true;
  }

  //TODO: refactor
  bool ObjectPoolBatchWriteJob::Commit() const{
    ObjectPoolPtr pool = ObjectPool::GetInstance();

    int64_t size = GetCurrentBatchSize();
    CHECK_BATCH_SIZE(size)
    LOG_JOB(INFO, this) << "committing ~" << size << "b of changes to pool.";

    leveldb::Status status;
    if(!(status = pool->Write(batch_)).ok()){
      LOG_JOB(ERROR, this) << "cannot commit batch of ~" << size << "b to pool: " << status.ToString();
      return false;
    }
    return true;
  }
}