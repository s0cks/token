#include "buffer.h"
#include "transaction_unsigned.h"

namespace token{
  std::string UnsignedTransaction::ToString() const{
    std::stringstream ss;
    ss << "UnsignedTransaction(";
    ss << "timestamp=" << FormatTimestampReadable(timestamp()) << ", ";
    ss << "inputs=[]" << ", ";
    ss << "outputs=[]";
    ss << ")";
    return ss.str();
  }

  BufferPtr UnsignedTransaction::ToBuffer() const{
    auto length = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(length);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
      DLOG(FATAL) << "cannot serialize UnsignedTransaction to buffer.";
      return nullptr;
    }
    return data;
  }
}