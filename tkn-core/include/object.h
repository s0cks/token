#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include "type.h"
#include "codec/codec.h"

namespace token{
  class Object{
   protected:
    Object() = default;
   public:
    virtual ~Object() = default;
    virtual Type type() const = 0;
    virtual std::string ToString() const = 0;
  };

  class BinaryObject : public Object{
   protected:
    BinaryObject() = default;

    template<class T, class E>
    static inline Hash
    ComputeHash(const E& value){
      E encoder(value, codec::kDefaultEncoderFlags);
      internal::BufferPtr data = internal::NewBufferFor(encoder);
      if(!encoder.Encode(data)){
        LOG(ERROR) << "cannot encoder IndexedTransaction to buffer.";
        return Hash();
      }
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->GetWritePosition());
    }
   public:
    ~BinaryObject() override = default;
    virtual Hash hash() const = 0;
  };
}

#endif //TOKEN_OBJECT_H