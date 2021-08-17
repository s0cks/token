#ifndef TKN_TASK_H
#define TKN_TASK_H

#include <iostream>
#include <glog/logging.h>

#include "atomic/relaxed_atomic.h"
#include "task/task_queue.h"

namespace token{
  namespace task{
#define FOR_EACH_TASK_STATUS(V) \
  V(Unqueued)                   \
  V(Queued)                     \
  V(Successful)                 \
  V(Failed)                     \
  V(Cancelled)                  \
  V(TimedOut)

    class TaskEngine;
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
      TaskEngine* engine_;
      Task* parent_;
      atomic::RelaxedAtomic<Status> status_;
      atomic::RelaxedAtomic<uint64_t> unfinished_;

      Task(TaskEngine* engine, Task* parent):
        engine_(engine),
        parent_(parent),
        status_(Status::kUnqueued),
        unfinished_(1){
        if(parent != nullptr){
          parent->unfinished_ += 1;
        }
      }
      explicit Task(Task* parent):
        Task(parent->GetEngine(), parent){}
      explicit Task(TaskEngine* engine):
        Task(engine, nullptr){}

      void SetParent(Task* parent){
        parent_ = parent;
      }

      void SetState(const Status& status){
        status_ = status;
      }
     public:
      virtual ~Task() = default;

      TaskEngine* GetEngine() const{
        return engine_;
      }

      bool HasParent() const{
        return parent_ != nullptr;
      }

      Task* GetParent() const{
        return parent_;
      }

      Status GetStatus() const{
        return (Status)status_;
      }

#define DEFINE_CHECK(Name) \
      bool Is##Name() const{ return GetStatus() == Status::k##Name; }
      FOR_EACH_TASK_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK

      virtual void DoWork() = 0;//TODO: make const?
      virtual void OnFinished() const{}
      virtual std::string GetName() const = 0;

      bool Submit(TaskQueue& queue){
        return queue.Push(reinterpret_cast<uword>(this));
      }

      bool IsFinished() const{
        return unfinished_ == 0;
      }

      bool Finish(){
        unfinished_ -= 1;
        if(HasParent())
          return GetParent()->Finish();
        OnFinished();
        return true;
      }

      bool Run(){
        if(IsFinished())
          return false;
        DoWork();
        return Finish();
      }
    };
  }
}

#endif//TKN_TASK_H