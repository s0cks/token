#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <set>
#include <memory>
#include <vector>
#include <fstream>
#include <leveldb/slice.h>

#include "hash.h"
#include "address.h"
#include "timestamp.h"

namespace token{
  namespace internal{
#define FOR_EACH_BUFFER_TYPE(V) \
    V(Byte, int8)                 \
    V(Short, int16)               \
    V(Int, int32)                 \
    V(Long, int64)

    class Buffer;
    typedef std::shared_ptr<Buffer> BufferPtr;

#define BUFFER_READ_POSITION "(" << rpos_ << "/" << length() << ")"
#define BUFFER_WRITE_POSITION "(" << wpos_ << "/" << length() << ")"

    class Buffer{
     protected:
      uint64_t wpos_;
      uint64_t rpos_;

      Buffer():
        wpos_(0),
        rpos_(0){}

      template<typename T>
      bool Append(T value){
        if((wpos_ + (uint64_t)sizeof(T)) > length())
          return false;
        memcpy(&data()[wpos_], &value, sizeof(T));
        wpos_ += sizeof(T);
        return true;
      }

      template<typename T>
      bool Insert(T value, uint64_t idx){
        if((idx + (uint64_t)sizeof(T)) > length())
          return false;
        memcpy(&data()[idx], (uint8_t*)&value, sizeof(T));
        wpos_ = idx+sizeof(T);
        return true;
      }

      template<typename T>
      T Read(uint64_t idx){
        if((idx + (uint64_t)sizeof(T)) > length()){
          LOG(ERROR) << "cannot read " << sizeof(T) << " bytes @" << idx << " from buffer.";
          return T();//TODO: investigate proper result
        }
        return *(T*)(data() + idx);
      }

      template<typename T>
      T Read(){
        T data = Read<T>(rpos_);
        rpos_ += sizeof(T);
        return data;
      }
     public:
      Buffer(const Buffer& other) = default;
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

#define DEFINE_PUT_SIGNED(Name, Type) \
    bool Put##Name(const Type##_t& val){ return Append<Type##_t>(val); } \
    bool Put##Name(const Type##_t& val, uint64_t pos){ return Insert<Type##_t>(val, pos); }
#define DEFINE_PUT_UNSIGNED(Name, Type) \
    bool PutUnsigned##Name(const u##Type##_t& val){ return Append<u##Type##_t>(val); } \
    bool PutUnsigned##Name(const u##Type##_t& val, uint64_t pos){ return Insert<u##Type##_t>(val, pos); }
#define DEFINE_PUT(Name, Type) \
    DEFINE_PUT_SIGNED(Name, Type) \
    DEFINE_PUT_UNSIGNED(Name, Type)

    FOR_EACH_BUFFER_TYPE(DEFINE_PUT);
#undef DEFINE_PUT
#undef DEFINE_PUT_SIGNED
#undef DEFINE_PUT_UNSIGNED

#define DEFINE_GET_SIGNED(Name, Type) \
    Type##_t Get##Name(){ return Read<Type##_t>(); } \
    Type##_t Get##Name(uint64_t pos){ return Read<Type##_t>(pos); }
#define DEFINE_GET_UNSIGNED(Name, Type) \
    u##Type##_t GetUnsigned##Name(){ return Read<u##Type##_t>(); } \
    u##Type##_t GetUnsigned##Name(uint64_t pos){ return Read<u##Type##_t>(pos); }
#define DEFINE_GET(Name, Type) \
    DEFINE_GET_SIGNED(Name, Type) \
    DEFINE_GET_UNSIGNED(Name, Type)

    FOR_EACH_BUFFER_TYPE(DEFINE_GET);
#undef DEFINE_GET
#undef DEFINE_GET_SIGNED
#undef DEFINE_GET_UNSIGNED

      bool PutHash(const Hash& val){
        return PutBytes(val.data(), val.size());
      }

      Hash GetHash(){
        uint8_t data[Hash::GetSize()];
        if(!GetBytes(data, Hash::GetSize()))
          return Hash();
        return Hash(data, Hash::GetSize());
      }

      bool GetBytes(uint8_t* bytes, const uint64_t& size){
        if((rpos_ + size) > length())
          return false;
        memcpy(bytes, &data()[rpos_], size);
        rpos_ += size;
        return true;
      }

      bool PutBytes(const uint8_t* bytes, uint64_t size){
        if((wpos_ + size) > length())
          return false;
        memcpy(&data()[wpos_], bytes, size);
        wpos_ += size;
        return true;
      }

      bool PutBytes(const std::string& val){
        return PutBytes((uint8_t*)val.data(), static_cast<uint64_t>(val.length()));
      }

      bool PutBytes(const BufferPtr& buff){
        DLOG(INFO) << "put " << buff->length() << " bytes into buffer.";
        return PutBytes(buff->data(), buff->length());
      }

      bool PutString(const std::string& val){
        if(!PutUnsignedLong(val.length()))
          return false;
        return PutBytes(val);
      }

      bool PutTimestamp(const Timestamp& val){
        return PutUnsignedLong(ToUnixTimestamp(val));
      }

      Timestamp GetTimestamp(){
        return FromUnixTimestamp(GetUnsignedLong());
      }

      leveldb::Slice AsSlice() const{
        return leveldb::Slice((const char*)data(), length());
      }

      bool WriteTo(FILE* file) const{
        if(!file)
          return false;
        if(fwrite(data(), sizeof(uint8_t), GetWritePosition(), file) != 0){
          LOG(FATAL) << "couldn't write buffer of size " << (sizeof(uint8_t)*GetWritePosition()) << "b to file: " << strerror(errno);
          return false;
        }
        return true;
      }

      template<class M>
      bool PutMessage(const M& raw){
        auto mbytes = raw.ByteSizeLong();
        auto nbytes = mbytes+sizeof(uint64_t);
        if((wpos_ + nbytes) > length()){
          LOG(ERROR) << "not enough bytes remaining in buffer to serialize message (" << nbytes << "b) " << BUFFER_WRITE_POSITION;
          return false;
        }

        if(!PutUnsignedLong(mbytes)){
          LOG(ERROR) << "cannot serialize message length (uint64_t) to buffer " << BUFFER_WRITE_POSITION;
          return false;
        }

        if(!raw.SerializeToArray(&data()[wpos_], nbytes)){
          LOG(ERROR) << "cannot serialize message (" << mbytes << "b) to buffer " << BUFFER_WRITE_POSITION;
          return false;
        }
        wpos_ += nbytes;
        return true;
      }

      template<class M>
      bool GetMessage(M& raw, const uint64_t& nbytes){
        if((rpos_ + nbytes) > length()){
          DLOG(ERROR) << "not enough bytes remaining in buffer to deserialize message of size: " << nbytes << "b " << BUFFER_READ_POSITION;
          return false;
        }

        if(!raw.ParseFromArray(&data()[rpos_], nbytes)){
          DLOG(ERROR) << "cannot deserialize message of size " << nbytes << "b from buffer " << BUFFER_READ_POSITION;
          return false;
        }
        rpos_ += nbytes;
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
          auto size = sizeof(uint8_t)*length;
          auto data = (uint8_t*)malloc(size);
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
      AllocatedBuffer(const AllocatedBuffer& other) = default;
      ~AllocatedBuffer() override = default;

      uint8_t* data() const override{
        return data_;
      }

      uint64_t length() const override{
        return length_;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "AllocatedBuffer(";
        ss << "data=" << std::hex << data() << ",";
        ss << "length=" << length();
        ss << ")";
        return ss.str();
      }

      AllocatedBuffer& operator=(const AllocatedBuffer& other) = default;
    };

    static inline BufferPtr
    NewBuffer(const uint64_t& length){
      return std::make_shared<AllocatedBuffer>(length);
    }

    template<class M>
    static inline BufferPtr
    NewBufferForProto(const M& msg){
      return NewBuffer(msg.ByteSizeLong()+sizeof(uint64_t));
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
      return CopyBufferFrom((uint8_t *) slice.data(), static_cast<uint64_t>(slice.length()));
    }

    static inline BufferPtr
    CopyBufferFrom(const json::String& body){
      return CopyBufferFrom((uint8_t *) body.GetString(), static_cast<uint64_t>(body.GetSize()));
    }

    static inline BufferPtr
    CopyBufferFrom(const leveldb::Slice& slice){
      return CopyBufferFrom((uint8_t *) slice.data(), slice.size());
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
}

/* bool empty() const{
      return bsize_ == 0;
    }

    bool PutUUID(const UUID& val){ return PutType<UUID>(val); }
    bool PutVersion(const Version& val){ return PutType<Version>(val); }
    bool PutUser(const User& val){ return PutType<User>(val); }
    bool PutProduct(const Product& val){ return PutType<Product>(val); }
    bool PutHash(const Hash& val){ return PutType<Hash>(val); }

    UUID GetUUID(){ return GetType<UUID>(); }
    Version GetVersion(){ return GetType<Version>(); }
    User GetUser(){ return GetType<User>(); }
    Product GetProduct(){ return GetType<Product>(); }
    Hash GetHash(){ return GetType<Hash>(); }

    RawProposal GetProposal(){
      Timestamp timestamp = GetTimestamp();
      UUID proposal_id = GetUUID();
      UUID proposer_id = GetUUID();
      BlockHeader value = GetBlockHeader();
      return RawProposal(timestamp, proposal_id, proposer_id, value);
    }

    bool PutProposal(const RawProposal& proposal){
      return PutTimestamp(proposal.timestamp())
          && PutUUID(proposal.proposal_id())
          && PutUUID(proposal.proposer_id())
          && PutBlockHeader(proposal.value());
    }

    BlockHeader GetBlockHeader(){
      Timestamp timestamp = GetTimestamp();
      int64_t height = GetLong();
      Hash previous_hash = GetHash();
      Hash merkle_root = GetHash();
      Hash hash = GetHash();
      return BlockHeader(timestamp, height, previous_hash, merkle_root, hash);
    }

    bool PutBlockHeader(const BlockHeader& val){
      return PutTimestamp(val.timestamp())
          && PutLong(val.height())
          && PutHash(val.previous_hash())
          && PutHash(val.merkle_root())
          && PutHash(val.hash());
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

    bool WriteTo(FILE* file, int64_t size) const{
      if((int64_t)fwrite(data(), sizeof(uint8_t), size, file) != size){
        LOG(WARNING) << "cannot write " << size << " bytes to file";
        return false;
      }
      return true;
    }

    bool WriteTo(std::fstream& stream, int64_t size) const{
      if(!stream.write(data(), size)){
        LOG(WARNING) << "cannot write " << size << " bytes from file";
        return false;
      }

      if(!stream.flush()){
        LOG(WARNING) << "cannot flush the file";
        return false;
      }
      return true;
    }

    bool WriteTo(std::fstream& stream) const{
      return WriteTo(stream, GetWrittenBytes());
    }

    bool WriteTo(FILE* file) const{
      return WriteTo(file, GetWrittenBytes());
    }

    bool ReadFrom(std::fstream& stream, int64_t size){
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
    bool PutVector(const std::vector<T>& items){
      if(!PutLong(static_cast<int64_t>(items.size())))
        return false;
      for(auto& it : items){
        if(!it.Write(shared_from_this()))
          return false;
      }
      return true;
    }

    template<class T, class C>
    bool PutSetOf(const std::set<T, C>& items){
      //TODO: better naming
      if(!PutLong(items.size()))
        return false;

      for(auto& item : items){
        if(!item->Write(shared_from_this()))
          return false;
      }
      return true;
    }

    bool PutReference(const TransactionReference& ref){
      return PutHash(ref.transaction())
          && PutLong(ref.index());
    }



    TransactionReference GetReference(){
      Hash hash = GetHash();
      int64_t index = GetLong();
      return TransactionReference(hash, index);
    }

    bool PutTimestamp(const Timestamp& timestamp){
      return PutUnsignedLong(ToUnixTimestamp(timestamp));
    }

    Timestamp GetTimestamp(){
      return FromUnixTimestamp(GetUnsignedLong());
    }



    template<class T, class C>
    bool GetSet(std::set<T, C>& results){
      int64_t length = GetLong();
      for(int64_t idx = 0; idx < length; idx++){
        T value = T(shared_from_this());
        if(!results.insert(value).second){
          LOG(WARNING) << "couldn't decode item " << idx << "/" << length << " from buffer.";
          return false;
        }
      }
      return true;
    }

    explicit operator leveldb::Slice() const{
      return AsSlice();
    }

    leveldb::Slice AsSlice() const{
      return leveldb::Slice(data(), GetWrittenBytes());
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Buffer(" << GetBufferSize() << ")";
      return ss.str();
    }

    static inline BufferPtr
    NewInstance(int64_t size){
      return std::make_shared<Buffer>(size);
    }

    template<class E>
    static inline BufferPtr
    AllocateFor(const E& encoder){
      return NewInstance(encoder.GetBufferSize());
    }

    static inline BufferPtr
    From(uint8_t* data, int64_t size){
      return std::make_shared<Buffer>(data, size);
    }

    static inline BufferPtr
    From(const char* data, size_t size){
      return std::make_shared<Buffer>(data, size);
    }

    static inline BufferPtr
    From(const std::string& slice){
      return From(slice.data(), slice.size());
    }

    static inline BufferPtr
    From(const leveldb::Slice& slice){
      return From(slice.data(), slice.size());
    }

    static inline BufferPtr
    From(uv_buf_t* buff){
      return From(buff->base, buff->len);
    }

    static inline BufferPtr
    From(const uv_buf_t* buff){
      return From(buff->base, buff->len);
    }

    static inline BufferPtr
    CopyBufferFrom(const char* data, size_t len){
      BufferPtr buffer = Buffer::NewInstance(len);
      if(!buffer->PutBytes((uint8_t*)data, len)){
        DLOG(WARNING) << "couldn't copy " << len << " bytes to new buffer.";
        return nullptr;
      }
      return buffer;
    }

    static inline BufferPtr
    CopyBufferFrom(const std::string& data){
      return CopyBufferFrom(data.data(), data.length());
    }

    static inline BufferPtr
    CopyBufferFrom(const json::String& data){
      return CopyBufferFrom(data.GetString(), data.GetSize());
    }

    template<class T, class C>
    static inline int64_t
    CalculateSizeOf(const std::set<T, C>& items){
      int64_t size = 0;
      size += sizeof(int64_t); // length
      size += (items.size() * T::GetSize());
      return size;
    }

    static inline int64_t
    CalculateSizeOf(const std::string& str){
      return (str.length() * sizeof(uint8_t));
    }

    static inline BufferPtr
    NewBufferFromFile(const std::string& filename){
      std::fstream stream(filename, std::ios::binary|std::ios::in);
      int64_t size = GetFilesize(stream);
      BufferPtr buffer = Buffer::NewBuffer(size);
      if(!buffer->ReadFrom(stream, size))
        return nullptr;
      return buffer;
    }
  };*/

#endif //TOKEN_BUFFER_H
