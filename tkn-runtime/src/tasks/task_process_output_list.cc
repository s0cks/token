#include <glog/logging.h>

#include "key.h"
#include "transaction_unclaimed.h"
#include "task_process_transaction.h"
#include "task_process_output_list.h"

namespace token{
  namespace task{
    ProcessOutputListTask::ProcessOutputListTask(ProcessTransactionTask* parent, const Hash& hash, const int64_t& index, OutputList& outputs):
      task::Task(parent),
      outputs_(outputs),
      transaction_(hash),
      index_(index){}

    void ProcessOutputListTask::DoWork(){
      DVLOG(2) << "processing output list....";
      for(auto& it : outputs_){
        auto val = UnclaimedTransaction::NewInstance(transaction_, index_++, it.GetUser(), it.GetProduct());
        auto hash = val->hash();
        DVLOG(2) << "indexing unclaimed transaction " << hash << "....";
        ObjectKey key(val->type(), hash);
        BufferPtr data = val->ToBuffer();
        batch_.Put((const leveldb::Slice&)key, data->AsSlice());
      }

      DVLOG(2) << "queueing batch (" << batch_.ApproximateSize() << "b)....";
      auto parent = (ProcessTransactionTask*)GetParent();
      auto& write_queue = parent->GetWriteQueue();
      write_queue.push_front(&batch_);//TODO: memory-leak
    }
  }
}