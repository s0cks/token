#ifndef TOKEN_STATS_H
#define TOKEN_STATS_H

namespace Token{
    template<typename T>
    class Counter{
    private:
        T value_;
    public:
        Counter(const T& initial):
            value_(initial){}
        Counter(const Counter<T>& counter):
            value_(counter.Get()){}
        ~Counter() = default;

        T& Get(){
            return value_;
        }

        T Get() const{
            return value_;
        }

        void operator=(const Counter<T>& counter){
            value_ = counter.Get();
        }

        friend bool operator==(const Counter<T>& a, const Counter<T>& b){
            return a.value_ == b.value_;
        }

        friend bool operator==(const Counter<T>& a, const T& b){
            return a.value_ == b;
        }

        friend bool operator!=(const Counter<T>& a, const Counter<T>& b){
            return a.value_ != b.value_;
        }

        friend bool operator!=(const Counter<T>& a, const T& b){
            return a.value_ != b;
        }

        friend Counter<T>& operator++(){
            value_++;
            return (*this);
        }

        friend Counter<T>& operator+=(const Counter<T>& value){
            value_ += value.value_;
            return (*this);
        }

        friend Counter<T>& operator+=(const T& value){
            value_ += value;
            return (*this);
        }

        friend Counter<T>& operator--(){
            value_--;
            return (*this);
        }

        friend Counter<T>& operator-=(const Counter<T>& value){
            value_ -= value.value_;
            return (*this);
        }

        friend Counter<T>& operator-=(const T& value){
            value_ -= value;
            return (*this);
        }
    };
}

#endif //TOKEN_STATS_H