#ifndef TOKEN_OBJECT_TAG_H
#define TOKEN_OBJECT_TAG_H

#include "bitfield.h"
#include "utils/file_reader.h"

namespace Token{
  class ObjectTag{
   public:
    enum Type{
      kNone = 0,
      kBlock = 1,
      kTransaction,
      kUnclaimedTransaction,
    };

    friend std::ostream &operator<<(std::ostream &stream, const Type &type){
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

    ObjectTag(const int32_t &magic, const Type &type):
      value_(MagicField::Encode(magic) | MagicField::Encode(type)){}

    int32_t GetMagicField() const{
      return MagicField::Decode(value_);
    }

    void SetMagicField(const int32_t &magic){
      value_ = MagicField::Update(magic, value_);
    }

    Type GetTypeField() const{
      return TypeField::Decode(value_);
    }

    void SetTypeField(const Type &type){
      value_ = TypeField::Update(type, value_);
    }
   public:
    ObjectTag(uint8_t *data):
      value_(*(uint64_t *) data){}
    ObjectTag():
      value_(MagicField::Encode(kMagic) | TypeField::Encode(Type::kNone)){}
    ObjectTag(const uint64_t &tag):
      value_(tag){}
    ObjectTag(const Type &type):
      value_(MagicField::Encode(kMagic) | TypeField::Encode(type)){}
    ObjectTag(const ObjectTag &tag):
      value_(tag.value_){}
    ~ObjectTag() = default;

    uint64_t &data(){
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

    void operator=(const ObjectTag &tag){
      value_ = tag.value_;
    }

    friend bool operator==(const ObjectTag &a, const ObjectTag &b){
      return a.value_ == b.value_;
    }

    friend bool operator!=(const ObjectTag &a, const ObjectTag &b){
      return a.value_ != b.value_;
    }

    friend std::ostream &operator<<(std::ostream &stream, const ObjectTag &tag){
      return stream << tag.GetTypeField();
    }

    static inline int64_t
    GetSize(){
      return sizeof(uint64_t);
    }
  };

  class ObjectTagReader : protected BinaryFileReader{
   public:
    ObjectTagReader(const std::string &filename):
      BinaryFileReader(filename){}
    ObjectTagReader(BinaryFileReader *parent):
      BinaryFileReader(parent){}
    ~ObjectTagReader() = default;

    ObjectTag ReadObjectTag(){
      return ObjectTag(ReadUnsignedLong());
    }
  };

  class ObjectTagVerifier : ObjectTagReader{
   private:
    ObjectTag expected_;
   public:
    ObjectTagVerifier(const std::string &filename, const ObjectTag::Type &expected_type):
      ObjectTagReader(filename),
      expected_(expected_type){}
    ObjectTagVerifier(BinaryFileReader *parent, const ObjectTag::Type &expected_type):
      ObjectTagReader(parent),
      expected_(expected_type){}
    ~ObjectTagVerifier() = default;

    ObjectTag &GetExpectedTag(){
      return expected_;
    }

    ObjectTag GetExpectedTag() const{
      return expected_;
    }

    bool IsValid(){
      if(expected_.IsMagic() && expected_.IsNone())
        return ReadObjectTag().IsMagic();
      return ReadObjectTag() == GetExpectedTag();
    }
  };
}

#endif //TOKEN_OBJECT_TAG_H