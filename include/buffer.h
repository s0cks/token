#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <set>
#include <memory>
#include <vector>
#include <fstream>
#include <leveldb/slice.h>

#include "uuid.h"
#include "user.h"
#include "hash.h"
#include "version.h"
#include "product.h"
#include "proposal.h"
#include "timestamp.h"
#include "transaction_reference.h"

namespace token{
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
      if((wpos_ + T::GetSize()) > GetBufferSize()){
        DLOG(ERROR) << "cannot put Type, not enough room for " << T::GetSize() << " bytes in buffer of size: " << GetBufferSize();
        return false;
      }
      memcpy(&raw()[wpos_], val.data(), T::GetSize());
      wpos_ += T::GetSize();
      return true;
    }

    template<class T>
    T GetType(){
      T val(&raw()[rpos_], T::GetSize());
      rpos_ += T::GetSize();
      return val;
    }
   public:
    Buffer():
      bsize_(0),
      wpos_(0),
      rpos_(0),
      data_(nullptr){}
    explicit Buffer(int64_t size):
      bsize_(size),
      wpos_(0),
      rpos_(0),
      data_(nullptr),
      owned_(true){
      data_ = (uint8_t*) malloc(sizeof(uint8_t) * size);
      memset(data(), 0, GetBufferSize());
    }
    Buffer(uint8_t* data, int64_t size):
      bsize_(size),
      wpos_(size),
      rpos_(0),
      data_(data),
      owned_(false){}
    Buffer(const char* data, size_t size):
      bsize_(size),
      wpos_(size),
      rpos_(0),
      data_((uint8_t*)data),
      owned_(false){}
    explicit Buffer(const std::string& data):
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

    int64_t GetBufferSize() const{
      return bsize_;
    }

    int64_t GetWrittenBytes() const{
      return wpos_;
    }

    int64_t GetReadBytes() const{
      return rpos_;
    }

    uint8_t operator[](intptr_t idx){
      return (raw()[idx]);
    }

    char* data() const{
      return (char*)raw();
    }

    uint8_t* begin() const{
      return (uint8_t*)raw();
    }

    uint8_t* end() const{
      return (uint8_t*)raw() + GetBufferSize();
    }

    bool empty() const{
      return bsize_ == 0;
    }

    void clear(){
      memset(data(), 0, GetBufferSize());
      rpos_ = 0;
      wpos_ = 0;
    }

    int64_t GetBytesRemaining() const{
      return GetBufferSize() - GetWrittenBytes();
    }

    bool HasBytesRemaining() const{
      return GetBufferSize() > 0 && rpos_ < GetBufferSize();
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
    bool Put##Name(const Type##_t& val){ return Append<Type##_t>(val); } \
    bool Put##Name(const Type##_t& val, int64_t pos){ return Insert<Type##_t>(val, pos); }
#define DEFINE_PUT_UNSIGNED(Name, Type) \
    bool PutUnsigned##Name(const u##Type##_t& val){ return Append<u##Type##_t>(val); } \
    bool PutUnsigned##Name(const u##Type##_t& val, int64_t pos){ return Insert<u##Type##_t>(val, pos); }
#define DEFINE_PUT(Name, Type) \
    DEFINE_PUT_SIGNED(Name, Type) \
    DEFINE_PUT_UNSIGNED(Name, Type)

    FOR_EACH_NATIVE_TYPE(DEFINE_PUT);
#undef DEFINE_PUT
#undef DEFINE_PUT_SIGNED
#undef DEFINE_PUT_UNSIGNED

#define DEFINE_GET_SIGNED(Name, Type) \
    Type##_t Get##Name(){ return Read<Type##_t>(); } \
    Type##_t Get##Name(int64_t pos){ return Read<Type##_t>(pos); }
#define DEFINE_GET_UNSIGNED(Name, Type) \
    u##Type##_t GetUnsigned##Name(){ return Read<u##Type##_t>(); } \
    u##Type##_t GetUnsigned##Name(int64_t pos){ return Read<u##Type##_t>(pos); }
#define DEFINE_GET(Name, Type) \
    DEFINE_GET_SIGNED(Name, Type) \
    DEFINE_GET_UNSIGNED(Name, Type)

    FOR_EACH_NATIVE_TYPE(DEFINE_GET);
#undef DEFINE_GET
#undef DEFINE_GET_SIGNED
#undef DEFINE_GET_UNSIGNED

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

    template<class T, class C>
    bool PutSet(const std::set<T, C>& items){
      if(!PutLong(items.size()))
        return false;
      for(auto& item : items){
        if(!item.Write(shared_from_this()))
          return false;
      }
      return true;
    }

    bool PutReference(const TransactionReference& ref){
      return PutHash(ref.transaction())
          && PutLong(ref.index());
    }

    bool PutAddress(const NodeAddress& val){
      return PutInt(val.address())
          && PutInt(val.port());
    }

    NodeAddress GetAddress(){
      return NodeAddress(GetInt(), GetInt());
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

    bool PutPeerList(const PeerList& peers){
      if(!PutLong(peers.size())){
        DLOG(ERROR) << "cannot encode PeerList size.";
        return false;
      }
      for(auto& it : peers){
        if(!PutAddress(it)){
          DLOG(ERROR) << "cannot encode PeerList address: " << it;
          return false;
        }
      }
      return true;
    }

    bool GetPeerList(PeerList& peers){
      int64_t size = GetLong();
      for(int64_t idx = 0; idx < size; idx++){
        if(!peers.insert(GetAddress()).second){
          DLOG(ERROR) << "cannot decode PeerList element #" << idx;
          return false;
        }
      }
      return true;
    }

    template<class T, class D>
    bool GetList(std::vector<T>& results, const D& decoder){
      int64_t length = GetLong();
      for(int64_t idx = 0; idx < length; idx++){
        T value;
        if(!decoder.Decode(shared_from_this(), value)){
          DLOG(FATAL) << "cannot decode list element #" << idx;
          return false;
        }
        results.push_back(value);
      }
      return true;
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
    CopyFrom(const char* data, size_t len){
      BufferPtr buffer = Buffer::NewInstance(len);
      if(!buffer->PutBytes((uint8_t*)data, len)){
        DLOG(WARNING) << "couldn't copy " << len << " bytes to new buffer.";
        return nullptr;
      }
      return buffer;
    }

    static inline BufferPtr
    CopyFrom(const std::string& data){
      return CopyFrom(data.data(), data.length());
    }

    static inline BufferPtr
    CopyFrom(const json::String& data){
      return CopyFrom(data.GetString(), data.GetSize());
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
    FromFile(const std::string& filename){
      std::fstream stream(filename, std::ios::binary|std::ios::in);
      int64_t size = GetFilesize(stream);
      BufferPtr buffer = Buffer::NewInstance(size);
      if(!buffer->ReadFrom(stream, size))
        return nullptr;
      return buffer;
    }
  };
}

#define SERIALIZE_BASIC_FIELD(Field, Type) \
  if(!buff->Put##Type(Field)){                 \
    LOG(WARNING) << "cannot serialize field " << #Field << " (" << #Type << ")"; \
    return false;                                \
  }

#define SERIALIZE_FIELD(Name, Type, Field) \
  if(!(Field).Write(buff)){                \
    LOG(WARNING) << "cannot serialize field " << #Name << " (" << #Type << ")"; \
    return false;                          \
  }

#define SERIALIZE_POINTER_FIELD(Name, Type, Field) \
  if(!(Field)->Write(buff)){   \
    LOG(WARNING) << "cannot serialize field " << #Name << " (" << #Type << ")"; \
    return false;              \
  }

#endif //TOKEN_BUFFER_H
