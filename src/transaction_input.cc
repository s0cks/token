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
    BufferPtr buffer = internal::For(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode Input.";
      //TODO: clear buffer
      return buffer;
    }
    return buffer;
  }

  int64_t Input::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<Input>::GetBufferSize();
    size += txref_encoder_.GetBufferSize();
    size += user_encoder_.GetBufferSize();
    return size;
  }

  bool Input::Encoder::Encode(const BufferPtr& buff) const{
    if(!BaseType::Encode(buff))
      return false;

    if(!txref_encoder_.Encode(buff)){
      LOG(FATAL) << "couldn't encode transaction reference to buffer.";
      return false;
    }

    if(!user_encoder_.Encode(buff)){
      LOG(FATAL) << "couldn't encode user to buffer.";
      return false;
    }
    return true;
  }

  bool Input::Decoder::Decode(const BufferPtr& buff, Input& result) const{
    // Decode reference_
    TransactionReference reference;
    if(!txref_decoder_.Decode(buff, reference)){
      LOG(FATAL) << "couldn't decode transaction reference from buffer.";
      return false;
    }

    // Decode user_
    User user;
    if(!user_decoder_.Decode(buff, user)){
      LOG(FATAL) << "couldn't decode user from buffer.";
      return false;
    }

    result = Input(reference, user);
    return true;
  }
}