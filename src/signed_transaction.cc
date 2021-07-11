#include "buffer.h"
#include "signed_transaction.h"

namespace token{
  std::string SignedTransaction::ToString() const{
    std::stringstream ss;
    ss << "SignedTransaction(";
    ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
    ss << "inputs=[]" << ", ";
    ss << "outputs=[]";
    ss << ")";
    return ss.str();
  }

  BufferPtr SignedTransaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = internal::For(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode IndexedTransaction";
      //TODO: clear buffer
    }
    return buffer;
  }

  int64_t SignedTransaction::Encoder::GetBufferSize() const{
    return TransactionEncoder<SignedTransaction>::GetBufferSize();
  }

  bool SignedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    return TransactionEncoder<SignedTransaction>::Encode(buff);
  }

  bool SignedTransaction::Decoder::Decode(const BufferPtr &buff, SignedTransaction& result) const{
    return TransactionDecoder<SignedTransaction>::Decode(buff, result);
  }
}