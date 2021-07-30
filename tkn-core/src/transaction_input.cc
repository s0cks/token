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

  Input* Input::Decoder::Decode(const BufferPtr& data) const{
    // Decode reference_
    TransactionReference* reference = nullptr;
    if(!(reference = decode_txref_.Decode(data)))
      CANNOT_DECODE_FIELD(reference_, TransactionReference);

    User* user = nullptr;
    if(!(user = decode_user_.Decode(data)))
      CANNOT_DECODE_FIELD(user_, User);
    return new Input(*reference, *user);//TODO: fix allocation
  }
}