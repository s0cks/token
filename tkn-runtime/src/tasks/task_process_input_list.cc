#include "tasks/task_process_input_list.h"
#include "tasks/task_process_transaction.h"

namespace token{
  namespace task{
    ProcessInputListTask::ProcessInputListTask(ProcessTransactionTask* parent, ObjectPool& pool, const Hash& transaction, const int64_t& index, std::vector<std::shared_ptr<Input>>& inputs):
      task::Task(parent),
      pool_(pool),
      batch_(),
      inputs_(inputs){}

    void ProcessInputListTask::DoWork(){
      DLOG(INFO) << "processing input list....";

      NOT_IMPLEMENTED(WARNING);
    }
  }
}