#ifndef TOKEN_ARRAY_H
#define TOKEN_ARRAY_H

#include "common.h"

namespace Token{
    template<typename Type>
    class Array{
    public:
        typedef Type* iterator;
        typedef const Type* const_iterator;
    private:
        uintptr_t length_;
        uintptr_t capacity_;
        Type* data_;

        void Resize(uintptr_t new_len){
            if(new_len > capacity_){
                uintptr_t new_cap = RoundUpPowTwo(new_len);
                Type* new_data = reinterpret_cast<Type*>(realloc(data_, new_cap * sizeof(Type)));
                data_ = new_data;
                capacity_ = new_cap;
            }
            length_ = new_len;
        }
    public:
        explicit Array(uintptr_t init_cap=10):
                length_(0),
                capacity_(0),
                data_(nullptr){
            if(init_cap > 0){
                capacity_ = RoundUpPowTwo(init_cap);
                data_ = reinterpret_cast<Type*>(malloc(capacity_ * sizeof(Type)));
            }
        }
        Array(const Array<Type>& other){
            length_ = other.length_;
            capacity_ = other.capacity_;
            data_ = reinterpret_cast<Type*>(malloc(capacity_ * sizeof(Type)));
            memcpy(data_, other.data_, capacity_ * sizeof(Type));
        }
        ~Array(){
            if(capacity_ > 0){
                free(data_);
                capacity_ = length_ = 0;
            }
        }

        uintptr_t Length() const{
            return length_;
        }

        Type& operator[](uintptr_t idx) const{
            return data_[idx];
        }

        Type& Last() const{
            return operator[](length_ - 1);
        }

        Type& Pop(){
            Type& result = Last();
            length_--;
            return result;
        }

        void Add(const Type& value){
            Resize(Length() + 1);
            Last() = value;
        }

        void AddAll(const Array<Type>& values){
            Resize(Length() + values.Length());
            for(auto& it : values){
                Last() = it;
            }
        }

        void Clear(){
            length_ = 0;
        }

        bool IsEmpty() const{
            return Length() == 0;
        }

        bool operator==(const Array<Type>& b) const;
        bool operator!=(const Array<Type>& b) const;

        iterator begin(){
            return &data_[0];
        }

        const_iterator begin() const{
            return &data_[0];
        }

        iterator end(){
            return &data_[Length()];
        }

        const_iterator end() const{
            return &data_[Length()];
        }
    };

    template<typename Type>
    bool Array<Type>::operator!=(const Token::Array<Type>& b) const{
        return !operator==(b);
    }

    template<typename Type>
    bool Array<Type>::operator==(const Token::Array<Type>& b) const{
        if(Length() != b.Length()) return false;
        for(uintptr_t idx = 0; idx < Length(); idx++){
            if(data_[idx] != b.data_[idx]){
                return false;
            }
        }
        return true;
    }
}

#endif //TOKEN_ARRAY_H