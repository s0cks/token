#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include "buffer.h"

namespace token{
 enum Type{
   kBlock,
   kInput,
   kOutput,
   kTransaction,
   kIndexedTransaction,
 };

  class Object{
   protected:
    Object() = default;
   public:
    virtual ~Object() = default;
    virtual Type type() const = 0;
  };

  class SerializableObject : public Object{
   protected:
    SerializableObject() = default;
   public:
    ~SerializableObject() override = default;
    virtual uint64_t GetBufferSize() const = 0;
    virtual bool WriteTo(const BufferPtr& data) const = 0;

    bool WriteTo(const std::string& filename) const;

    BufferPtr ToBuffer() const{
      auto data = NewBuffer(GetBufferSize());
      if(!WriteTo(data))
        return nullptr;//TODO: better error handling.
      return data;
    }
  };

  class BinaryObject : public SerializableObject{
   protected:
    BinaryObject() = default;
   public:
    ~BinaryObject() override = default;

    uint256 hash() const{
      auto data = NewBuffer(GetBufferSize());
      if(!SerializableObject::WriteTo(data)){
        LOG(FATAL) << "cannot write to buffer.";
        return {};
      }
      return sha256::Of(data->data(), data->length());
    }
  };
}

#endif //TOKEN_OBJECT_H