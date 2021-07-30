#include "buffer.h"
#include "transaction_unclaimed.h"

namespace token{
  std::string UnclaimedTransaction::ToString() const{
    NOT_IMPLEMENTED(ERROR);//TODO: implement
    return "UnclaimedTransaction()";
  }

  internal::BufferPtr UnclaimedTransaction::ToBuffer() const{
    auto length = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(length);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
      DLOG(FATAL) << "cannot serialize UnclaimedTransaction to buffer.";
      return nullptr;
    }
    return data;
  }
}