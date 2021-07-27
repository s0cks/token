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

  Input::Encoder::Encoder(const Input* value, const codec::EncoderFlags &flags):
    codec::TypeEncoder<Input>(value, flags),
    encode_txref_(&value->reference_, flags),
    encode_user_(&value->user_, flags){}

  int64_t Input::Encoder::GetBufferSize() const {
    auto size = codec::TypeEncoder<Input>::GetBufferSize();
    size += encode_txref_.GetBufferSize();
    size += encode_user_.GetBufferSize();
    return size;
  }

  bool Input::Encoder::Encode(const BufferPtr& buff) const{
    if(!TypeEncoder<Input>::Encode(buff))
      return false;

    if(!encode_txref_.Encode(buff)){
      LOG(FATAL) << "couldn't encoder transaction reference to buffer.";
      return false;
    }

    if(!encode_user_.Encode(buff)){
      LOG(FATAL) << "couldn't encoder user to buffer.";
      return false;
    }
    return true;
  }

  bool Input::Decoder::Decode(const BufferPtr& buff, Input& result) const{
    // Decode reference_
    TransactionReference reference;
    if(!decode_txref_.Decode(buff, reference)){
      LOG(FATAL) << "couldn't decode transaction reference from buffer.";
      return false;
    }

    // Decode user_
    User user;
    if(!decode_user_.Decode(buff, user)){
      LOG(FATAL) << "couldn't decode user from buffer.";
      return false;
    }

    result = Input(reference, user);
    return true;
  }
}