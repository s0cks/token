#ifndef TOKEN_BYTEARRAY_H
#define TOKEN_BYTEARRAY_H

#include "common.h"

namespace Token{
    class ByteArray{
    private:
        uintptr_t length_;
        uintptr_t capacity_;
        uint8_t* data_;

        void Resize(uintptr_t nlen){
            if(nlen > capacity_){
                uintptr_t ncap = RoundUpPowTwo(nlen);
                uint8_t* ndata = (uint8_t*)realloc(data_, ncap * sizeof(uint8_t));
                data_ = ndata;
                capacity_ = ncap;
            }
            length_ = nlen;
        }

        friend class Message;
        friend class NodeServerSession;
        friend class NodeClientSession;
    public:
        ByteArray(uintptr_t init_capacity):
            length_(0),
            capacity_(0),
            data_(nullptr){
            if(init_capacity > 0){
                capacity_ = RoundUpPowTwo(init_capacity);
                data_ = (uint8_t*)malloc(sizeof(uint8_t) * capacity_);
            }
        }
        ~ByteArray(){
            if(capacity_ > 0){
                free(data_);
                capacity_ = length_ = 0;
            }
        }

        uint32_t GetUInt32(size_t idx){
            return data_[idx] |
                    (data_[idx + 1] << 8) |
                    (data_[idx + 2] << 16) |
                    (data_[idx + 3] << 24);
        }

        void PutUInt32(size_t idx, uint32_t val){
            data_[idx] = (val);
            data_[idx + 1] = (val >> 8);
            data_[idx + 2] = (val >> 16);
            data_[idx + 3] = (val >> 24);
        }

        uintptr_t Length() const{
            return length_;
        }

        uintptr_t Capacity() const{
            return capacity_;
        }

        uint8_t* Data() const{
            return data_;
        }

        uint8_t& operator[](size_t idx) const{
            return data_[idx];
        }

        void Clear(){
            memset(data_, 0, capacity_ * sizeof(uint8_t));
            length_ = 0;
        }

        bool IsEmpty() const{
            return Length() == 0;
        }

        bool operator==(const ByteArray& b) const{
            if(Length() != b.Length()) return false;
            for(size_t idx = 0; idx < Length(); idx++){
                if(data_[idx] != b.data_[idx]) return false;
            }
            return true;
        }

        bool operator!=(const ByteArray& b) const{
            return !operator==(b);
        }
    };
}

#endif //TOKEN_BYTEARRAY_H