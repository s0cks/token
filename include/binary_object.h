#ifndef TOKEN_BINARY_OBJECT_H
#define TOKEN_BINARY_OBJECT_H

#include "hash.h"
#include "crypto.h"
#include "object.h"

namespace token{
  class BinaryObject : public Object{
   protected:
    BinaryObject() = default;

    template<typename HashFunction>
    static inline Hash
    ComputeHash(const uint8_t* data, const size_t& size){
      HashFunction function;
      CryptoPP::SecByteBlock digest(HashFunction::DIGESTSIZE);
      CryptoPP::ArraySource source(data, size, true, new CryptoPP::HashFilter(function, new CryptoPP::ArraySink(digest.data(), digest.size())));
      return Hash(digest.data(), HashFunction::DIGESTSIZE);
    }
   public:
    ~BinaryObject() override = default;
    virtual internal::BufferPtr ToBuffer() const = 0;

    Hash hash() const;
  };
}


#endif//TOKEN_BINARY_OBJECT_H