#include "buffer.h"
#include "transaction_output.h"

namespace token{
  internal::BufferPtr Output::ToBuffer() const{
    RawOutput raw;
    raw.set_user(user().ToString());
    raw.set_product(product().ToString());
    auto data = internal::NewBufferForProto(raw);
    if(data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize Output to buffer of size: " << data->length();
      return nullptr;
    }
    return data;
  }
}