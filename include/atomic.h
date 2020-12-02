#ifndef TOKEN_ATOMIC_H
#define TOKEN_ATOMIC_H

#include <atomic>

namespace Token{
    template<typename T>
    class Atomic{
    private:
        std::atomic<T> value_;
    public:
        Atomic():
            value_(){}
        ~Atomic() = default;

        T Load(const std::memory_order& order=std::memory_order_relaxed) const{
            return value_.load(order);
        }

        void Store(T value, const std::memory_order& order=std::memory_order_relaxed){
            value_.store(value, order);
        }

        bool CompareExchangeWeak(T& expected, T desired, const std::memory_order& order=std::memory_order_relaxed){
            return value_.compare_exchange_weak(expected, desired, order, order);
        }

        bool CompareExchangeStrong(T& expected, T desired, const std::memory_order& order=std::memory_order_relaxed){
            return value_.compare_exchange_strong(expected, desired, order, order);
        }
    };
}

#endif //TOKEN_ATOMIC_H