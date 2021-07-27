#include <glog/logging.h>
#include "task/task_engine.h"
#include "task_process_transaction.h"

namespace token{
  namespace task{
    const size_t ProcessTransactionTask::kDefaultChunkSize = 128;

    void ProcessTransactionTask::DoWork(){
      DLOG(INFO) << "processing transaction " << hash() << "....";
      ProcessInputs(inputs(), kDefaultChunkSize);
      ProcessOutputs(outputs(), kDefaultChunkSize);
    }
  }
}