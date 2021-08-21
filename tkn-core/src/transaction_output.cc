#include "buffer.h"
#include "transaction_output.h"

namespace token{
  internal::BufferPtr Output::ToBuffer() const{
    RawOutput raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " into " << data->ToString() << ".";
      return nullptr;
    }
    DVLOG(2) << "serialized " << ToString() << " into " << data->ToString();
    return data;
  }
}