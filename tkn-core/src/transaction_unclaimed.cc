#include "buffer.h"
#include "transaction_unclaimed.h"

namespace token{
  BufferPtr UnclaimedTransaction::ToBuffer() const{
    codec::UnclaimedTransactionEncoder encoder((*this), codec::kDefaultEncoderFlags);
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

  namespace codec{
    UnclaimedTransactionEncoder::UnclaimedTransactionEncoder(const UnclaimedTransaction &value,
                                                             const codec::EncoderFlags &flags):
        codec::EncoderBase<UnclaimedTransaction>(value, flags),
        encode_txref_(value.GetReference(), flags),
        encode_user_(value.GetUser(), flags),
        encode_product_(value.GetProduct(), flags){}

    int64_t UnclaimedTransactionEncoder::GetBufferSize() const{
      auto size = codec::EncoderBase<UnclaimedTransaction>::GetBufferSize();
      size += encode_txref_.GetBufferSize();
      size += encode_user_.GetBufferSize();
      size += encode_product_.GetBufferSize();
      return size;
    }

    bool UnclaimedTransactionEncoder::Encode(const BufferPtr& buff) const{
      if(!BaseType::Encode(buff))
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

    UnclaimedTransactionDecoder::UnclaimedTransactionDecoder(const codec::DecoderHints &hints):
      codec::DecoderBase<UnclaimedTransaction>(hints),
      decode_txref_(hints),
      decode_user_(hints),
      decode_product_(hints){}

    bool UnclaimedTransactionDecoder::Decode(const BufferPtr& buff, UnclaimedTransaction& result) const{
      TransactionReference reference;
      if(!decode_txref_.Decode(buff, reference)){
        LOG(FATAL) << "cannot decode transaction reference from buffer.";
        return false;
      }

      User user;
      if(!decode_user_.Decode(buff, user)){
        LOG(FATAL) << "cannot decode User from buffer.";
        return false;
      }

      Product product;
      if(!decode_product_.Decode(buff, product)){
        LOG(FATAL) << "cannot decode product from buffer.";
        return false;
      }

      result = UnclaimedTransaction(reference, user, product);
      return true;
    }
  }
}