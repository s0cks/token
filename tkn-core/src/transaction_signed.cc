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

  namespace codec{
    int64_t SignedTransactionEncoder::GetBufferSize() const{
      return TransactionEncoder<SignedTransaction>::GetBufferSize();
    }

    bool SignedTransactionEncoder::Encode(const BufferPtr& buff) const{
      return TransactionEncoder<SignedTransaction>::Encode(buff);
    }

    bool SignedTransactionDecoder::Decode(const BufferPtr &buff, SignedTransaction& result) const{
      Timestamp timestamp;
      InputList inputs;
      OutputList outputs;
      if(!DecodeTransactionData(buff, timestamp, inputs, outputs))
        return false;
      result = SignedTransaction(timestamp, inputs, outputs);
      return true;
    }
  }
}