#ifndef TKN_TASK_COMMIT_INPUT_H
#define TKN_TASK_COMMIT_INPUT_H

#include "task/task.h"
#include "transaction_input.h"

namespace token{
  class CommitTransactionInputsTask;
  class CommitInputTask : public task::Task{
  private:
    InputPtr value_;
  public:
    CommitInputTask(CommitTransactionInputsTask* parent, const InputPtr& val);
    ~CommitInputTask() override = default;

    std::string GetName() const override{
      return "CommitInputTask()";
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_COMMIT_INPUT_H