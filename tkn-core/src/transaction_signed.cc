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

  SignedTransaction* SignedTransaction::Decoder::Decode(const BufferPtr& data) const{
    Timestamp timestamp;
    InputList inputs;
    OutputList outputs;
    if(!DecodeTransactionData(data, timestamp, inputs, outputs))
      return nullptr;
    return new SignedTransaction(timestamp, inputs, outputs);
  }
}