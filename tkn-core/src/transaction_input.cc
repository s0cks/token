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

  namespace codec{
    InputEncoder::InputEncoder(const Input &value, const codec::EncoderFlags &flags):
      codec::EncoderBase<Input>(value, flags),
      encode_txref_(value.GetReference(), flags),
      encode_user_(value.GetUser(), flags){}

    int64_t InputEncoder::GetBufferSize() const {
      auto size = codec::EncoderBase<Input>::GetBufferSize();
      size += encode_txref_.GetBufferSize();
      size += encode_user_.GetBufferSize();
      return size;
    }

    bool InputEncoder::Encode(const BufferPtr& buff) const{
      if(!BaseType::Encode(buff))
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

    bool InputDecoder::Decode(const BufferPtr& buff, Input& result) const{
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
}