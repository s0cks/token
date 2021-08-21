#ifndef TKN_TASK_COMMIT_TRANSACTION_OBJECTS_H
#define TKN_TASK_COMMIT_TRANSACTION_OBJECTS_H

#include "task/task.h"
#include "object_pool.h"
#include "transaction_indexed.h"

namespace token{
  namespace internal{
    template<class T>
    class CommitTransactionObjectsTask : public task::Task{
      //TODO: add stats
    protected:
      Hash hash_;
      IndexedTransactionPtr transaction_;
      uint64_t total_;
      std::atomic<uint64_t> processed_;
      std::atomic<uint64_t> valid_;
      std::atomic<uint64_t> invalid_;
      UnclaimedTransactionPool& pool_;

      CommitTransactionObjectsTask(task::Task* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
        task::Task(parent),
        hash_(val->hash()),
        transaction_(val),
        total_(val->GetNumberOfOutputs()), //TODO: fix
        processed_(0),
        valid_(0),
        invalid_(0),
        pool_(pool){
      }

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      ~CommitTransactionObjectsTask() override = default;

      Hash hash() const{
        return hash_;
      }

      IndexedTransactionPtr transaction() const{
        return transaction_;
      }

      uint64_t total() const{
        return total_;
      }

      uint64_t processed() const{
        return processed_.load();
      }

      uint64_t valid() const{
        return valid_.load();
      }

      uint64_t invalid() const{
        return invalid_.load();
      }

      //TODO: GetStats
    };
  }
}

#endif//TKN_TASK_COMMIT_TRANSACTION_OBJECTS_H