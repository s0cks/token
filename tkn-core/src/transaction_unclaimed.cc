#include "buffer.h"
#include "transaction_unclaimed.h"

namespace token{
  BufferPtr UnclaimedTransaction::ToBuffer() const{
    Encoder encoder((UnclaimedTransaction*)this, codec::kDefaultEncoderFlags);
    BufferPtr buffer = internal::NewBufferFor(encoder);
    if(!encoder.Encode(buffer)){
      LOG(FATAL) << "cannot encode unclaimed transaction to buffer.";
      return nullptr;
    }
    return buffer;
  }

  std::string UnclaimedTransaction::ToString() const{
    std::stringstream ss;
    ss << "UnclaimedTransaction(";
    ss << "reference=" << reference_ << ", ";
    ss << "user=" << user_ << ", ";
    ss << "product=" << product_;
    ss << ")";
    return ss.str();
  }

  UnclaimedTransaction::Encoder::Encoder(const UnclaimedTransaction* value, const codec::EncoderFlags &flags):
    codec::TypeEncoder<UnclaimedTransaction>(value, flags),
    encode_txref_(&value->reference_, flags),
    encode_user_(&value->user_, flags),
    encode_product_(&value->product_, flags){}

  int64_t UnclaimedTransaction::Encoder::GetBufferSize() const{
    auto size = codec::TypeEncoder<UnclaimedTransaction>::GetBufferSize();
    size += encode_txref_.GetBufferSize();
    size += encode_user_.GetBufferSize();
    size += encode_product_.GetBufferSize();
    return size;
  }

  bool UnclaimedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    if(!TypeEncoder<UnclaimedTransaction>::Encode(buff))
      return false;

    if(!encode_txref_.Encode(buff)){
      LOG(FATAL) << "cannot encoder transaction reference to buffer.";
      return false;
    }

    if(!encode_user_.Encode(buff)){
      LOG(FATAL) << "cannot encoder user to buffer.";
      return false;
    }

    if(!encode_product_.Encode(buff)){
      LOG(FATAL) << "cannot encoder product to buffer.";
      return false;
    }
    return true;
  }

  UnclaimedTransaction::Decoder::Decoder(const codec::DecoderHints &hints):
    codec::TypeDecoder<UnclaimedTransaction>(hints),
    decode_txref_(hints),
    decode_user_(hints),
    decode_product_(hints){}

  UnclaimedTransaction* UnclaimedTransaction::Decoder::Decode(const BufferPtr& data) const{
    TransactionReference* reference = nullptr;
    if(!(reference = decode_txref_.Decode(data)))
      CANNOT_DECODE_FIELD(reference_, TransactionReference);
    User* user = nullptr;
    if(!(user = decode_user_.Decode(data)))
      CANNOT_DECODE_FIELD(user_, User);
    Product* product = nullptr;
    if(!(product = decode_product_.Decode(data)))
      CANNOT_DECODE_FIELD(product_, Product);
    return new UnclaimedTransaction(*reference, *user, *product);//TODO: fix allocation
  }
}