#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include <uuid/uuid.h>
#include <leveldb/slice.h>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "hash.h"
#include "utils/bitfield.h"

namespace Token{
#define FOR_EACH_RAW_TYPE(V) \
  V(Byte, int8_t)            \
  V(Short, int16_t)          \
  V(Int, int32_t)            \
  V(Long, int64_t)

#define FOR_EACH_MESSAGE_TYPE(V) \
    V(Version) \
    V(Verack) \
    V(Prepare) \
    V(Promise) \
    V(Commit) \
    V(Accepted) \
    V(Rejected) \
    V(GetData) \
    V(GetBlocks) \
    V(Block) \
    V(Transaction) \
    V(Inventory) \
    V(NotFound) \
    V(GetPeers) \
    V(PeerList)

#define FOR_EACH_SERIALIZABLE_TYPE(V) \
  V(Hash)                             \
  V(UUID)                             \
  V(User)                             \
  V(Product)

#define FOR_EACH_BASIC_TYPE(V) \
  FOR_EACH_SERIALIZABLE_TYPE(V) \
  V(Input)                     \
  V(Output)

#define FOR_EACH_POOL_TYPE(V) \
  V(Block)                    \
  V(Transaction)              \
  V(UnclaimedTransaction)

#define FOR_EACH_TYPE(V) \
  FOR_EACH_BASIC_TYPE(V) \
  FOR_EACH_POOL_TYPE(V)

  class Buffer;
  typedef std::shared_ptr<Buffer> BufferPtr;

  class Object{
   public:
    enum class Type{
#define DEFINE_TYPE(Name) k##Name##Type,
      FOR_EACH_TYPE(DEFINE_TYPE)
#undef DEFINE_TYPE
      kReferenceType, //TODO: refactor kReferenceType,
    };

    friend std::ostream& operator<<(std::ostream& stream, const Type& type){
      switch(type){
#define DEFINE_TOSTRING(Name) \
        case Object::Type::k##Name##Type: \
          return stream << #Name;
        FOR_EACH_TYPE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   public:
    virtual ~Object() = default;
    virtual std::string ToString() const = 0;
  };

  class BinaryFileWriter;
  class SerializableObject : public Object{
   protected:
    SerializableObject():
      Object(){}
   public:
    virtual ~SerializableObject() = default;

    virtual int64_t GetBufferSize() const = 0;
    virtual bool Write(const BufferPtr& buff) const = 0;
    virtual bool Write(Json::Writer& writer) const{ return false; }
    bool WriteToFile(BinaryFileWriter* writer) const;
  };

  class BinaryObject : public SerializableObject{
   protected:
    BinaryObject() = default;
   public:
    virtual ~BinaryObject() = default;
    Hash GetHash() const;
  };

  template<int64_t Size>
  class RawType{
   protected:
    uint8_t data_[Size];

    RawType():
      data_(){
      memset(data_, 0, GetSize());
    }
    RawType(const uint8_t* bytes, int64_t size):
      data_(){
      memset(data_, 0, GetSize());
      memcpy(data_, bytes, std::min(size, GetSize()));
    }
    RawType(const RawType& raw):
      data_(){
      memset(data_, 0, GetSize());
      memcpy(data_, raw.data_, GetSize());
    }

    static inline int
    Compare(const RawType<Size>& a, const RawType<Size>& b){
      return memcmp(a.data(), b.data(), Size);
    }
   public:
    virtual ~RawType() = default;

    char* data() const{
      return (char*) data_;
    }

    int64_t size() const{
      return std::min(strlen((char*)data_), (size_t)GetSize());
    }

    std::string str() const{
      return std::string(data(), size());
    }

    bool Write(Json::Writer& writer) const{
      return writer.String(data(), size());
    }

    static inline int64_t
    GetSize(){
      return Size;
    }
  };

  class Product : public RawType<64>{
    using Base = RawType<64>;
   public:
    Product():
      Base(){}
    Product(const uint8_t* bytes, int64_t size):
      Base(bytes, size){}
    Product(const Product& product):
      Base(){
      memcpy(data(), product.data(), Base::GetSize());
    }
    Product(const std::string& value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    ~Product() = default;

    void operator=(const Product& product){
      memcpy(data(), product.data(), Base::GetSize());
    }

    friend bool operator==(const Product& a, const Product& b){
      return Base::Compare(a, b) == 0;
    }

    friend bool operator!=(const Product& a, const Product& b){
      return Base::Compare(a, b) != 0;
    }

    friend int operator<(const Product& a, const Product& b){
      return Base::Compare(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Product& product){
      stream << product.str();
      return stream;
    }
  };

  class User : public RawType<64>{
    using Base = RawType<64>;
   public:
    User():
      Base(){}
    User(const uint8_t* bytes, int64_t size):
      Base(bytes, size){}
    User(const User& user):
      Base(){memcpy(data(), user.data(), Base::GetSize());
    }
    User(const std::string& value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    ~User() = default;

    void operator=(const User& user){
      memcpy(data(), user.data(), Base::GetSize());
    }

    friend bool operator==(const User& a, const User& b){
      return Base::Compare(a, b) == 0;
    }

    friend bool operator!=(const User& a, const User& b){
      return Base::Compare(a, b) != 0;
    }

    friend bool operator<(const User& a, const User& b){
      return Base::Compare(a, b) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const User& user){
      stream << user.str();
      return stream;
    }

    static int Compare(const User& a, const User& b){
      return Base::Compare(a, b);
    }
  };

  class UUID : public RawType<16>{
    using Base = RawType<64>;
   public:
    UUID():
      RawType(){
      uuid_generate_time_safe(data_);
    }
    UUID(uint8_t* bytes, int64_t size):
      RawType(bytes, size){}
    UUID(const std::string& uuid):
      RawType(){
      uuid_parse(uuid.data(), data_);
    }
    UUID(const UUID& other):
      RawType(){
      uuid_copy(data_, other.data_);
    }
    ~UUID() = default;

    std::string ToString() const{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 37);
    }

    void operator=(const UUID& other){
      uuid_copy(data_, other.data_);
    }

    friend bool operator==(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) == 0;
    }

    friend bool operator!=(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) != 0;
    }

    friend bool operator<(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const UUID& uuid){
      return stream << uuid.ToString();
    }
  };

  class TransactionReference{
   public:
    static const int64_t kSize = Hash::kSize + sizeof(int64_t);
   private:
    Hash transaction_;
    int64_t index_;
   public:
    TransactionReference(const Hash& tx, int64_t index):
      transaction_(tx),
      index_(index){}
    TransactionReference(const TransactionReference& ref):
      transaction_(ref.transaction_),
      index_(ref.index_){}
    ~TransactionReference() = default;

    Hash GetTransactionHash() const{
      return transaction_;
    }

    int64_t GetIndex() const{
      return index_;
    }

    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
          && Json::SetField(writer, "hash", transaction_)
          && Json::SetField(writer, "index", index_)
          && writer.EndObject();
    }

    TransactionReference& operator=(const TransactionReference& ref){
      transaction_ = ref.transaction_;
      index_ = ref.index_;
      return (*this);
    }

    friend bool operator==(const TransactionReference& a, const TransactionReference& b){
      return a.transaction_ == b.transaction_ && a.index_ == b.index_;
    }

    friend bool operator!=(const TransactionReference& a, const TransactionReference& b){
      return !operator==(a, b);
    }

    friend bool operator<(const TransactionReference& a, const TransactionReference& b){
      if(a.transaction_ == b.transaction_)
        return a.index_ < b.index_;
      return a.transaction_ < b.transaction_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const TransactionReference& ref){
      return stream << ref.GetTransactionHash() << "[" << ref.GetIndex() << "]";
    }
  };
}

#endif //TOKEN_OBJECT_H