#include "type/product.h"

namespace token{
  Product::Encoder::Encoder(const Product* value, const codec::EncoderFlags &flags):
    codec::TypeEncoder<Product>(value, flags){}

  int64_t Product::Encoder::GetBufferSize() const{
    auto size = TypeEncoder<Product>::GetBufferSize();
    size += kMaxProductLength;
    return size;
  }

  bool Product::Encoder::Encode(const BufferPtr &buff) const{
    if(!TypeEncoder<Product>::Encode(buff))
      return false;

    auto data = value()->data();
    auto length = kMaxProductLength;
    if(!buff->PutBytes((uint8_t*)data, length)){
      DLOG(FATAL) << "cannot put bytes (" << length << "b).";
      return false;
    }
    return true;
  }

  Product* Product::Decoder::Decode(const BufferPtr& data) const{
    auto length = kMaxProductLength;
    uint8_t bytes[length];
    if(!data->GetBytes(bytes, length))
      CANNOT_DECODE_FIELD(bytes, uint8_t[]);
    return new Product(bytes, length);
  }
}