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
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode IndexedTransaction";
      buffer->clear();
    }
    return buffer;
  }

  int64_t IndexedTransaction::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<IndexedTransaction>::GetBufferSize();
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
    if(ShouldEncodeVersion()){
      if(!buff->PutVersion(codec::GetCurrentVersion())){
        CANNOT_ENCODE_CODEC_VERSION(FATAL);
        return false;
      }
      DLOG(INFO) << "encoded codec version: " << codec::GetCurrentVersion();
    }

    const Timestamp& timestamp = value().timestamp();
    if(!buff->PutTimestamp(timestamp)){
      CANNOT_ENCODE_VALUE(FATAL, Timestamp, ToUnixTimestamp(timestamp));
      return false;
    }
    DVLOG(3) << "encoded IndexedTransaction timestamp: " << FormatTimestampReadable(timestamp);

    const InputList& inputs = value().inputs();
    if(!EncodeList<Input, Input::Encoder>(buff, inputs)){
      DLOG(ERROR) << "cannot encode IndexedTransaction inputs: " << inputs.size();
      return false;
    }
    DVLOG(3) << "encoded IndexedTransaction inputs: " << inputs.size();

    const OutputList& outputs = value().outputs();
    if(!EncodeList<Output, Output::Encoder>(buff, outputs)){
      DLOG(ERROR) << "cannot encode IndexedTransaction outputs: " << outputs.size();
      return false;
    }
    DVLOG(3) << "encoded IndexedTransaction outputs: " << outputs.size();

    const int64_t index = value().GetIndex();
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
    return buff->GetList<V, D>(list, hints);
  }

  bool IndexedTransaction::Decoder::Decode(const BufferPtr& buff, IndexedTransaction& result) const{
    CHECK_CODEC_VERSION(FATAL, buff);

    Timestamp timestamp = buff->GetTimestamp();
    DLOG(INFO) << "decoded IndexedTransaction timestamp: " << FormatTimestampReadable(timestamp);

    InputList inputs;
    if(!DecodeList<Input, Input::Decoder>(buff, inputs)){
      CANNOT_DECODE_VALUE(FATAL, inputs, InputList);
      return false;
    }
    DLOG(INFO) << "decoded inputs (InputList), " << inputs.size() << " items.";

    OutputList outputs;
    if(!DecodeList<Output, Output::Decoder>(buff, outputs)){
      CANNOT_DECODE_VALUE(FATAL, outputs, OutputList);
      return false;
    }
    DLOG(INFO) << "decoded outputs (OutputList), " << outputs.size() << " items.";

    int64_t index = buff->GetLong();
    DLOG(INFO) << "decoded IndexedTransaction index: " << index;

    result = IndexedTransaction(timestamp, index, inputs, outputs);
    return true;
  }
}