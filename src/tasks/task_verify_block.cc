#include "tasks/task_verify_block.h"

namespace token{
  namespace task{
    void VerifyBlockTask::DoWork(){
      LOG(INFO) << "verifying block " << GetBlock()->hash() << "....";
      SetState(Status::kSuccessful);
      return;
    }
  }
}