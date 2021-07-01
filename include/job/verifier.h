#ifndef TOKEN_VERIFIER_H
#define TOKEN_VERIFIER_H

#include "job.h"
#include "block.h"
#include "transaction.h"

namespace token{
  class VerifyBlockJob;
  class VerifyTransactionJob;
  class VerifyTransactionInputsJob;
  class VerifyInputListJob;
  class VerifyTransactionOutputsJob;
  class VerifyOutputListJob;

  class VerifierJob : public Job{
   protected:
    VerifierJob(Job* parent, const std::string& name):
      Job(parent, name){}
   public:
    virtual ~VerifierJob() = default;
    virtual void AppendValid(const Hash& hash) = 0;
    virtual void AppendInvalid(const Hash& hash) = 0;
  };

  class VerifyBlockJob : public Job{
   private:
    BlockPtr block_;
   protected:
    JobResult DoWork();
   public:
    VerifyBlockJob(const BlockPtr& blk):
      Job(nullptr, "VerifyBlock"),
      block_(blk){}
    ~VerifyBlockJob() = default;

    BlockPtr& GetBlock(){
      return block_;
    }

    BlockPtr GetBlock() const{
      return block_;
    }
  };

  class VerifyTransactionJob : public Job{
   private:
    IndexedTransactionPtr transaction_;
   protected:
    JobResult DoWork();
   public:
    VerifyTransactionJob(VerifyBlockJob* parent, const IndexedTransactionPtr& tx):
      Job(parent, "VerifyTransactionJob"),
      transaction_(tx){}
    ~VerifyTransactionJob() = default;

    IndexedTransactionPtr GetTransaction() const{
      return transaction_;
    }
  };

  class VerifyTransactionInputsJob : public Job{
   private:
    InputList inputs_;
   protected:
    JobResult DoWork();
   public:
    VerifyTransactionInputsJob(VerifyTransactionJob* parent, const InputList& inputs):
      Job(parent, "VerifyTransactionInputsJob"),
      inputs_(inputs){}
    ~VerifyTransactionInputsJob() = default;

    IndexedTransactionPtr GetTransaction() const{
      return ((VerifyTransactionJob*) GetParent())->GetTransaction();
    }
  };

  class VerifyInputListJob : public Job{
   public:
    static const int64_t kMaxNumberOfInputs = 128;
   private:
    InputList inputs_;
    InputList valid_;
    InputList invalid_;
   protected:
    JobResult DoWork();
   public:
    VerifyInputListJob(VerifyTransactionInputsJob* parent, const InputList& inputs):
      Job(parent, "VerifyInputListJob"),
      inputs_(inputs),
      valid_(),
      invalid_(){}
    ~VerifyInputListJob() = default;

    InputList& GetValid(){
      return valid_;
    }

    InputList GetValid() const{
      return valid_;
    }

    InputList& GetInvalid(){
      return invalid_;
    }

    InputList GetInvalid() const{
      return invalid_;
    }
  };

  class VerifyTransactionOutputsJob : public Job{
   private:
    OutputList outputs_;
   protected:
    JobResult DoWork();
   public:
    VerifyTransactionOutputsJob(VerifyTransactionJob* parent, const OutputList& outputs):
      Job(parent, "VerifyTransactionOutputsJob"),
      outputs_(outputs){}
    ~VerifyTransactionOutputsJob() = default;

    IndexedTransactionPtr GetTransaction() const{
      return ((VerifyTransactionJob*) GetParent())->GetTransaction();
    }
  };

  class VerifyOutputListJob : public Job{
   public:
    static const int64_t kMaxNumberOfOutputs = 128;
   protected:
    OutputList outputs_;

    JobResult DoWork();
   public:
    VerifyOutputListJob(VerifyTransactionOutputsJob* parent, const OutputList& outputs):
      Job(parent, "VerifyOutputListJob"),
      outputs_(outputs){}
    ~VerifyOutputListJob() = default;
  };
}

#endif //TOKEN_VERIFIER_H