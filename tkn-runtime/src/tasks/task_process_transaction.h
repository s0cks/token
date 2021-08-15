#ifndef TKN_TASK_PROCESS_TRANSACTION_H
#define TKN_TASK_PROCESS_TRANSACTION_H

#include <leveldb/write_batch.h>

#include "task/task.h"
#include "task/task_engine.h"
#include "transaction_indexed.h"
#include "task_process_input_list.h"
#include "task_process_output_list.h"
#include "atomic/linked_list.h"

#include "tasks/task_batch_write.h"

namespace token{
  namespace task{
    class ProcessTransactionTask : public task::Task{
      friend class ProcessInputListTask;
      friend class ProcessOutputListTask;
    public:
      static const size_t kDefaultChunkSize;
    private:
      ObjectPool& pool_;
      internal::WriteBatch* batch_;
      IndexedTransactionPtr value_;
      atomic::RelaxedAtomic<uint64_t> processed_inputs_;
      atomic::RelaxedAtomic<uint64_t> processed_outputs_;

      template<class T, class Processor>
      inline void
      ProcessParallel(const std::vector<T>& list, const size_t& chunk_size){
        auto& queue = GetEngine()->GetWorker(platform::GetCurrentThreadId())->GetTaskQueue();

        int64_t index = 0;
        auto current = list.begin();
        auto end = list.end();
        while(current != end){
          auto next = std::distance(current, end) > chunk_size
                      ? current + chunk_size //TODO: investigate clang tidy issue
                      : end;
          std::vector<T> chunk(current, next);
          auto task = new Processor(this, pool_, hash(), index, chunk);
          if(!queue.Push((reinterpret_cast<uword>(task)))){
            LOG(FATAL) << "cannot push new task to task queue.";
            return;//TODO: better error handling
          }
          current = next;
        }
      }

      template<const size_t& ChunkSize>
      inline void ProcessOutputs(){
        DLOG(INFO) << "processing transaction " << hash() << " outputs....";
        std::vector<Output> outputs;
        value_->GetOutputs(outputs);
        return ProcessParallel<Output, ProcessOutputListTask>(outputs, ChunkSize);
      }
    public:
      explicit ProcessTransactionTask(TaskEngine* engine, ObjectPool& pool, const IndexedTransactionPtr& val):
        task::Task(engine),
        pool_(pool),
        value_(val),
        processed_inputs_(0),
        processed_outputs_(0){}
      ~ProcessTransactionTask() override = default;

      std::string GetName() const override{
        return "ProcessTransactionTask";
      }

      Hash hash() const{
        return value_->hash();
      }

      uint64_t GetNumberOfInputsProcessed() const{
        return (uint64_t)processed_inputs_;
      }

      uint64_t GetNumberOfOutputsProcessed() const{
        return (uint64_t)processed_outputs_;
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_TRANSACTION_H