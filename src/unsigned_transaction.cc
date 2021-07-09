#include "buffer.h"
#include "unsigned_transaction.h"

namespace token{
  std::string UnsignedTransaction::ToString() const{
    std::stringstream ss;
    ss << "UnsignedTransaction(";
    ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
    ss << "inputs=[]" << ", ";
    ss << "outputs=[]";
    ss << ")";
    return ss.str();
  }

  BufferPtr UnsignedTransaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode IndexedTransaction";
      buffer->clear();
    }
    return buffer;
  }


  bool UnsignedTransaction::Decoder::Decode(const BufferPtr &buff, UnsignedTransaction& result) const{
    return TransactionDecoder<UnsignedTransaction>::Decode(buff, result);
  }
}