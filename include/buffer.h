#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include "object.h"

namespace Token{
    class Buffer : public Object{
    private:
        intptr_t size_;
        intptr_t wpos_;
        intptr_t rpos_;
        uint8_t data_[1];

        Buffer(intptr_t size):
            Object(),
            size_(size),
            data_(){
            SetType(Type::kBufferType);
        }

        template<typename T>
        void Append(T value){
            memcpy(&data_[wpos_], (uint8_t*) &value, sizeof(T));
            wpos_ += sizeof(T);
        }

        template<typename T>
        void Insert(T value, intptr_t idx){
            memcpy(&data_[idx], (uint8_t*)&value, sizeof(T));
            wpos_ = idx + sizeof(T);
        }

        template<typename T>
        T Read(intptr_t idx) {
            if((intptr_t(idx + sizeof(T)) > GetBufferSize()))
                return 0;
            return (*((T*) &data_[idx]));
        }

        template<typename T>
        T Read() {
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }
    protected:
        static void* operator new(size_t size) = delete;
        static void* operator new(size_t size, size_t length, bool){
            return Object::operator new(size + (sizeof(uint8_t) * length));
        }
        static void operator delete(void*, size_t, bool){}
        using Object::operator delete;
    public:
        ~Buffer() = default;

        uint8_t operator[](intptr_t idx){
            return data_[idx];
        }

        char* data() const{
            return (char*)data_;
        }

        intptr_t GetBufferSize() const{
            return size_;
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

        void PutString(const std::string& value){
            memcpy(&data_[wpos_], value.data(), value.length());
            wpos_ += value.length();
        }

        void Initialize(uv_buf_t* buff){
            buff->len = size_;
            buff->base = data();
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << "Buffer(" << GetBufferSize() << ")";
            return ss.str();
        }

        static Handle<Buffer> NewInstance(intptr_t size){
            return new (size, false)Buffer(size);
        }
    };
}

#endif //TOKEN_BUFFER_H
