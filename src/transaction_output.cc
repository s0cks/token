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

  BufferPtr Output::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode Output.";
      buffer->clear();
      return buffer;
    }
    return buffer;
  }

  bool Output::Encoder::Encode(const BufferPtr& buff) const{
    if(ShouldEncodeVersion()){
      if(!buff->PutVersion(codec::GetCurrentVersion())){
        CANNOT_ENCODE_CODEC_VERSION(FATAL);
        return false;
      }
      DVLOG(3) << "encoded version: " << codec::GetCurrentVersion();
    }

    const User& user = value().GetUser();
    if(!buff->PutUser(user)){
      CANNOT_ENCODE_VALUE(FATAL, User, user);
      return false;
    }
    DVLOG(3) << "encoded user: " << user;

    const Product& product = value().GetProduct();
    if(!buff->PutProduct(product)){
      CANNOT_ENCODE_VALUE(FATAL, Product, product);
      return false;
    }
    return true;
  }

  bool Output::Decoder::Decode(const BufferPtr& buff, Output& result) const{
    CHECK_CODEC_VERSION(FATAL, buff);

    User user = buff->GetUser();
    DVLOG(3) << "decoded user: " << user;

    Product product = buff->GetProduct();
    DVLOG(3) << "decoded product: " << product;
    result = Output(user, product);
    return true;
  }
}