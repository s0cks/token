#include "buffer.h"
#include "transaction_indexed.h"

namespace token{
  internal::BufferPtr IndexedTransaction::ToBuffer() const{
    RawIndexedTransaction raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
      return nullptr;
    }
    DVLOG(2) << "serialized " << ToString() << " into " << data->ToString() << ".";
    return data;
  }
}