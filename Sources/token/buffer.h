#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <set>
#include <memory>
#include <vector>
#include <fstream>
#include <leveldb/slice.h>

#include "token/hash.h"
#include "token/user.h"
#include "token/product.h"
#include "token/address.h"
#include "token/platform.h"
#include "token/timestamp.h"
#include "token/transaction_reference.h"

namespace token{
 class Buffer;
 typedef std::shared_ptr<Buffer> BufferPtr;

#define BUFFER_READ_POSITION "(" << rpos_ << "/" << length() << ")"
#define BUFFER_WRITE_POSITION "(" << wpos_ << "/" << length() << ")"

#define FOR_EACH_BUFFER_TYPE(V) \
 V(Byte)                        \
 V(Short)                       \
 V(Int)                         \
 V(Long)

 class Buffer : public std::enable_shared_from_this<Buffer>{
  protected:
   uint64_t wpos_;
   uint64_t rpos_;

   Buffer():
     wpos_(0),
     rpos_(0){
   }

   template<typename T>
   bool Append(T value){
     if((wpos_ + (uint64_t) sizeof(T)) > length())
       return false;
     memcpy(&data()[wpos_], &value, sizeof(T));
     wpos_ += sizeof(T);
     return true;
   }

   template<typename T>
   bool Insert(T value, uint64_t idx){
     if((idx + (uint64_t) sizeof(T)) > length())
       return false;
     memcpy(&data()[idx], (uint8_t*) &value, sizeof(T));
     wpos_ = idx + sizeof(T);
     return true;
   }

   template<typename T>
   T Read(uint64_t idx){
     if((idx + (uint64_t) sizeof(T)) > length()){
       LOG(ERROR) << "cannot read " << sizeof(T) << " bytes @" << idx << " from buffer.";
       return T();//TODO: investigate proper result
     }
     return *(T*) (data() + idx);
   }

   template<typename T>
   T Read(){
     T data = Read<T>(rpos_);
     rpos_ += sizeof(T);
     return data;
   }
  public:
   Buffer(const Buffer& rhs) = delete;
   virtual ~Buffer() = default;

   virtual uint8_t* data() const = 0;
   virtual uint64_t length() const = 0;

   uint64_t GetWritePosition() const{
     return wpos_;
   }

   void SetWritePosition(const uint64_t& pos){
     wpos_ = pos;
   }

   uint64_t GetReadPosition() const{
     return rpos_;
   }

   void SetReadPosition(const uint64_t& pos){
     rpos_ = pos;
   }

   bool empty() const{
     return IsUnallocated() || GetWritePosition() == 0;
   }

   bool IsUnallocated() const{
     return length() == 0;
   }

#define DEFINE_PUT_BUFFER_SIGNED_TYPE(Name) \
   inline bool Put##Name(Name val){ return Append<Name>(val); } \
   inline bool Put##Name(Name val, uint64_t pos){ return Insert<Name>(val, pos); }
#define DEFINE_PUT_BUFFER_UNSIGNED_TYPE(Name) \
   inline bool PutUnsigned##Name(Unsigned##Name val){ return Append<Unsigned##Name>(val); } \
   inline bool PutUnsigned##Name(Unsigned##Name val, uint64_t pos){ return Insert<Unsigned##Name>(val, pos); }
#define DEFINE_PUT_BUFFER_TYPE(Name) \
   DEFINE_PUT_BUFFER_SIGNED_TYPE(Name) \
   DEFINE_PUT_BUFFER_UNSIGNED_TYPE(Name)

#define DEFINE_GET_BUFFER_SIGNED_TYPE(Name) \
   inline Name Get##Name(){ return Read<Name>(); } \
   inline Name Get##Name(uint64_t pos){ return Read<Name>(pos); }
#define DEFINE_GET_BUFFER_UNSIGNED_TYPE(Name) \
   inline Unsigned##Name GetUnsigned##Name(){ return Read<Unsigned##Name>(); } \
   inline Unsigned##Name GetUnsigned##Name(uint64_t pos){ return Read<Unsigned##Name>(pos); }
#define DEFINE_GET_BUFFER_TYPE(Name) \
   DEFINE_GET_BUFFER_SIGNED_TYPE(Name) \
   DEFINE_GET_BUFFER_UNSIGNED_TYPE(Name)

   FOR_EACH_BUFFER_TYPE(DEFINE_PUT_BUFFER_TYPE)
   FOR_EACH_BUFFER_TYPE(DEFINE_GET_BUFFER_TYPE)
#undef DEFINE_PUT_BUFFER_TYPE
#undef DEFINE_PUT_BUFFER_SIGNED_TYPE
#undef DEFINE_PUT_BUFFER_UNSIGNED_TYPE

#undef DEFINE_GET_BUFFER_TYPE
#undef DEFINE_GET_BUFFER_SIGNED_TYPE
#undef DEFINE_GET_BUFFER_UNSIGNED_TYPE

   bool PutHash(const uint256& val){
     return PutBytes(val.data(), val.size());
   }

   template<class H>
   bool PutHash(const typename H::Storage& val){
     return PutBytes(val.data(), val.size());
   }

   inline bool
   PutSHA256(const uint256& val){
     return PutHash<sha256>(val);
   }

   template<class H>
   typename H::Storage GetHash(){
     uint8_t data[H::kSize];
     if(!GetBytes(data, H::kSize))
       return {};
     auto size = H::kSize;
     typename H::Storage Storage;
     return {(const uint8_t*)data, size};
   }

   inline uint256
   GetSHA256(){
     return GetHash<sha256>();
   }

   bool GetBytes(uint8_t* bytes, const uint64_t& size){
     if((rpos_ + size) > length())
       return false;
     memcpy(bytes, &data()[rpos_], size);
     rpos_ += size;
     return true;
   }

   bool PutBytes(const uint8_t* bytes, uint64_t size){
     if((wpos_ + size) > length()){
       DLOG(ERROR) << "buffer is too small.";
       return false;
     }
     memcpy(&data()[wpos_], bytes, size);
     wpos_ += size;
     return true;
   }

   bool PutBytes(const std::string& val){
     return PutBytes((uint8_t*) val.data(), static_cast<uint64_t>(val.length()));
   }

   bool PutBytes(const BufferPtr& buff){
     return PutBytes(buff->data(), buff->length());
   }

   bool PutString(const std::string& val){
     if(!PutUnsignedLong(val.length()))
       return false;
     return PutBytes(val);
   }

   bool PutTimestamp(const Timestamp& val){
     DLOG(INFO) << "put timestamp: " << FormatTimestampReadable(val);
     return PutUnsignedLong(ToUnixTimestamp(val));
   }

   Timestamp GetTimestamp(){
     auto val = FromUnixTimestamp(GetUnsignedLong());
     DLOG(INFO) << "parsed timestamp: " << FormatTimestampReadable(val);
     return val;
   }

   template<class T>
   bool PutBytes(const T& val){
     return PutBytes(val.data(), T::kSize);
   }

   template<class T>
   T Get(){
     uint8_t data[T::kSize];
     if(!GetBytes(data, T::kSize))
       return {};
     return { data, T::kSize };
   }

   inline bool
   PutUser(const User& val){
     return PutBytes<User>(val);
   }

   inline User
   GetUser(){
     return Get<User>();
   }

   inline bool
   PutProduct(const Product& val){
     return PutBytes<Product>(val);
   }

   inline Product
   GetProduct(){
     return Get<Product>();
   }

   inline bool
   PutTransactionReference(const TransactionReference& val){
     return PutSHA256(val.hash())
         && PutUnsignedLong(val.index());
   }

   inline TransactionReference
   GetTransactionReference(){
     auto hash = GetSHA256();
     auto index = GetUnsignedLong();
     return TransactionReference(hash, index);
   }

   leveldb::Slice AsSlice() const{
     return {(const char*) data(), length()};
   }

   bool WriteTo(FILE* file) const{
     if(!file)
       return false;
     if(fwrite(data(), sizeof(uint8_t), GetWritePosition(), file) != 0){
       LOG(FATAL) << "couldn't write buffer of size " << (sizeof(uint8_t) * GetWritePosition()) << "b to file: "
                  << strerror(errno);
       return false;
     }
     return true;
   }

   template<typename T>
   bool PutList(const T* list, uint64_t length){//TODO: unit test
     if(!PutUnsignedLong(length))
       return false;
     std::for_each(&list[0], &list[length], [&](const T& val){
       val.WriteTo(shared_from_this());
     });
     return true;
   }

   template<typename T>
   bool GetList(uint64_t* length, T** result){//TODO: unit test
     auto total = (*length) = GetUnsignedLong();
     DVLOG(2) << "parsed list length: " << total;
     auto data = (*result) = new T[total];
     for(auto idx = 0; idx < total; idx++)
       new (&data[idx])T(shared_from_this());
     return true;
   }

   virtual std::string ToString() const = 0;

   explicit operator leveldb::Slice() const{
     return AsSlice();
   }

   Buffer& operator=(const Buffer& other) = default;
 };

 template<const long& Size>
 class StackBuffer : public Buffer{
  protected:
   uint8_t data_[Size];
  public:
   StackBuffer():
       Buffer(),
       data_(){}
   StackBuffer(const StackBuffer& other) = default;
   ~StackBuffer() override = default;

   uint8_t* data() const override{
     return data_;
   }

   uint64_t length() const override{
     return Size;
   }

   std::string ToString() const override{
     std::stringstream ss;
     ss << "StackBuffer(";
     ss << "data=" << std::hex << data() << ",";
     ss << "length=" << length();
     ss << ")";
     return ss.str();
   }

   StackBuffer& operator=(const StackBuffer& other) = default;
 };

 class AllocatedBuffer : public Buffer{
  private:
   uint8_t* data_;
   uint64_t length_;
  public:
   AllocatedBuffer():
       AllocatedBuffer(0){}
   explicit AllocatedBuffer(const uint64_t& length)://TODO: RoundUpPow2(length)
       Buffer(),
       data_(nullptr),
       length_(length){
     if(length > 0){
       auto size = sizeof(uint8_t) * length;
       auto data = (uint8_t*) malloc(size);
       if(!data){
         DLOG(WARNING) << "cannot allocate internal buffer of size: " << size;
         return;
       }

       DVLOG(2) << "allocated new internal buffer of size: " << size;
       data_ = data;
       length_ = size;
     } else if(length <= 0){
       DVLOG(2) << "cannot allocate an empty buffer.";
     }
   }
   AllocatedBuffer(const AllocatedBuffer& other) = delete;
   ~AllocatedBuffer() override{
     if(data_){
       DVLOG(2) << "freeing buffer of size " << length_;
       free(data_);
     }
   }

   uint8_t* data() const override{
     return data_;
   }

   uint64_t length() const override{
     return length_;
   }

   std::string ToString() const override{
     std::stringstream ss;
     ss << "AllocatedBuffer(";
     ss << "data=" << std::hex << data_ << ",";
     ss << "length=" << length();
     ss << ")";
     return ss.str();
   }

   AllocatedBuffer& operator=(const AllocatedBuffer& other) = delete;
 };

 static inline BufferPtr
 NewBuffer(const uint64_t& length){
   return std::make_shared<AllocatedBuffer>(length);
 }

 template<class M>
 static inline BufferPtr
 NewBufferForProto(const M& msg){
   return NewBuffer(msg.ByteSizeLong() + sizeof(uint64_t));
 }

 template<class Encoder>
 static inline BufferPtr
 NewBufferFor(const Encoder& val){
   return NewBuffer(val.GetBufferSize());
 }

 static inline BufferPtr
 CopyBufferFrom(uint8_t* data, const uint64_t& length){
   auto buffer = NewBuffer(length);
   if(!buffer->PutBytes(data, length))
     return nullptr;
   return buffer;
 }

 static inline BufferPtr
 CopyBufferFrom(const std::string& slice){
   return CopyBufferFrom((uint8_t*) slice.data(), static_cast<uint64_t>(slice.length()));
 }

 static inline BufferPtr
 CopyBufferFrom(const leveldb::Slice& slice){
   return CopyBufferFrom((uint8_t*) slice.data(), slice.size());
 }

 template<const int64_t& Length>
 static inline BufferPtr
 CopyBufferFrom(const StackBuffer<Length>& buffer){
   return CopyBufferFrom(buffer.data(), buffer.length());
 }

 static inline BufferPtr
 NewBufferFromFile(FILE* file, const uint64_t& length){
   uint8_t data[length];
   if(fread(data, sizeof(uint8_t), length, file) != length){
     LOG(ERROR) << "cannot read " << length << " bytes from file into buffer.";
     return nullptr;
   }
   return CopyBufferFrom(data, length);
 }

 static inline BufferPtr
 NewBufferFromFile(const std::string& filename, const uint64_t& length){
   FILE* file;
   if(!(file = fopen(filename.data(), "rb"))){
     LOG(FATAL) << "cannot open file " << filename << ": " << strerror(errno);
     return nullptr;
   }
   return NewBufferFromFile(file, length);
 }
}

#endif //TOKEN_BUFFER_H
