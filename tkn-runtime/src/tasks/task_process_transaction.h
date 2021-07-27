#ifndef TKN_TASK_PROCESS_TRANSACTION_H
#define TKN_TASK_PROCESS_TRANSACTION_H

#include <leveldb/write_batch.h>

#include "task/task.h"
#include "task/task_engine.h"
#include "transaction_indexed.h"
#include "task_process_input_list.h"
#include "task_process_output_list.h"
#include "atomic/linked_list.h"

namespace token{
  namespace task{
    typedef leveldb::WriteBatch WriteOperation;

    class ProcessTransactionTask : public task::Task{
    public:
      static const size_t kDefaultChunkSize;
    private:
      IndexedTransactionPtr value_;
      atomic::LinkedList<WriteOperation>& writes_;

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
          auto task = new Processor(this, hash(), index, chunk);
          DLOG(INFO) << "processing " << chunk.size() << " items....";
          if(!queue.Push((reinterpret_cast<uword>(task)))){
            LOG(FATAL) << "cannot push new task to task queue.";
            return;//TODO: better error handling
          }
          current = next;
        }
      }

      inline void
      ProcessInputs(const InputList& list, const size_t& chunk_size){
        DLOG(INFO) << "processing " << list.size() << " inputs....";
        return ProcessParallel<Input, ProcessInputListTask>(list, chunk_size);
      }

      inline void
      ProcessOutputs(const OutputList& list, const size_t& chunk_size){
        DLOG(INFO) << "processing " << list.size() << " outputs....";
        return ProcessParallel<Output, ProcessOutputListTask>(list, chunk_size);
      }
    public:
      explicit ProcessTransactionTask(TaskEngine* engine, atomic::LinkedList<WriteOperation>& writes, const IndexedTransactionPtr& val):
        task::Task(engine),
        value_(val),
        writes_(writes){}
      ~ProcessTransactionTask() override = default;

      std::string GetName() const override{
        return "ProcessTransactionTask";
      }

      Hash hash() const{
        return value_->hash();
      }

      InputList& inputs(){
        return value_->inputs();
      }

      OutputList& outputs(){
        return value_->outputs();
      }

      atomic::LinkedList<WriteOperation>&
      GetWriteQueue(){
        return writes_;
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_TRANSACTION_H