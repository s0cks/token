#ifndef TOKEN_BYTE_BUFFER_H
#define TOKEN_BYTE_BUFFER_H

#include <cstdint>
#include <cstring>
#include <string>
#include "Hash.h"

namespace Token {
    class ByteBuffer {
        //TODO:
        // - research benefits of signed vs unsigned
        // - convert to object instance
    private:
        uint8_t* data_;
        size_t cap_;
        size_t wpos_;
        size_t rpos_;

        void Resize(size_t size) {
            if (size > cap_) {
                uint8_t* ndata = (uint8_t*) realloc(data_, sizeof(uint8_t) * size);
                if (!ndata) {
                    LOG(WARNING) << "couldn't resize buffer to size: " << size;
                    return;
                }
                data_ = ndata;
                cap_ = size;
            }
        }

        template<typename T>
        void Append(T value) {
            if ((wpos_ + sizeof(T)) > GetCapacity())
                Resize(wpos_ + sizeof(T));
            memcpy(&data_[wpos_], (uint8_t*) &value, sizeof(T));
            wpos_ += sizeof(T);
        }

        template<typename T>
        void Insert(T value, size_t idx) {
            if ((idx + sizeof(T)) > GetCapacity())
                Resize(GetCapacity() + sizeof(T));
            memcpy(&data_[idx], (uint8_t*) &value, sizeof(T));
            wpos_ = idx + sizeof(T);
        }

        template<typename T>
        T Read(size_t idx) {
            if ((idx + sizeof(T)) > GetCapacity())
                return 0;
            return (*((T*) &data_[idx]));
        }

        template<typename T>
        T Read() {
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }
    public:
        ByteBuffer(size_t max=65536) :
                data_(nullptr),
                cap_(0),
                wpos_(0),
                rpos_(0) {
            if (max > 0) {
                max = RoundUpPowTwo(max);
                if (!(data_ = (uint8_t*) malloc(sizeof(uint8_t) * max))) {
                    LOG(WARNING) << "couldn't allocate new buffer of size: " << max;
                }
                memset(data_, 0, sizeof(uint8_t) * max);
            }
            cap_ = max;
        }

        ByteBuffer(uint8_t* data, size_t size) :
                data_(nullptr),
                cap_(0),
                wpos_(0),
                rpos_(0) {
            if (size > 0) {
                if (!(data_ = (uint8_t*) malloc(sizeof(uint8_t) * size))) {
                    LOG(WARNING) << "couldn't malloc new buffer of size: " << size;
                    return;
                }
                memcpy(data_, data, sizeof(uint8_t)*size);
                cap_ = size;
                wpos_ = size;
            } else {
                LOG(WARNING) << "allocating empty buffer, requested size: " << size;
            }
        }

        ~ByteBuffer() {
            if (data_) free(data_);
        }

//@format:off
#define DEFINE_PUT(Name, Type) \
        void Put##Unsigned##Name(u##Type value){ \
            Append<u##Type>(value); \
        } \
        void Put##Unsigned##Name(intptr_t pos, u##Type value){ \
            Insert<u##Type>(value, pos); \
        } \
        void Put##Name(Type value){ \
            Append<u##Type>(value); \
        } \
        void Put##Name(intptr_t pos, Type value){ \
            Insert<Type>(value, pos); \
        }
        DEFINE_PUT(Byte, int8_t)
        DEFINE_PUT(Short, int16_t)
        DEFINE_PUT(Int, int32_t)
        DEFINE_PUT(Long, int64_t)
#undef DEFINE_PUT

#define DEFINE_GET(Name, Type) \
        Type Get##Unsigned##Name(){ \
            return Read<u##Type > (); \
        } \
        Type Get##Unsigned##Name(intptr_t pos){ \
            return Read<u##Type > (pos); \
        } \
        Type Get##Name(){ \
            return Read<Type>(); \
        } \
        Type Get##Name(intptr_t pos){ \
            return Read<Type>(pos); \
        }
        DEFINE_GET(Byte, int8_t)
        DEFINE_GET(Short, int16_t)
        DEFINE_GET(Int, int32_t)
        DEFINE_GET(Long, int64_t)
#undef DEFINE_GET
//@format:on

        void PutBytes(uint8_t* bytes, size_t size){
            if((wpos_ + size) >= GetCapacity())
                Resize(GetCapacity() + size);
            memcpy(&data_[wpos_], bytes, size);
            wpos_ += size;
        }

        bool GetBytes(uint8_t* result, size_t size){
            if((rpos_ + size) >= GetCapacity()) return false;
            memcpy(result, &data_[rpos_], size);
            rpos_ += size;
            return true;
        }

        void PutString(const std::string& value){
            if((wpos_ + sizeof(uint32_t) + value.length()) >= GetCapacity())
                Resize(GetCapacity() + sizeof(uint32_t) + value.length());
            PutInt(value.length());
            memcpy(&data_[wpos_], value.data(), value.length());
            wpos_ += value.length();
        }

        std::string GetString(){
            uint32_t len = GetInt();
            uint8_t data[len];
            memcpy(data, &data_[rpos_], len);
            rpos_ += len;
            return std::string((char*)data, len);
        }

        void PutHash(const Hash& value){
            if((wpos_ + Hash::kSize) >= GetCapacity())
                Resize(GetCapacity() + Hash::kSize);
            memcpy(&data_[wpos_], value.data(), Hash::kSize);
            wpos_ += Hash::kSize;
        }

        Hash
        GetHash(){
            uint8_t bytes[Hash::kSize];
            memcpy(bytes, &data_[rpos_], Hash::kSize);
            rpos_ += Hash::kSize;
            return Hash::FromBytes(bytes);
        }

        size_t GetWrittenBytes() const{
            return wpos_;
        }

        size_t GetCapacity() const{
            return cap_;
        }

        uint8_t* data() const{
            return data_;
        }

        void operator=(const ByteBuffer& other){
            if(data_)free(data_);
        }
    };
}

#endif //TOKEN_BYTE_BUFFER_H