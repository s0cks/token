#include "buffer.h"
#include "transaction_unclaimed.h"

namespace token{
  internal::BufferPtr UnclaimedTransaction::ToBuffer() const{
    RawUnclaimedTransaction raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
      return nullptr;
    }
    DLOG(INFO) << "serialized " << ToString() << " into " << data->ToString() << ".";
    return data;
  }
}