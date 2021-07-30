#include "buffer.h"
#include "transaction_input.h"

namespace token{
  std::string Input::ToString() const{
    std::stringstream ss;
    ss << "Input(";

    ss << ")";
    return ss.str();
  }

  internal::BufferPtr Input::ToBuffer() const{
    auto size = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(size);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(size))){
      DLOG(FATAL) << "cannot serialize Input to buffer.";
      return nullptr;
    }
    return data;
  }
}