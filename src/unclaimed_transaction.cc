#include "buffer.h"
#include "unclaimed_transaction.h"

namespace token{
  BufferPtr UnclaimedTransaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = internal::For(encoder);
    if (!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode UnclaimedTransaction.";
      //TODO: clear buffer
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

  int64_t UnclaimedTransaction::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<UnclaimedTransaction>::GetBufferSize();
    size += txref_encoder_.GetBufferSize();
    size += user_encoder_.GetBufferSize();
    size += product_encoder_.GetBufferSize();
    return size;
  }

  bool UnclaimedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    if(!BaseType::Encode(buff))
      return false;

    if(!txref_encoder_.Encode(buff)){
      LOG(FATAL) << "cannot encode transaction reference to buffer.";
      return false;
    }

    if(!user_encoder_.Encode(buff)){
      LOG(FATAL) << "cannot encode user to buffer.";
      return false;
    }

    if(!product_encoder_.Encode(buff)){
      LOG(FATAL) << "cannot encode product to buffer.";
      return false;
    }
    return true;
  }

  bool UnclaimedTransaction::Decoder::Decode(const BufferPtr& buff, UnclaimedTransaction& result) const{
    TransactionReference reference;
    if(!txref_decoder_.Decode(buff, reference)){
      LOG(FATAL) << "cannot decode transaction reference from buffer.";
      return false;
    }

    User user;
    if(!user_decoder_.Decode(buff, user)){
      LOG(FATAL) << "cannot decode User from buffer.";
      return false;
    }

    Product product;
    if(!product_decoder_.Decode(buff, product)){
      LOG(FATAL) << "cannot decode product from buffer.";
      return false;
    }

    result = UnclaimedTransaction(reference, user, product);
    return true;
  }
}