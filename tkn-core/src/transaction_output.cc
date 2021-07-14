#include "buffer.h"
#include "transaction_output.h"

namespace token{
  std::string Output::ToString() const{
    std::stringstream ss;
    ss << "Output(";
    ss << "user=" << GetUser() << ",";
    ss << "product=" << GetProduct();
    ss << ")";
    return ss.str();
  }

  namespace codec{
    OutputEncoder::OutputEncoder(const Output &value, const codec::EncoderFlags &flags):
      codec::EncoderBase<Output>(value, flags),
      encode_user_( value.GetUser(), flags),
      encode_product_(value.GetProduct(), flags){}

    int64_t OutputEncoder::GetBufferSize() const{
      auto size = BaseType::GetBufferSize();
      size += encode_user_.GetBufferSize();
      size += encode_product_.GetBufferSize();
      return size;
    }

    bool OutputEncoder::Encode(const BufferPtr &buff) const{
      if(!BaseType::Encode(buff)){
        LOG(FATAL) << "TODO";
        return false;
      }

      if(!encode_user_.Encode(buff)){
        LOG(FATAL) << "cannot encode user to buffer.";
        return false;
      }

      if(!encode_product_.Encode(buff)){
        LOG(FATAL) << "cannot encode product to buffer.";
        return false;
      }
      return true;
    }

    OutputDecoder::OutputDecoder(const codec::DecoderHints &hints):
      codec::DecoderBase<Output>(hints),
      decode_user_(hints),
      decode_product_(hints){}

    bool OutputDecoder::Decode(const BufferPtr &buff, Output &result) const{
      User user;
      if(!decode_user_.Decode(buff, user)){
        LOG(FATAL) << "cannot decode user from buffer.";
        return false;
      }

      Product product;
      if(!decode_product_.Decode(buff, product)){
        LOG(FATAL) << "cannot decode product from buffer.";
        return false;
      }

      result = Output(user, product);
      return true;
    }
  }
}