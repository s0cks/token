#include "user.h"
#include "buffer.h"
#include "transaction_input.h"

namespace token{
  std::string Input::ToString() const{
    std::stringstream stream;
    stream << "Input(";
    stream << "reference=" << GetReference() << ",";
    stream << "user=" << GetUser();
    return stream.str();
  }

  BufferPtr Input::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode Input.";
      buffer->clear();
      return buffer;
    }
    return buffer;
  }

  int64_t Input::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<Input>::GetBufferSize();
    size += TransactionReference::GetSize();
    size += User::GetSize();
    return size;
  }

  bool Input::Encoder::Encode(const BufferPtr& buff) const{
    if(ShouldEncodeVersion()){
      if(!buff->PutVersion(codec::GetCurrentVersion())){
        CANNOT_ENCODE_CODEC_VERSION(FATAL);
        return false;
      }
      DLOG(INFO) << "encoded version: " << codec::GetCurrentVersion();
    }

    // Encode reference_
    const TransactionReference& reference = value().GetReference();
    if(!buff->PutReference(reference)){
      CANNOT_ENCODE_VALUE(FATAL, TransactionReference, reference);
      return false;
    }
    DLOG(INFO) << "encoded reference: " << reference;

    // Encode user_
    const User& user = value().GetUser();
    if(!buff->PutUser(user)){
      CANNOT_ENCODE_VALUE(FATAL, User, user);
      return false;
    }
    DLOG(INFO) << "encoded user: " << user;
    return true;
  }

  bool Input::Decoder::Decode(const BufferPtr& buff, Input& result) const{
    CHECK_CODEC_VERSION(FATAL, buff);

    // Decode reference_
    TransactionReference reference = buff->GetReference();
    DLOG(INFO) << "decoded reference: " << reference;

    // Decode user_
    User user = buff->GetUser();
    DLOG(INFO) << "decoded user: " << user;
    result = Input(reference, user);
    return true;
  }
}