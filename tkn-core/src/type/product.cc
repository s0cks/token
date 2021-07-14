#include "type/product.h"

namespace token{
  namespace codec{
    ProductEncoder::ProductEncoder(const Product &value, const codec::EncoderFlags &flags):
      codec::EncoderBase<Product>(value, flags){}

    int64_t ProductEncoder::GetBufferSize() const{
      auto size = BaseType::GetBufferSize();

      return size;
    }

    bool ProductEncoder::Encode(const BufferPtr &buff) const{
      return false;
    }

    ProductDecoder::ProductDecoder(const codec::DecoderHints &hints):
      codec::DecoderBase<Product>(hints){}

    bool ProductDecoder::Decode(const BufferPtr &buff, Product &result) const{
      return false;
    }
  }
}