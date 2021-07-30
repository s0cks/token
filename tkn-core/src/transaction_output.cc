#include "buffer.h"
#include "transaction_output.h"

namespace token{
  std::string Output::ToString() const{
    std::stringstream ss;
    ss << "Output(";
    ss << "user=" << user() << ",";
    ss << "product=" << product();
    ss << ")";
    return ss.str();
  }

  internal::BufferPtr Output::ToBuffer() const{
    auto length = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(length);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
      DLOG(FATAL) << "cannot serialize Output to buffer.";
      return nullptr;
    }
    return data;
  }
}