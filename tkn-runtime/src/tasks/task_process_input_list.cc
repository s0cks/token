#include "task_process_transaction.h"
#include "task_process_input_list.h"

namespace token{
  namespace task{
    ProcessInputListTask::ProcessInputListTask(ProcessTransactionTask* parent, const Hash& transaction, const int64_t& index, InputList& inputs):
      task::Task(parent),
      inputs_(inputs){}

    void ProcessInputListTask::DoWork(){
      DLOG(INFO) << "processing input list....";

      NOT_IMPLEMENTED(WARNING);
    }
  }
}