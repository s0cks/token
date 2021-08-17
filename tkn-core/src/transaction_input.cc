#include "buffer.h"
#include "transaction_input.h"

namespace token{
  internal::BufferPtr Input::ToBuffer() const{
    RawInput raw;
    raw.set_hash(hash_.HexString());
    raw.set_transaction(source_.transaction().HexString());
    raw.set_index(source_.index());
    auto length = static_cast<uint64_t>(raw.ByteSizeLong());
    auto data = internal::NewBuffer(length+sizeof(uint64_t));
    if(!data->PutUnsignedLong(length)){ // length_
      LOG(FATAL) << "cannot serialize Input (" << length << "b) to buffer of size: " << data->length();
      return nullptr;
    }
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize Input (" << length << "b) to buffer of size: " << data->length();
      return nullptr;
    }
    return data;
  }
}