#ifndef TKN_TASK_BATCH_WRITE_H
#define TKN_TASK_BATCH_WRITE_H

#include <leveldb/write_batch.h>
#include "task/task.h"

namespace token{
  class BatchWriteTask : public task::Task{
  protected:
    leveldb::WriteBatch batch_;
  public:
    BatchWriteTask(task::TaskEngine* engine, task::Task* parent):
      task::Task(engine, parent),
      batch_(){}
    explicit BatchWriteTask(task::TaskEngine* engine):
      task::Task(engine),
      batch_(){}
    ~BatchWriteTask() override = default;
  };
}

#endif//TKN_TASK_BATCH_WRITE_H