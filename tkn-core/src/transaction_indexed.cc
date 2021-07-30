#include "buffer.h"
#include "transaction_indexed.h"

namespace token{
  std::string IndexedTransaction::ToString() const{
    NOT_IMPLEMENTED(ERROR);
    return "IndexedTransaction()";
  }

  internal::BufferPtr IndexedTransaction::ToBuffer() const{
    auto length = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(length);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
      DLOG(FATAL) << "cannot serialize IndexedTransaction to buffer.";
      return nullptr;
    }
    return data;
  }
}