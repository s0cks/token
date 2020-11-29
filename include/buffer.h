#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include "object.h"
#include "user.h"
#include "product.h"

namespace Token{
    class Buffer : public Object{
    private:
        intptr_t bsize_;
        intptr_t wpos_;
        intptr_t rpos_;
#ifdef TOKEN_GCMODE_NONE
        uint8_t* data_;
#endif//TOKEN_GCMODE_NONE

        uint8_t* raw() {
#ifdef TOKEN_GCMODE_NONE
            return data_;
#else
            return (uint8_t*)(&rpos_ + sizeof(intptr_t));
#endif//TOKEN_GCMODE_NONE
        }

        const uint8_t* raw() const{
#ifdef TOKEN_GCMODE_NONE
            return data_;
#else
            return (uint8_t*)(&rpos_ + sizeof(intptr_t));
#endif//TOKEN_GCMODE_NONE
        }

        Buffer(intptr_t size):
            Object(),
            bsize_(size),
            wpos_(0),
            rpos_(0){
            SetType(Type::kBufferType);
#ifdef TOKEN_GCMODE_NONE
            data_ = (uint8_t*)malloc(sizeof(uint8_t)*size);
#endif//TOKEN_GCMODE_NONE
            memset(data(), 0, GetBufferSize());
        }

        template<typename T>
        void Append(T value){
            memcpy(&raw()[wpos_], &value, sizeof(T));
            wpos_ += sizeof(T);
        }

        template<typename T>
        void Insert(T value, intptr_t idx){
            memcpy(&raw()[idx], (uint8_t*)&value, sizeof(T));
            wpos_ = idx + (intptr_t)sizeof(T);
        }

        template<typename T>
        T Read(intptr_t idx) {
            if((idx + (intptr_t)sizeof(T)) > GetBufferSize()) {
                LOG(INFO) << "cannot read " << sizeof(T) << " bytes from pos: " << idx;
                return 0;
            }
            return *(T*)(raw() + idx);
        }

        template<typename T>
        T Read() {
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }
    protected:
#ifndef TOKEN_GCMODE_NONE
        static void* operator new(size_t size) = delete;
        static void* operator new(size_t size, size_t length, bool){
            intptr_t buffer_size = (sizeof(uint8_t)*length);
            intptr_t total_size = size + buffer_size;
            return Allocator::AllocateObject(total_size);
        }
        static void operator delete(void*, size_t, bool){}
        using Object::operator delete;
#endif//!TOKEN_GCMODE_NONE
    public:
        ~Buffer() = default;

        uint8_t operator[](intptr_t idx){
            return (raw()[idx]);
        }

        char* data() const{
            return (char*)raw();
        }

        intptr_t GetBufferSize() const{
            return bsize_;
        }

        intptr_t GetWrittenBytes() const{
            return wpos_;
        }

        intptr_t GetReadBytes() const{
            return rpos_;
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
        void WriteBytesTo(std::fstream& stream, intptr_t size){
            uint8_t bytes[size];
            GetBytes(bytes, size);
            if(!stream.write((char*)bytes, size)){
                LOG(WARNING) << "cannot read " << size << " bytes from file";
                return;
            }

            if(!stream.flush()){
                LOG(WARNING) << "cannot flush the file";
                return;
            }
        }

        void ReadBytesFrom(std::fstream& stream, intptr_t size){
            uint8_t bytes[size];
            if(!stream.read((char*)bytes, size)){
                LOG(WARNING) << "cannot read " << size << " bytes from file";
                return;
            }
            PutBytes(bytes, size);
        }

        void PutBytes(uint8_t* bytes, intptr_t size){
            memcpy(&raw()[wpos_], bytes, size);
            wpos_ += size;
        }

        bool GetBytes(uint8_t* result, intptr_t size){
            if((rpos_ + size) >= GetBufferSize()) {
                LOG(WARNING) << "cannot read " << size << " bytes from buffer of size: " << GetBufferSize();
                return false;
            }
            memcpy(result, &raw()[rpos_], size);
            rpos_ += size;
            return true;
        }

        void PutUser(const User& user) {
            memcpy(&raw()[wpos_], user.data(), User::kSize);
            wpos_ += User::kSize;
        }

        User GetUser(){
            User user(&raw()[rpos_]);
            rpos_ += User::kSize;
            return user;
        }

        void PutProduct(const Product& product){
            memcpy(&raw()[wpos_], product.data(), Product::kSize);
            wpos_ += Product::kSize;
        }

        Product GetProduct(){
            Product product(&raw()[rpos_]);
            rpos_ += Product::kSize;
            return product;
        }

        void PutHash(const Hash& value){
            memcpy(&raw()[wpos_], value.data(), Hash::kSize);
            wpos_ += Hash::kSize;
        }

        Hash
        GetHash(){
            Hash hash(&raw()[rpos_]);
            rpos_ += Hash::kSize;
            return hash;
        }

        void PutString(const std::string& value){
            memcpy(&raw()[wpos_], value.data(), value.length());
            wpos_ += value.length();
        }

        void Reset(){
            memset(data(), 0, GetBufferSize());
            rpos_ = 0;
            wpos_ = 0;
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << "Buffer(" << GetBufferSize() << ")";
            return ss.str();
        }

        static Handle<Buffer> NewInstance(intptr_t size){
            size = RoundUpPowTwo(size);
#ifdef TOKEN_GCMODE_NONE
            return new Buffer(size);
#else
            return new (size, false) Buffer(size);
#endif//TOKEN_GCMODE_NONE
        }
    };
}

#endif //TOKEN_BUFFER_H
