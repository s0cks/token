#ifndef TOKEN_BYTES_H
#define TOKEN_BYTES_H

#include <cstdint>
#include <cstring>
#include <string>
#include "uint256_t.h"

#include "object.h"

//TODO:
// - research benefits of signed vs unsigned
namespace Token{
    class ByteBuffer{
        //TODO:
        // - convert to object instance
    private:
        uint8_t* data_;
        size_t cap_;
        size_t wpos_;
        size_t rpos_;

        void Resize(size_t size){
            if(size > cap_){
                uint8_t* ndata = (uint8_t*)realloc(data_, sizeof(uint8_t)*size);
                if(!ndata){
                    LOG(WARNING) << "couldn't resize buffer to size: " << size;
                    return;
                }
                data_ = ndata;
                cap_ = size;
            }
        }

        template<typename T>
        void Append(T value){
            if((wpos_ + sizeof(T)) > GetCapacity())
                Resize(wpos_ + sizeof(T));
            memcpy(&data_[wpos_], (uint8_t*)&value, sizeof(T));
            wpos_ += sizeof(T);
        }

        template<typename T>
        void Insert(T value, size_t idx){
            if((idx + sizeof(T)) > GetCapacity())
                Resize(GetCapacity() + sizeof(T));
            memcpy(&data_[idx], (uint8_t*)&value, sizeof(T));
            wpos_ = idx + sizeof(T);
        }

        template<typename T>
        T Read(size_t idx){
            if((idx + sizeof(T)) > GetCapacity())
                return 0;
            return (*((T*)&data_[idx]));
        }

        template<typename T>
        T Read(){
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }
    public:
        ByteBuffer(size_t max):
            data_(nullptr),
            cap_(0),
            rpos_(0),
            wpos_(0){
            if(max > 0){
                max = RoundUpPowTwo(max);
                if(!(data_ = (uint8_t*)malloc(sizeof(uint8_t)*max))){
                    LOG(WARNING) << "couldn't allocate new buffer of size: " << max;
                }
                memset(data_, 0, sizeof(uint8_t)*max);
            }
            cap_ = max;
        }
        ByteBuffer(uint8_t* data, size_t size):
            data_(nullptr),
            cap_(0),
            rpos_(0),
            wpos_(0){
            if(size > 0){
                if(!(data_ = (uint8_t*)malloc(sizeof(uint8_t)*size))){
                    LOG(WARNING) << "couldn't malloc new buffer of size: " << size;
                    return;
                }
                memcpy(data_, data, sizeof(uint8_t)*size);
                cap_ = size;
            } else{
                LOG(WARNING) << "allocating empty buffer, requested size: " << size;
            }
        }
        ~ByteBuffer(){
            if(data_) free(data_);
        }

#define DEFINE_ENCODER(Name, Type) \
        void Put##Name(Type value){ \
            Append<Type>(value); \
        } \
        void Put##Name(Type value, size_t idx){ \
            Insert<Type>(value, idx); \
        }
        DEFINE_ENCODER(Byte, uint8_t);
        DEFINE_ENCODER(Short, uint16_t);
        DEFINE_ENCODER(Int, uint32_t);
        DEFINE_ENCODER(Long, uint64_t);
#undef DEFINE_ENCODER

#define DEFINE_DECODER(Name, Type) \
        Type Get##Name(){ \
            return Read<Type>(); \
        } \
        Type Get##Name(size_t idx){ \
            return Read<Type>(idx); \
        }
        DEFINE_DECODER(Byte, uint8_t);
        DEFINE_DECODER(Short, uint16_t);
        DEFINE_DECODER(Int, uint32_t);
        DEFINE_DECODER(Long, uint64_t);
#undef DEFINE_DECODER
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

        void PutHash(const uint256_t& value){
            if((wpos_ + uint256_t::kSize) >= GetCapacity())
                Resize(GetCapacity() + wpos_ + uint256_t::kSize);
            memcpy(&data_[wpos_], value.data(), uint256_t::kSize);
            wpos_ += uint256_t::kSize;
        }

        uint256_t
        GetHash(){
            uint256_t hash;
            memcpy(hash.data(), &data_[rpos_], uint256_t::kSize);
            rpos_ += uint256_t::kSize;
            return hash;
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

#endif //TOKEN_BYTES_H