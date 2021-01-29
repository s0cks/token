#ifndef TOKEN_RELAXED_ATOMIC_H
#define TOKEN_RELAXED_ATOMIC_H

#include <atomic>

namespace Token{
  template<typename T>
  class RelaxedAtomic{
   private:
    std::atomic<T> val_;
   public:
    RelaxedAtomic(const T& init):
      val_(init){}
    ~RelaxedAtomic() = default;

    T load(std::memory_order order=std::memory_order_relaxed) const{
      return val_.load(order);
    }

    void store(T arg, std::memory_order order=std::memory_order_relaxed){
      val_.store(arg, order);
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order order=std::memory_order_relaxed){
      return val_.compare_exchange_weak(expected, desired, order, order);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order=std::memory_order_relaxed){
      return val_.compare_exchange_strong(expected, desired, order, order);
    }

    operator T() const{
      return load();
    }

    T operator=(T arg){
      store(arg);
      return arg;
    }

    T operator=(const RelaxedAtomic& arg){
      T loaded = arg;
      store(loaded);
      return loaded;
    }
  };
}

#endif//TOKEN_RELAXED_ATOMIC_H