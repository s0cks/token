#include "buffer.h"
#include "transaction_signed.h"

namespace token{
  internal::BufferPtr SignedTransaction::ToBuffer() const{
    RawSignedTransaction raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
      return nullptr;
    }
    DVLOG(2) << "serialized " << ToString() << "(" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
    return data;
  }
}