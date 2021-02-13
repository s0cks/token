#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include <strings.h>
#include <uuid/uuid.h>
#include <leveldb/slice.h>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "hash.h"
#include "json.h"
#include "utils/bitfield.h"

namespace token{
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
  V(NotFound)

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

  enum class Type{
    kUnknown = 0,
#define DEFINE_TYPE(Name) k##Name,
    FOR_EACH_TYPE(DEFINE_TYPE)
#undef DEFINE_TYPE
    kReference,
    kBlockHeader,

#define DEFINE_MESSAGE_TYPE(Name) k##Name##Message,
    FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_TYPE)
#undef DEFINE_MESSAGE_TYPE

    kHttpRequest,
    kHttpResponse,
  };

  static std::ostream& operator<<(std::ostream& stream, const Type& type){
    switch(type){
#define DEFINE_TOSTRING(Name) \
        case Type::k##Name: \
          return stream << #Name;
      FOR_EACH_TYPE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
      case Type::kReference:
        return stream << "Reference";
      default:
        return stream << "Unknown";
    }
  }

  typedef int64_t RawObjectTag;

  class ObjectTag{
   public:
    static inline int
    CompareType(const ObjectTag& a, const ObjectTag& b){
      if(a.GetType() < b.GetType()){
        return -1;
      } else if(a.GetType() > b.GetType()){
        return +1;
      }
      return 0;
    }

    static inline int
    CompareSize(const ObjectTag& a, const ObjectTag& b){
      if(a.GetSize() < b.GetSize()){
        return -1;
      } else if(a.GetSize() > b.GetSize()){
        return +1;
      }
      return 0;
    }

    enum Layout{
      // GetMagic
      kMagicOffset = 0,
      kBitsForMagic = 16,
      // type
      kTypeOffset = kMagicOffset+kBitsForMagic,
      kBitsForType = 16,
      // size
      kSizeOffset = kTypeOffset+kBitsForType,
      kBitsForSize = 16
    };

    class MagicField : public BitField<RawObjectTag, uint16_t, kMagicOffset, kBitsForMagic>{};
    class TypeField : public BitField<RawObjectTag, uint16_t, kTypeOffset, kBitsForType>{};
    class SizeField : public BitField<RawObjectTag, uint16_t, kSizeOffset, kBitsForSize>{};
   private:
    RawObjectTag raw_;
   public:
    ObjectTag():
      raw_(0){}
    ObjectTag(const RawObjectTag& raw):
      raw_(raw){}
    ObjectTag(const Type& type, const uint16_t& size):
      raw_(MagicField::Encode(TOKEN_MAGIC)|TypeField::Encode((uint16_t)type)|SizeField::Encode(size)){}
    ObjectTag(const ObjectTag& tag):
      raw_(tag.raw_){}
    ~ObjectTag() = default;

    RawObjectTag& raw(){
      return raw_;
    }

    RawObjectTag raw() const{
      return raw_;
    }

    bool IsValid() const{
      return MagicField::Decode(raw_)
          == static_cast<uint16_t>(TOKEN_MAGIC);
    }

    Type GetType() const{
      return static_cast<Type>(TypeField::Decode(raw_));
    }

    int64_t GetSize() const{
      return static_cast<int64_t>(SizeField::Decode(raw_));
    }

#define DEFINE_TYPE_CHECK(Name) \
    bool Is##Name##Type() const{\
      return GetType() == Type::k##Name; \
    }
    FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

    ObjectTag& operator=(const ObjectTag& tag){
      raw_ = tag.raw_;
      return (*this);
    }

    friend bool operator==(const ObjectTag& a, const ObjectTag& b){
      return a.raw() == b.raw();
    }

    friend bool operator!=(const ObjectTag& a, const ObjectTag& b){
      return a.raw() != b.raw();
    }

    friend std::ostream& operator<<(std::ostream& stream, const ObjectTag& tag){
      return stream << "ObjectTag(" << tag.GetType() << ", " << tag.GetSize() << ")";
    }
  };

  class Object{
   public:
    virtual ~Object() = default;
    virtual std::string ToString() const = 0;
  };

  class SerializableObject : public Object{
   protected:
    SerializableObject() = default;
   public:
    virtual ~SerializableObject() = default;
    virtual Type GetType() const = 0;

    ObjectTag GetTag() const{
      return ObjectTag(GetType(), GetBufferSize());
    }

#define DEFINE_TYPE_CHECK(Name) \
    bool Is##Name##Message() const{ return GetType() == Type::k##Name##Message; }
    FOR_EACH_MESSAGE_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

#define DEFINE_TYPE_CHECK(Name) \
    bool Is##Name() const{ return GetType() == Type::k##Name; }
    FOR_EACH_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

    BufferPtr ToBuffer() const;
    BufferPtr ToBufferTagged() const;
    bool ToFile(const std::string& filename) const;

    virtual int64_t GetBufferSize() const = 0;
    virtual bool Write(const BufferPtr& buff) const = 0;
    virtual bool Write(Json::Writer& writer) const{ return false; }
  };

  class BinaryObject : public SerializableObject{
   protected:
    BinaryObject() = default;
   public:
    virtual ~BinaryObject() = default;
    Hash GetHash() const;
  };

  template<int16_t Size>
  class RawType{
    //TODO: refactor
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

    uint8_t* raw_data() const{
      return (uint8_t*)data_;
    }

    char* data() const{
      return (char*) data_;
    }

    size_t size() const{
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

  static const int16_t kRawUserSize = 64;
  class User : public RawType<kRawUserSize>{
    using Base = RawType<kRawUserSize>;
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
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const User& a, const User& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const User& a, const User& b){
      return Compare(a, b) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const User& user){
      stream << user.str();
      return stream;
    }

    static int Compare(const User& a, const User& b){
      return Base::Compare(a, b);
    }
  };

  static const int16_t kRawReferenceSize = 64;
  class Reference : public RawType<kRawReferenceSize>{
    using Base = RawType<kRawReferenceSize>;
   public:
    Reference():
      Base(){}
    Reference(const uint8_t* bytes, const int16_t size=kRawReferenceSize):
      Base(bytes, size){}
    Reference(const Reference& ref):
      Base(){
      memcpy(data(), ref.data(), std::min(ref.size(), (size_t)kRawReferenceSize));
    }
    Reference(const std::string& ref):
      Base(){
      memcpy(data(), ref.data(), std::min(ref.length(), (size_t)kRawReferenceSize));
    }
    ~Reference() = default;

    Reference& operator=(const Reference& ref){
      memcpy(data(), ref.data(), ref.size());
      return (*this);
    }

    friend bool operator==(const Reference& a, const Reference& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Reference& a, const Reference& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const Reference& a, const Reference& b){
      return Compare(a, b) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Reference& ref){
      return stream << ref.str();
    }

    static inline int
    Compare(const Reference& a, const Reference& b){
      return strncasecmp(a.data(), b.data(), kRawReferenceSize);
    }
  };

  static const int16_t kRawUUIDSize = 16;
  class UUID : public RawType<kRawUUIDSize>{
    using Base = RawType<kRawUUIDSize>;
   public:
    UUID():
      RawType(){
      uuid_generate_time_safe(data_);
    }
    UUID(uint8_t* bytes, int64_t size):
      RawType(bytes, size){}
    UUID(const char* uuid):
      RawType(){
      uuid_parse(uuid, data_);
    }
    UUID(const std::string& uuid):
      RawType(){
      memcpy(data(), uuid.data(), kRawUUIDSize);
    }
    UUID(const leveldb::Slice& slice):
      RawType(){
      memcpy(data(), slice.data(), kRawUUIDSize);
    }
    UUID(const UUID& other):
      RawType(){
      memcpy(data(), other.data(), kRawUUIDSize);
    }
    ~UUID() = default;

    std::string ToString() const{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 37);
    }

    UUID& operator=(const UUID& other){
      memcpy(data(), other.data(), kRawUUIDSize);
      return (*this);
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

    operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
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