#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include <leveldb/slice.h>
#include "hash.h"
#include "utils/bitfield.h"

namespace Token{
  class Buffer;
  typedef std::shared_ptr<Buffer> BufferPtr;

  class Object{
   public:
    enum class Type{
      kBlock = 1,
      kTransaction = 2,
      kUnclaimedTransaction = 3,
    };

    friend std::ostream& operator<<(std::ostream& stream, const Type& type){
      switch(type){
        case Type::kBlock:return stream << "Block";
        case Type::kTransaction:return stream << "Transaction";
        case Type::kUnclaimedTransaction:return stream << "UnclaimedTransaction";
        default:return stream << "Unknown";
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
    virtual bool Write(BinaryFileWriter* writer) const{ return false; }
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

    std::string str() const{
      return std::string(data(), std::min(strlen((char*) data_), (size_t) Size));
    }

    static inline int64_t
    GetSize(){
      return Size;
    }
  };

  class Product : public RawType<64>{
    using Base = RawType<64>;
   public:
    static const int64_t kSize = 64;

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

    int64_t size() const{
      return std::min((int64_t) strlen((char*) data_), Base::GetSize());
    }

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
    static const int64_t kSize = 64;

    User():
      Base(){}
    User(const uint8_t* bytes, int64_t size):
      Base(bytes, size){}
    User(const User& user):
      Base(){
      memcpy(data(), user.data(), Base::GetSize());
    }
    User(const std::string& value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    ~User() = default;

    int64_t size() const{
      return std::min((int64_t) strlen((char*) data_), Base::GetSize());
    }

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

  class ObjectTag{
   public:
    enum Type{
      kNone = 0,
      kBlock = 1,
      kTransaction,
      kUnclaimedTransaction,
    };

    friend std::ostream& operator<<(std::ostream& stream, const Type& type){
      switch(type){
        case Type::kBlock:return stream << "Block";
        case Type::kTransaction:return stream << "Transaction";
        case Type::kUnclaimedTransaction:return stream << "UnclaimedTransaction";
        case Type::kNone:
        default:return stream << "None";
      }
    }

    static const int32_t kMagic;
   private:
    enum ObjectTagLayout{
      kMagicFieldOffset = 0,
      kMagicFieldSize = 32,
      kTypeFieldOffset = kMagicFieldOffset + kMagicFieldSize,
      kTypeFieldSize = 32,
    };

    class MagicField : public BitField<uint64_t, int32_t, kMagicFieldOffset, kMagicFieldSize>{};
    class TypeField : public BitField<uint64_t, Type, kTypeFieldOffset, kTypeFieldSize>{};

    uint64_t value_;

    ObjectTag(const int32_t& magic, const Type& type):
      value_(MagicField::Encode(magic) | MagicField::Encode(type)){}

    int32_t GetMagicField() const{
      return MagicField::Decode(value_);
    }

    void SetMagicField(const int32_t& magic){
      value_ = MagicField::Update(magic, value_);
    }

    Type GetTypeField() const{
      return TypeField::Decode(value_);
    }

    void SetTypeField(const Type& type){
      value_ = TypeField::Update(type, value_);
    }
   public:
    ObjectTag(uint8_t* data):
      value_(*(uint64_t*) data){}
    ObjectTag():
      value_(MagicField::Encode(kMagic) | TypeField::Encode(Type::kNone)){}
    ObjectTag(const uint64_t& tag):
      value_(tag){}
    ObjectTag(const Type& type):
      value_(MagicField::Encode(kMagic) | TypeField::Encode(type)){}
    ObjectTag(const ObjectTag& tag):
      value_(tag.value_){}
    ~ObjectTag() = default;

    uint64_t& data(){
      return value_;
    }

    uint64_t data() const{
      return value_;
    }

    bool IsMagic() const{
      return GetMagicField() == kMagic;
    }

    bool IsNone() const{
      return GetTypeField() == Type::kNone;
    }

    bool IsBlock() const{
      return GetTypeField() == Type::kBlock;
    }

    bool IsTransaction() const{
      return GetTypeField() == Type::kTransaction;
    }

    bool IsUnclaimedTransaction() const{
      return GetTypeField() == Type::kUnclaimedTransaction;
    }

    void operator=(const ObjectTag& tag){
      value_ = tag.value_;
    }

    friend bool operator==(const ObjectTag& a, const ObjectTag& b){
      return a.value_ == b.value_;
    }

    friend bool operator!=(const ObjectTag& a, const ObjectTag& b){
      return a.value_ != b.value_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const ObjectTag& tag){
      return stream << tag.GetTypeField();
    }

    static inline int64_t
    GetSize(){
      return sizeof(uint64_t);
    }
  };
}

#endif //TOKEN_OBJECT_H