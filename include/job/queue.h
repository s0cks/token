#ifndef TOKEN_QUEUE_H
#define TOKEN_QUEUE_H

namespace Token{
  class JobQueue{
   private:
    std::vector<Job*> jobs_;
    std::atomic<size_t> top_;
    std::atomic<size_t> bottom_;
   public:
    JobQueue(size_t max_size):
      jobs_(max_size, nullptr),
      top_(0),
      bottom_(0){}
    ~JobQueue() = default;

    size_t GetSize() const{
      return bottom_.load(std::memory_order_seq_cst) - top_.load(std::memory_order_seq_cst);
    }

    bool IsEmpty() const{
      return GetSize() == 0;
    }

    bool Push(Job* job){
      size_t bottom = bottom_.load(std::memory_order_acquire);
      if(bottom < jobs_.size()){
        jobs_[bottom % jobs_.size()] = job;
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(bottom + 1, std::memory_order_release);
        return true;
      }
      return false;
    }

    Job* Steal(){
      size_t top = top_.load(std::memory_order_acquire);
      std::atomic_thread_fence(std::memory_order_acquire);
      size_t bottom = bottom_.load(std::memory_order_acquire);
      if(top < bottom){
        Job* job = jobs_[top % jobs_.size()];
        size_t next = top + 1;
        size_t desired_top = next;
        if(!top_.compare_exchange_weak(top, desired_top, std::memory_order_acq_rel))
          return nullptr;
        return job;
      }
      return nullptr;
    }

    Job* Pop(){
      size_t bottom = bottom_.load(std::memory_order_acquire);
      if(bottom > 0){
        bottom = bottom - 1;
        bottom_.store(bottom, std::memory_order_release);
      }

      std::atomic_thread_fence(std::memory_order_release);
      size_t top = top_.load(std::memory_order_acquire);
      if(top <= bottom){
        Job* job = jobs_[bottom % jobs_.size()];
        if(top == bottom){
          size_t expected = top;
          size_t next = top + 1;
          size_t desired = next;
          if(!top_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel)){
            job = nullptr;
          }
          bottom_.store(next, std::memory_order_release);
        }
        return job;
      }

      bottom_.store(top, std::memory_order_release);
      return nullptr;
    }
  };
}

#endif //TOKEN_QUEUE_H