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

  Product::Decoder::Decoder(const codec::DecoderHints &hints):
    codec::DecoderBase<Product>(hints){}

  bool Product::Decoder::Decode(const BufferPtr &buff, Product &result) const{
    auto length = kMaxProductLength;
    uint8_t data[length];
    if(!buff->GetBytes(data, length)){
      DLOG(FATAL) << "cannot decode data (uint8_t[" << length << "]) from buffer.";
      return false;
    }

    result = Product(data, length);
    return true;
  }
}