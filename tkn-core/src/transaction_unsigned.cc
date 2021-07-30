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

  BufferPtr UnsignedTransaction::ToBuffer() const{
    UnsignedTransaction::Encoder encoder(this, codec::kDefaultEncoderFlags);
    BufferPtr buffer = internal::NewBufferFor(encoder);
    if(!encoder.Encode(buffer)){
      LOG(FATAL) << "cannot encode UnsignedTransaction to buffer.";
      return nullptr;
    }
    return buffer;
  }

  int64_t UnsignedTransaction::Encoder::GetBufferSize() const{
    return TransactionEncoder<UnsignedTransaction>::GetBufferSize();
  }

  bool UnsignedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    return TransactionEncoder<UnsignedTransaction>::Encode(buff);
  }

  UnsignedTransaction* UnsignedTransaction::Decoder::Decode(const BufferPtr& data) const{
    Timestamp timestamp;
    InputList inputs;
    OutputList outputs;
    if(!DecodeTransactionData(data, timestamp, inputs, outputs))
      return nullptr;
    return new UnsignedTransaction(timestamp, inputs, outputs);//TODO: fix allocation
  }
}