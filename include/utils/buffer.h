#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <set>
#include <vector>
#include "object.h"
#include "object_tag.h"

namespace Token{
  class Buffer;
  typedef std::shared_ptr<Buffer> BufferPtr;

  class Buffer : public std::enable_shared_from_this<Buffer>{
   private:
    int64_t bsize_;
    int64_t wpos_;
    int64_t rpos_;
    uint8_t* data_;

    uint8_t* raw(){
      return data_;
    }

    const uint8_t* raw() const{
      return data_;
    }

    template<typename T>
    void Append(T value){
      memcpy(&raw()[wpos_], &value, sizeof(T));
      wpos_ += sizeof(T);
    }

    template<typename T>
    void Insert(T value, intptr_t idx){
      memcpy(&raw()[idx], (uint8_t*) &value, sizeof(T));
      wpos_ = idx + (intptr_t) sizeof(T);
    }

    template<typename T>
    T Read(intptr_t idx){
      if((idx + (intptr_t) sizeof(T)) > GetBufferSize()){
        LOG(INFO) << "cannot read " << sizeof(T) << " bytes from pos: " << idx;
        return 0;
      }
      return *(T*) (raw() + idx);
    }

    template<typename T>
    T Read(){
      T data = Read<T>(rpos_);
      rpos_ += sizeof(T);
      return data;
    }
   public:
    Buffer(int64_t size):
      bsize_(size),
      wpos_(0),
      rpos_(0),
      data_(nullptr){
      data_ = (uint8_t*) malloc(sizeof(uint8_t) * size);
      memset(data(), 0, GetBufferSize());
    }
    Buffer(const char* data, size_t size):
      bsize_(size),
      wpos_(size),
      rpos_(0),
      data_(nullptr){
      data_ = (uint8_t*)malloc(sizeof(uint8_t)*size);
      memcpy(data_, data, size);
    }
    ~Buffer(){
      if(data_)
        free(data_);
    }

    uint8_t operator[](intptr_t idx){
      return (raw()[idx]);
    }

    char* data() const{
      return (char*) raw();
    }

    uint8_t* begin() const{
      return (uint8_t*) raw();
    }

    uint8_t* end() const{
      return (uint8_t*) raw() + GetBufferSize();
    }

    int64_t GetBufferSize() const{
      return bsize_;
    }

    int64_t GetWrittenBytes() const{
      return wpos_;
    }

    int64_t GetBytesRemaining() const{
      return GetBufferSize() - GetWrittenBytes();
    }

    int64_t GetReadBytes() const{
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
      if(!stream.write((char*) bytes, size)){
        LOG(WARNING) << "cannot read " << size << " bytes from file";
        return;
      }

      if(!stream.flush()){
        LOG(WARNING) << "cannot flush the file";
        return;
      }
    }

    bool ReadBytesFrom(std::fstream& stream, int64_t size){
      if(GetBufferSize() < size){
        LOG(WARNING) << "cannot read " << size << " bytes into a buffer of size: " << GetBufferSize();
        return false;
      }

      if(!stream.read((char*) &data_[wpos_], size)){
        LOG(WARNING) << "cannot read " << size << " bytes from file";
        return false;
      }

      wpos_ += size;
      return true;
    }

    void PutBytes(uint8_t* bytes, int64_t size){
      memcpy(&raw()[wpos_], bytes, size);
      wpos_ += size;
    }

    bool PutBytes(const BufferPtr& buff){
      PutBytes(buff->data_, buff->wpos_);
      return true;
    }

    bool GetBytes(uint8_t* result, intptr_t size){
      if((rpos_ + size) > GetBufferSize()){
        LOG(WARNING) << "cannot read " << size << " bytes from buffer of size: " << GetBufferSize();
        return false;
      }
      memcpy(result, &raw()[rpos_], size);
      rpos_ += size;
      return true;
    }

    void PutUser(const User& user){
      memcpy(&raw()[wpos_], user.data(), User::GetSize());
      wpos_ += User::GetSize();
    }

    User GetUser(){
      User user(&raw()[rpos_], User::GetSize());
      rpos_ += User::GetSize();
      return user;
    }

    void PutProduct(const Product& product){
      memcpy(&raw()[wpos_], product.data(), Product::GetSize());
      wpos_ += Product::GetSize();
    }

    Product GetProduct(){
      Product product(&raw()[rpos_], Product::GetSize());
      rpos_ += Product::GetSize();
      return product;
    }

    void PutHash(const Hash& value){
      memcpy(&raw()[wpos_], value.data(), Hash::GetSize());
      wpos_ += Hash::GetSize();
    }

    Hash
    GetHash(){
      Hash hash(&raw()[rpos_], Hash::GetSize());
      rpos_ += Hash::GetSize();
      return hash;
    }

    void PutString(const std::string& value){
      memcpy(&raw()[wpos_], value.data(), value.length());
      wpos_ += value.length();
    }

    std::string GetString(int64_t size){
      char data[size];
      memcpy(&raw()[rpos_], data, size);
      rpos_ += size;
      return std::string(data, size);
    }

    ObjectTag GetObjectTag(){
      return ObjectTag(GetUnsignedLong());
    }

    void PutObjectTag(const ObjectTag& tag){
      PutUnsignedLong(tag.data());
    }

    template<class T>
    void PutList(const std::vector<T>& items){
      PutLong(items.size());
      for(auto& item : items){
        item.Write(shared_from_this());
      }
    }

    template<class T, class C>
    void PutSet(const std::set<T, C>& items){
      PutLong(items.size());
      for(auto& item : items){
        item->Write(shared_from_this());
      }
    }

    void Reset(){
      memset(data(), 0, GetBufferSize());
      rpos_ = 0;
      wpos_ = 0;
    }

    void SetWritePosition(int64_t pos){
      if(pos > GetBufferSize())
        return;
      wpos_ = pos;
    }

    void SetReadPosition(int64_t pos){
      if(pos > GetBufferSize())
        return;
      rpos_ = pos;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Buffer(" << GetBufferSize() << ")";
      return ss.str();
    }

    static inline BufferPtr NewInstance(int64_t size){
      return std::make_shared<Buffer>(size);
    }

    static inline BufferPtr From(const char* data, size_t size){
      return std::make_shared<Buffer>(data, size);
    }

    static inline BufferPtr From(const std::string& slice){
      return From(slice.data(), slice.size());
    }

    static inline BufferPtr From(const leveldb::Slice& slice){
      return From(slice.data(), slice.size());
    }

    static inline BufferPtr FromFile(const std::string& filename){
      std::fstream fd(filename, std::ios::in | std::ios::binary);
      size_t size = GetFilesize(fd);
      BufferPtr buff = NewInstance(size);
      if(!buff->ReadBytesFrom(fd, static_cast<int64_t>(size))){
        LOG(WARNING) << "couldn't read " << size << " bytes from: " << filename;
        return BufferPtr(nullptr);
      }
      return buff;
    }
  };
}

#endif //TOKEN_BUFFER_H
