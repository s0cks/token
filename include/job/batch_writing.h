#ifndef TOKEN_JOB_BATCH_WRITING_H
#define TOKEN_JOB_BATCH_WRITING_H

#include <leveldb/write_batch.h>
#include "atomic/linked_list.h"

namespace token{
  class BatchCommit{
  private:
    leveldb::WriteBatch writes_;
  public:
    BatchCommit():
      writes_(){}
    BatchCommit(const BatchCommit& other) = default;
    ~BatchCommit() = default;

    BatchCommit& operator=(const BatchCommit& other){
      writes_ = other.writes_;
    }
  };

  class BatchCommitWriter{
  private:
    AtomicLinkedList<BatchCommit> commits_;
  public:
    BatchCommitWriter() = default;
    ~BatchCommitWriter() = default;

    void Commit(const leveldb::WriteBatch& writes){
      commits_.push_front(BatchCommit(writes));
    }
  };
}

#endif//TOKEN_JOB_BATCH_WRITING_H