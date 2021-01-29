#ifndef TOKEN_TAG_H
#define TOKEN_TAG_H

#include "object.h"
#include "utils/bitfield.h"

namespace Token{
#define TOKEN_OBJECT_TAG_MAGIC 0xFAFE

  typedef int64_t RawObjectTag;

  class ObjectTag{
   private:
    enum ObjectTagLayout{
      // magic
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

    RawObjectTag raw_;
   public:
    ObjectTag():
      raw_(0){}
    ObjectTag(const RawObjectTag& raw):
      raw_(raw){}
    ObjectTag(const Object::Type& type, const uint16_t& size):
      raw_(MagicField::Encode(TOKEN_OBJECT_TAG_MAGIC)
          |TypeField::Encode(static_cast<uint16_t>(type))
          |SizeField::Encode(size)){}
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
          == static_cast<uint16_t>(TOKEN_OBJECT_TAG_MAGIC);
    }

    Object::Type GetType() const{
      return static_cast<Object::Type>(TypeField::Decode(raw_));
    }

    int64_t GetSize() const{
      return static_cast<int64_t>(SizeField::Decode(raw_));
    }

#define DEFINE_TYPE_CHECK(Name) \
    bool Is##Name##Type() const{\
      return GetType() == Object::Type::k##Name##Type; \
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
}

#endif//TOKEN_TAG_H