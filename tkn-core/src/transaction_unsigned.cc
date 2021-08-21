#include "buffer.h"
#include "transaction_unsigned.h"

namespace token{
  internal::BufferPtr UnsignedTransaction::ToBuffer() const{
    RawUnsignedTransaction raw;
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