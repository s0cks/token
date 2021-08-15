#ifndef TKN_ARRAY_H
#define TKN_ARRAY_H

#include "common.h"

namespace token{
  template<typename T>
  class Array{
  private:
    uint64_t length_;
    uint64_t capacity_;
    T* data_;

    inline void
    Resize(const uint64_t& new_capacity){
      length_ = new_capacity;//?????????
      if(new_capacity < capacity_){
        DLOG(ERROR) << "cannot resize array of size " << capacity_ << " to " << new_capacity;
        return;
      }
      new_capacity = RoundUpPowTwo(new_capacity);
      T* new_data = nullptr;
      if(!(new_data = reinterpret_cast<T*>(realloc(data_, new_capacity*sizeof(T))))){
        DLOG(ERROR) << "cannot reallocate array of size " << capacity_ << " to " << new_capacity;
        return;
      }
      data_ = new_data;
      capacity_ = new_capacity;
    }
  public:
    explicit Array(const uint64_t& init_capacity):
      length_(0),
      capacity_(0),
      data_(nullptr){
      if(init_capacity > 0){
        capacity_ = RoundUpPowTwo(init_capacity);
        data_ = reinterpret_cast<T*>(malloc(capacity_*sizeof(T)));
      }
    }
    ~Array(){
      if(data_)
        free(data_);
    }

    uint64_t Length() const{
      return length_;
    }

    T& operator[](const uint64_t& index) const{
      return data_[index];
    }

    T& Last() const{
      return operator[](length_ - 1);
    }

    T& Pop() {
      T& result = Last();
      length_--;
      return result;
    }

    void Add(const T& value){
      Resize(Length() + 1);
      Last() = value;
    }

    void Clear(){
      length_ = 0;
    }

    bool IsEmpty() const{
      return Length() == 0;
    }
  };
}

#endif//TKN_ARRAY_H