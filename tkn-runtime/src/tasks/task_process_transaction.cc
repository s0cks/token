#include <glog/logging.h>
#include "tasks/task_process_transaction.h"

namespace token{
  namespace task{
    const size_t ProcessTransactionTask::kDefaultChunkSize = 128;

    void ProcessTransactionTask::DoWork(){
      DLOG(INFO) << "processing transaction " << hash() << "....";
      ProcessOutputs<kDefaultChunkSize>();
    }
  }
}