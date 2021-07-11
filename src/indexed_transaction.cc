#include "buffer.h"
#include "indexed_transaction.h"

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

  BufferPtr IndexedTransaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = internal::For(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode IndexedTransaction";
      //TODO: clear buffer
    }
    return buffer;
  }

  int64_t IndexedTransaction::Encoder::GetBufferSize() const{
    int64_t size = TransactionEncoder<IndexedTransaction>::GetBufferSize();
    size += sizeof(RawTimestamp);
    size += codec::EncoderBase<IndexedTransaction>::GetBufferSize<Input, Input::Encoder>(value().inputs());
    size += codec::EncoderBase<IndexedTransaction>::GetBufferSize<Output, Output::Encoder>(value().outputs());
    size += sizeof(int64_t);
    return size;
  }

  template<class V, class E>
  static inline bool
  EncodeList(const BufferPtr& buff, const std::vector<V>& list, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags){
    if(!buff->PutLong(list.size())){
      DLOG(ERROR) << "cannot encode list size.";
      return false;
    }

    for(auto& it : list){
      E encoder(it, flags);
      if(!encoder.Encode(buff)){
        DLOG(ERROR) << "cannot encode list item: " << it;
        return false;
      }
    }
    return true;
  }

  bool IndexedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    if(!TransactionEncoder<IndexedTransaction>::Encode(buff))
      return false;

    const int64_t index = value().index();
    if(!buff->PutLong(index)){
      CANNOT_ENCODE_VALUE(FATAL, int64_t, index);
      return false;
    }
    DVLOG(3) << "encoded IndexedTransaction index: " << index;
    return true;
  }

  template<class V, class D>
  static inline bool
  DecodeList(const BufferPtr& buff, std::vector<V>& list, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    D decoder(hints);

    auto length = buff->GetLong();
    DLOG(INFO) << "decoded list length: " << length;

    for(auto idx = 0; idx < length; idx++){
      V item;
      if(!decoder.Decode(buff, item)){
        DLOG(ERROR) << "couldn't decode list item #" << idx;
        return false;
      }
      DLOG(INFO) << "decoded list item #" << idx << ": " << item;
      list.push_back(item);
    }
    return true;
  }

  bool IndexedTransaction::Decoder::Decode(const BufferPtr& buff, IndexedTransaction& result) const{
    Timestamp timestamp = buff->GetTimestamp();
    DLOG(INFO) << "decoded IndexedTransaction timestamp: " << FormatTimestampReadable(timestamp);

    InputList inputs;
    if(!DecodeList<Input, Input::Decoder>(buff, inputs)){
      LOG(FATAL) << "cannot decode InputList from buffer.";
      return false;
    }
    DLOG(INFO) << "decoded inputs (InputList), " << inputs.size() << " items.";

    OutputList outputs;
    if(!DecodeList<Output, Output::Decoder>(buff, outputs)){
      LOG(FATAL) << "cannot decode OutputList from buffer.";
      return false;
    }
    DLOG(INFO) << "decoded outputs (OutputList), " << outputs.size() << " items.";

    int64_t index = buff->GetLong();
    DLOG(INFO) << "decoded IndexedTransaction index: " << index;

    result = IndexedTransaction(index, timestamp, inputs, outputs);
    return true;
  }
}