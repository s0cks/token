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
    auto size = TransactionEncoder<IndexedTransaction>::GetBufferSize();
    size += sizeof(int64_t); //index
    return size;
  }

  bool IndexedTransaction::Encoder::Encode(const BufferPtr &buff) const{
    if(!TransactionEncoder<IndexedTransaction>::Encode(buff))
      return false;

    const auto& index = value()->index();
    if(!buff->PutLong(index)){
      LOG(FATAL) << "cannot encode index to buffer.";
      return false;
    }
    return true;
  }

  IndexedTransaction* IndexedTransaction::Decoder::Decode(const BufferPtr& data) const{
    Timestamp timestamp;
    InputList inputs;
    OutputList outputs;
    if(!TransactionDecoder<IndexedTransaction>::DecodeTransactionData(data, timestamp, inputs, outputs))
      return nullptr;

    auto index = data->GetLong();
    DECODED_FIELD(index_, Long, index);

    return new IndexedTransaction(index, timestamp, inputs, outputs);
  }
}