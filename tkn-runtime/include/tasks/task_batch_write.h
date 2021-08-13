#ifndef TKN_TASK_BATCH_WRITE_H
#define TKN_TASK_BATCH_WRITE_H

#include <leveldb/write_batch.h>

#include "pool.h"
#include "task/task.h"

namespace token{
  class BatchWriteTask : public task::Task{
  protected:
    leveldb::WriteBatch batch_;

    BatchWriteTask(task::TaskEngine* engine, task::Task* parent):
      task::Task(engine, parent),
      batch_(){}
    explicit BatchWriteTask(task::TaskEngine* engine):
      task::Task(engine),
      batch_(){}
  public:
    ~BatchWriteTask() override = default;
  };

  class PoolBatchWriteTask : public BatchWriteTask{
  protected:
    PoolBatchWriteTask(task::TaskEngine* engine, task::Task* parent):
      BatchWriteTask(engine, parent){}
    explicit PoolBatchWriteTask(task::TaskEngine* engine):
      BatchWriteTask(engine){}

#define DEFINE_PUT_POOL_OBJECT(Name) \
    inline void \
    Put##Name(const Hash& key, const std::shared_ptr<Name>& val){ \
      return ObjectPool::Put##Name##Object(&batch_, key, val);     \
    }
#define DEFINE_REMOVE_POOL_OBJECT(Name) \
    inline void                         \
    Remove##Name(const Hash& key){ \
      return ObjectPool::Delete##Name##Object(&batch_, key); \
    }
#define DEFINE_POOL_TYPE_METHODS(Name) \
    DEFINE_PUT_POOL_OBJECT(Name)       \
    DEFINE_REMOVE_POOL_OBJECT(Name)

    FOR_EACH_POOL_TYPE(DEFINE_POOL_TYPE_METHODS)
#undef DEFINE_POOL_TYPE_METHODS
#undef DEFINE_PUT_POOL_OBJECT
  public:
    ~PoolBatchWriteTask() override = default;
  };
}

#endif//TKN_TASK_BATCH_WRITE_H