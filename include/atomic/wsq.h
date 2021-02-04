#ifndef TOKEN_WORK_STEALING_QUEUE_H
#define TOKEN_WORK_STEALING_QUEUE_H

namespace token{
  template<typename T>
  class WorkStealingQueue{
   private:
    struct Page{
      int64_t C;
      int64_t M;
      std::atomic<T>* S;

      explicit Page(int64_t c):
        C(c),
        M(c-1),
        S(new std::atomic<T>[C]){}
      ~Page(){
        delete[] S;
      }

      int64_t GetCapacity() const{
        return C;
      }

      void Push(int64_t idx, T val){
        S[idx & M].store(std::forward<T>(val), std::memory_order_relaxed);
      }

      T Pop(int64_t idx){
        return S[idx & M].load(std::memory_order_relaxed);
      }

      Page* Resize(int64_t bottom, int64_t top){
        Page* page = new Page(C*2);
        for(int64_t i = top; i != bottom; i++)
          page->Push(i, Pop(i));
        return page;
      }
    };

    std::atomic<int64_t> top_;
    std::atomic<int64_t> bottom_;
    std::atomic<Page*> data_;
    std::vector<Page*> garbage_;
   public:
    explicit WorkStealingQueue(int64_t cap):
      top_(),
      bottom_(),
      data_(),
      garbage_(){
      top_.store(0, std::memory_order_relaxed);
      bottom_.store(0, std::memory_order_relaxed);
      data_.store(new Page(cap), std::memory_order_relaxed);
      garbage_.reserve(32);
    }
    ~WorkStealingQueue(){
      for(auto& it : garbage_)
        delete it;
      delete data_.load();
    }

    bool IsEmpty() const{
      int64_t bottom = bottom_.load(std::memory_order_relaxed);
      int64_t top = top_.load(std::memory_order_relaxed);
      return bottom <= top;
    }

    int64_t GetSize() const{
      int64_t bottom = bottom_.load(std::memory_order_relaxed);
      int64_t top = top_.load(std::memory_order_relaxed);
      return bottom >= top ? bottom - top : 0;
    }

    template<typename U>
    bool Push(U&& val){
      int64_t bottom = bottom_.load(std::memory_order_relaxed);
      int64_t top = bottom_.load(std::memory_order_acquire);
      Page* data = data_.load(std::memory_order_relaxed);
      if(data->GetCapacity() - 1 < (bottom - top)){
        Page* tmp = data->Resize(bottom, top);
        garbage_.push_back(data);
        std::swap(data, tmp);
        data_.store(data, std::memory_order_relaxed);
      }

      data->Push(bottom, std::forward<U>(val));
      std::atomic_thread_fence(std::memory_order_release);
      bottom_.store(bottom + 1, std::memory_order_relaxed);
      return true;
    }

    T Pop(){
      int64_t bottom = bottom_.load(std::memory_order_relaxed) - 1;
      Page* data = data_.load(std::memory_order_relaxed);
      bottom_.store(bottom, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_seq_cst);
      int64_t top = top_.load(std::memory_order_relaxed);

      T item = nullptr;
      if(top <= bottom){
        item = data->Pop(bottom);
        if(top == bottom){
          if(!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            item = nullptr;
          bottom_.store(bottom + 1, std::memory_order_relaxed);
        }
      } else{
        bottom_.store(bottom + 1, std::memory_order_relaxed);
      }
      return item;
    }

    T Steal(){
      int64_t top = top_.load(std::memory_order_acquire);
      std::atomic_thread_fence(std::memory_order_seq_cst);
      int64_t bottom = bottom_.load(std::memory_order_acquire);

      T item = nullptr;
      if(top < bottom){
        Page* data = data_.load(std::memory_order_consume);
        item = data->Pop(top);
        if(!top_.compare_exchange_strong(top, top+1, std::memory_order_seq_cst, std::memory_order_relaxed))
          return nullptr;
      }
      return item;
    }

    int64_t GetCapacity() const{
      return data_.load(std::memory_order_relaxed)->GetCapacity();
    }
  };
}

#endif //TOKEN_WORK_STEALING_QUEUE_H