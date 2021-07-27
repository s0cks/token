#include "buffer.h"
#include "transaction_signed.h"

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

  int64_t SignedTransaction::Encoder::GetBufferSize() const{
    return TransactionEncoder<SignedTransaction>::GetBufferSize();
  }

  bool SignedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    return TransactionEncoder<SignedTransaction>::Encode(buff);
  }

  bool SignedTransaction::Decoder::Decode(const BufferPtr &buff, SignedTransaction& result) const{
    Timestamp timestamp;
    InputList inputs;
    OutputList outputs;
    if(!DecodeTransactionData(buff, timestamp, inputs, outputs))
      return false;
    result = SignedTransaction(timestamp, inputs, outputs);
    return true;
  }
}