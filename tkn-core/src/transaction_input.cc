#include "buffer.h"
#include "transaction_input.h"

namespace token{
  internal::BufferPtr Input::ToBuffer() const{
    RawInput raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
      return nullptr;
    }
    DVLOG(2) << "serialized " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
    return data;
  }
}