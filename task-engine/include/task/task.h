#ifndef TKN_TASK_H
#define TKN_TASK_H

#include <atomic>
#include <iostream>

namespace token{
  namespace task{
#define FOR_EACH_TASK_STATUS(V) \
  V(Unqueued)                   \
  V(Queued)                     \
  V(Successful)                 \
  V(Failed)                     \
  V(Cancelled)                  \
  V(TimedOut)

    class Task{
     public:
      enum class Status{
#define DEFINE_STATUS(Name) k##Name,
        FOR_EACH_TASK_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
      };

      friend std::ostream& operator<<(std::ostream& stream, const Status& status){
        switch(status){
#define DEFINE_TOSTRING(Name) \
          case Status::k##Name: \
            return stream << #Name;
          FOR_EACH_TASK_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
          default:
            return stream << "Unknown";
        }
      }
     protected:
      Task* parent_;
      std::atomic<Status> status_; //TODO: use RelaxedAtomic
      std::atomic<int64_t> unfinished_; //TODO: use RelaxedAtomic

      explicit Task(Task* parent):
        parent_(parent),
        status_(Status::kUnqueued),
        unfinished_(){
        unfinished_.store(1, std::memory_order_relaxed);
        if(parent != nullptr){

        }
      }

      void IncrementUnfinished(){

      }

      void DecrementUnfinished(){

      }

      void SetParent(Task* parent){
        parent_ = parent;
      }

      void SetStatus(const Status& status){
        status_.store(status, std::memory_order_relaxed);
      }
     public:
      virtual ~Task() = default;

      Task* GetParent() const{
        return parent_;
      }

      Status GetStatus() const{
        return status_.load(std::memory_order_relaxed);
      }

      virtual std::string GetName() const = 0;
    };
  }
}

#endif//TKN_TASK_H