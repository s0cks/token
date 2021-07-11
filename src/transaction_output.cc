#include "buffer.h"
#include "transaction_output.h"

namespace token{
  std::string Output::ToString() const{
    std::stringstream ss;
    ss << "Output(";
    ss << "user=" << GetUser() << ",";
    ss << "product=" << GetProduct();
    ss << ")";
    return ss.str();
  }

  BufferPtr Output::ToBuffer() const{
    Encoder encoder((*this), codec::kDefaultEncoderFlags);
    BufferPtr buffer = internal::NewBufferFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode Output.";
      //TODO: Clear the buffer?
      return buffer;
    }
    return buffer;
  }
}