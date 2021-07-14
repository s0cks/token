#include "buffer.h"
#include "transaction_unsigned.h"

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

  namespace codec{
    int64_t UnsignedTransactionEncoder::GetBufferSize() const{
      return TransactionEncoder<UnsignedTransaction>::GetBufferSize();
    }

    bool UnsignedTransactionEncoder::Encode(const BufferPtr& buff) const{
      return TransactionEncoder<UnsignedTransaction>::Encode(buff);
    }

    bool UnsignedTransactionDecoder::Decode(const BufferPtr &buff, UnsignedTransaction& result) const{
      Timestamp timestamp;
      InputList inputs;
      OutputList outputs;
      if(!DecodeTransactionData(buff, timestamp, inputs, outputs))
        return false;
      result = UnsignedTransaction(timestamp, inputs, outputs);
      return true;
    }
  }
}