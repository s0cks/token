#ifndef TKN_TASK_VERIFY_TRANSACTION_OBJECTS_H
#define TKN_TASK_VERIFY_TRANSACTION_OBJECTS_H

#include "task/task.h"
#include "object_pool.h"

namespace token{
  namespace internal{
    template<class T>
    class VerifyTransactionObjectsTask : public task::Task{
    public:
      struct Stats{
        Timestamp started;
        Timestamp finished;
        uint64_t total;
        uint64_t processed;
        uint64_t valid;
        uint64_t invalid;

        Stats(const Timestamp& start,
            const Timestamp& stop,
            const uint64_t& total_inputs,
            const uint64_t& processed_inputs,
            const uint64_t& valid_inputs,
            const uint64_t& invalid_inputs):
            started(start),
            finished(stop),
            total(total_inputs),
            processed(processed_inputs),
            valid(valid_inputs),
            invalid(invalid_inputs){
        }
        Stats(const Stats& rhs) = default;
        ~Stats() = default;

        double GetProcessedPercentage() const{
          return GetPercentageOf(total, processed);
        }

        double GetValidPercentage() const{
          return GetPercentageOf(processed, valid);
        }

        double GetInvalidPercentage() const{
          return GetPercentageOf(processed, invalid);
        }

        Stats& operator=(const Stats& rhs) = default;
      };

      inline friend std::ostream&
      operator<<(std::ostream& stream, const Stats& stats){
        stream << "total=" << stats.total << ", ";
        stream << "processed=" << stats.processed << " (" << stats.GetProcessedPercentage() << "%), ";
        stream << "valid=" << stats.valid << " (" << stats.GetValidPercentage() << "%), ";
        stream << "invalid=" << stats.invalid << " (" << stats.GetInvalidPercentage() << "%)";
        return stream;
      }
    protected:
      Hash hash_;
      IndexedTransactionPtr transaction_;
      Timestamp start_;
      Timestamp finished_;
      uint64_t total_;
      atomic::RelaxedAtomic<uint64_t> processed_;
      atomic::RelaxedAtomic<uint64_t> valid_;
      atomic::RelaxedAtomic<uint64_t> invalid_;
      UnclaimedTransactionPool& pool_;

      VerifyTransactionObjectsTask(task::TaskEngine* engine, task::Task* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
          task::Task(engine, parent),
          hash_(val->hash()),
          transaction_(IndexedTransaction::CopyFrom(val)),
          start_(Clock::now()),
          finished_(Clock::now()),
          total_(val->GetNumberOfOutputs()),
          processed_(0),
          valid_(0),
          invalid_(0),
          pool_(pool){}
      VerifyTransactionObjectsTask(task::Task* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
          VerifyTransactionObjectsTask(parent->GetEngine(), parent, pool, val){}

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      ~VerifyTransactionObjectsTask() override = default;

      IndexedTransactionPtr transaction() const{
        return transaction_;
      }

      Hash hash() const{
        return hash_;
      }

      uint64_t total() const{
        return total_;
      }

      uint64_t processed() const{
        return (uint64_t)processed_;
      }

      uint64_t valid() const{
        return (uint64_t)valid_;
      }

      uint64_t invalid() const{
        return (uint64_t)invalid_;
      }

      Stats GetStats() const{
        return Stats(Clock::now(), Clock::now(), total(), processed(), valid(), invalid());
      }
    };
  }
}

#endif//TKN_TASK_VERIFY_TRANSACTION_OBJECTS_H