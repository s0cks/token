#include "buffer.h"
#include "transaction_indexed.h"

namespace token{
  std::string IndexedTransaction::ToString() const{
    std::stringstream ss;
    ss << "IndexedTransaction(";
    ss << "index=" << index_ << ", ";
    ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
    ss << "inputs=[]" << ", ";
    ss << "outputs=[]";
    ss << ")";
    return ss.str();
  }

  int64_t IndexedTransaction::Encoder::GetBufferSize() const{
    auto size = codec::TransactionEncoder<IndexedTransaction>::GetBufferSize();
    size += sizeof(int64_t); //index
    return size;
  }

  bool IndexedTransaction::Encoder::Encode(const BufferPtr &buff) const{
    if(!codec::TransactionEncoder<IndexedTransaction>::Encode(buff))
      return false;

    const auto& index = value()->index();
    if(!buff->PutLong(index)){
      LOG(FATAL) << "cannot encode index to buffer.";
      return false;
    }
    return true;
  }

  bool IndexedTransaction::Decoder::Decode(const BufferPtr &buff, IndexedTransaction &result) const{
    Timestamp timestamp;
    InputList inputs;
    OutputList outputs;
    if(!TransactionDecoder<IndexedTransaction>::DecodeTransactionData(buff, timestamp, inputs, outputs))
      return false;

    auto index = buff->GetLong();
    DLOG(INFO) << "decoded transaction index: " << index;

    result = IndexedTransaction(index, timestamp, inputs, outputs);
    return true;
  }
}