#include <glog/logging.h>

#include "pool.h"
#include "buffer.h"
#include "job/job.h"
#include "wallet_manager.h"

namespace token{
#define CHECK_BATCH_SIZE(Size) \
  if((Size) <= GetMinimumBatchSize() || (Size) >= GetMaximumBatchSize()){\
    LOG_JOB(ERROR, this) << "cannot write batch of ~" << (Size) << "b, batch size should be ~" << GetMinimumBatchSize() << "-" << GetMaximumBatchSize() << "b"; \
    return false;                   \
  }

  //TODO: refactor?
  void WalletManagerBatchWriteJob::PutWallet(const User& user, const Wallet& wallet){
    BufferPtr buffer = internal::NewBuffer(GetBufferSize(wallet));
    if(!Encode(buffer, wallet)){
      LOG(WARNING) << "cannot encode wallet.";
      return;
    }
    batch_.Put((leveldb::Slice)user, buffer->operator leveldb::Slice());
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

  template<class T>
  static inline void
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const std::shared_ptr<T>& val){
    ObjectKey key(val->type(), hash);
    BufferPtr value = val->ToBuffer();
    batch.Put((const leveldb::Slice&)key, value->AsSlice());
  }

  bool ObjectPoolBatchWriteJob::PutBlock(const Hash& hash, const BlockPtr& val){
    PutObject(batch_, hash, val);
    return true;
  }

  bool ObjectPoolBatchWriteJob::PutTransaction(const Hash& hash, const UnsignedTransactionPtr& val){
    PutObject(batch_, hash, val);
    return true;
  }

  bool ObjectPoolBatchWriteJob::PutUnclaimedTransaction(const Hash& hash, const UnclaimedTransactionPtr& val){
    PutObject(batch_, hash, val);
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