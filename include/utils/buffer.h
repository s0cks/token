#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <set>
#include <vector>
#include "object.h"

namespace Token{
  class Buffer;
  typedef std::shared_ptr<Buffer> BufferPtr;

  class Buffer : public std::enable_shared_from_this<Buffer>{
   private:
    int64_t bsize_;
    int64_t wpos_;
    int64_t rpos_;
    uint8_t* data_;
    bool owned_;

    uint8_t* raw(){
      return data_;
    }

    const uint8_t* raw() const{
      return data_;
    }

    template<typename T>
    bool Append(T value){
      if((wpos_ + (int64_t)sizeof(T)) > GetBufferSize())
        return false;
      memcpy(&raw()[wpos_], &value, sizeof(T));
      wpos_ += sizeof(T);
      return true;
    }

    template<typename T>
    bool Insert(T value, int64_t idx){
      if((idx + (int64_t)sizeof(T)) > GetBufferSize())
        return false;
      memcpy(&raw()[idx], (uint8_t*)&value, sizeof(T));
      wpos_ = idx + (int64_t)sizeof(T);
      return true;
    }

    template<typename T>
    T Read(int64_t idx){
      if((idx + (int64_t)sizeof(T)) > GetBufferSize()){
        LOG(INFO) << "cannot read " << sizeof(T) << " bytes from pos: " << idx;
        return 0;
      }
      return *(T*)(raw() + idx);
    }

    template<typename T>
    T Read(){
      T data = Read<T>(rpos_);
      rpos_ += sizeof(T);
      return data;
    }

    template<class T>
    bool PutType(const T& val){
      if((wpos_ + T::kSize) > GetBufferSize())
        return false;
      memcpy(&raw()[wpos_], val.data(), T::kSize);
      wpos_ += T::kSize;
      return true;
    }

    template<class T>
    T GetType(){
      T val(&raw()[rpos_], T::kSize);
      rpos_ += T::kSize;
      return val;
    }
   public:
    Buffer(int64_t size):
      bsize_(size),
      wpos_(0),
      rpos_(0),
      data_(nullptr),
      owned_(true){
      data_ = (uint8_t*) malloc(sizeof(uint8_t) * size);
      memset(data(), 0, GetBufferSize());
    }
    Buffer(const char* data, size_t size):
      bsize_(size),
      wpos_(0),
      rpos_(0),
      data_((uint8_t*)data),
      owned_(false){}
    Buffer(const std::string& data):
      bsize_(data.size()),
      wpos_(data.size()),
      rpos_(0),
      data_(nullptr),
      owned_(false){
      data_ = (uint8_t*)data.data();
    }
    ~Buffer(){
      if(owned_ && data_)
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

    bool Resize(int64_t nsize){
      if(nsize <= GetBufferSize())
        return true;
      uint8_t* ndata;
      if(!(ndata = (uint8_t*)realloc(data_, nsize))){
        LOG(WARNING) << "couldn't reallocate buffer of size " << GetBufferSize() << " to: " << nsize;
        return false;
      }
      data_ = ndata;
      bsize_ = nsize;
      return true;
    }

#define DEFINE_PUT_SIGNED(Name, Type) \
    bool Put##Name(const Type& val){ return Append<Type>(val); } \
    bool Put##Name(const Type& val, int64_t pos){ return Insert<Type>(val, pos); }
#define DEFINE_PUT_UNSIGNED(Name, Type) \
    bool PutUnsigned##Name(const u##Type& val){ return Append<u##Type>(val); } \
    bool PutUnsigned##Name(const u##Type& val, int64_t pos){ return Insert<u##Type>(val, pos); }
#define DEFINE_PUT(Name, Type) \
    DEFINE_PUT_SIGNED(Name, Type) \
    DEFINE_PUT_UNSIGNED(Name, Type)

#define DEFINE_GET_SIGNED(Name, Type) \
    Type Get##Name(){ return Read<Type>(); } \
    Type Get##Name(int64_t pos){ return Read<Type>(pos); }
#define DEFINE_GET_UNSIGNED(Name, Type) \
    Type GetUnsigned##Name(){ return Read<u##Type>(); } \
    Type GetUnsigned##Name(int64_t pos){ return Read<u##Type>(pos); }
#define DEFINE_GET(Name, Type) \
    DEFINE_GET_SIGNED(Name, Type) \
    DEFINE_GET_UNSIGNED(Name, Type)
#define DEFINE_PUT_TYPE(Name) \
    bool Put##Name(const Name& val){ return PutType<Name>(val); }
#define DEFINE_GET_TYPE(Name) \
    Name Get##Name(){ return GetType<Name>(); }

    FOR_EACH_RAW_TYPE(DEFINE_PUT);
    FOR_EACH_RAW_TYPE(DEFINE_GET);
    FOR_EACH_SERIALIZABLE_TYPE(DEFINE_PUT_TYPE);
    FOR_EACH_SERIALIZABLE_TYPE(DEFINE_GET_TYPE);

#undef DEFINE_PUT_SIGNED
#undef DEFINE_PUT_UNSIGNED
#undef DEFINE_PUT_TYPE

#undef DEFINE_GET_SIGNED
#undef DEFINE_GET_UNSIGNED
#undef DEFINE_GET_TYPE

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

    bool PutBytes(uint8_t* bytes, int64_t size){
      if((wpos_ + size) > GetBufferSize())
        return false;
      memcpy(&raw()[wpos_], bytes, size);
      wpos_ += size;
      return true;
    }

    bool PutBytes(const BufferPtr& buff){
      return PutBytes(buff->data_, buff->wpos_);
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

    bool PutString(const std::string& value){
      if((wpos_ + value.length()) > (size_t)GetBufferSize())
        return false;
      memcpy(&raw()[wpos_], value.data(), value.length());
      wpos_ += value.length();
      return true;
    }

    std::string GetString(int64_t size){
      char data[size];
      memcpy(&raw()[rpos_], data, size);
      rpos_ += size;
      return std::string(data, size);
    }

    template<class T>
    void PutList(const std::vector<T>& items){
      PutLong(items.size());
      for(auto& item : items){
        item.Write(shared_from_this());
      }
    }

    template<class T, class C>
    bool PutSet(const std::set<T, C>& items){
      if(!PutLong(items.size())){
        return false;
      }
      for(auto& item : items){
        if(!item->Write(shared_from_this())){
          return false;
        }
      }
      return true;
    }

    bool PutReference(const TransactionReference& ref){
      return PutHash(ref.GetTransactionHash())
          && PutLong(ref.GetIndex());
    }

    TransactionReference GetReference(){
      Hash hash = GetHash();
      int64_t index = GetLong();
      return TransactionReference(hash, index);
    }

    void Reset(){
      memset(data(), 0, GetBufferSize());
      rpos_ = 0;
      wpos_ = 0;
    }

    void SetWritePosition(int64_t pos){
      if(pos > GetBufferSize()){
        return;
      }
      wpos_ = pos;
    }

    void SetReadPosition(int64_t pos){
      if(pos > GetBufferSize()){
        return;
      }
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

    static inline BufferPtr From(uv_buf_t* buff){
      return From(buff->base, buff->len);
    }

    static inline BufferPtr From(const uv_buf_t* buff){
      return From(buff->base, buff->len);
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
