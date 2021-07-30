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

  Output::Encoder::Encoder(const Output* value, const codec::EncoderFlags &flags):
    codec::TypeEncoder<Output>(value, flags),
    encode_user_(&value->user_, flags),
    encode_product_(&value->product_, flags){}

  int64_t Output::Encoder::GetBufferSize() const{
    auto size = TypeEncoder<Output>::GetBufferSize();
    size += encode_user_.GetBufferSize();
    size += encode_product_.GetBufferSize();
    return size;
  }

  bool Output::Encoder::Encode(const BufferPtr &buff) const{
    if(!TypeEncoder<Output>::Encode(buff)){
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

  Output::Decoder::Decoder(const codec::DecoderHints &hints):
    codec::TypeDecoder<Output>(hints),
    decode_user_(hints),
    decode_product_(hints){}

  Output* Output::Decoder::Decode(const BufferPtr& data) const{
    User* user = nullptr;
    if(!(user = decode_user_.Decode(data))){
      LOG(FATAL) << "cannot decode user from buffer.";
      return nullptr;
    }

    Product* product = nullptr;
    if(!(product = decode_product_.Decode(data))){
      LOG(FATAL) << "cannot decode product from buffer.";
      return nullptr;
    }

    //TODO: fix memory leak
    return new Output(*user, *product);
  }
}