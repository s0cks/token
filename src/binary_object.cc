#include "buffer.h"
#include "binary_object.h"

namespace token{
  Hash BinaryObject::hash() const{
    BufferPtr buffer = ToBuffer();
    return ComputeHash<CryptoPP::SHA256>((const uint8_t*)buffer->data(), buffer->GetWrittenBytes());
  }
}