#include "buffer.h"
#include "unclaimed_transaction.h"

namespace token{
  BufferPtr UnclaimedTransaction::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if (!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode UnclaimedTransaction.";
      buffer->clear();
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
    size += TransactionReference::GetSize();
    size += kUserSize;
    size += kProductSize;
    return size;
  }

  bool UnclaimedTransaction::Encoder::Encode(const BufferPtr& buff) const{
    if (ShouldEncodeVersion()){
      if (!buff->PutVersion(codec::GetCurrentVersion())){
        LOG(FATAL) << "cannot encode codec version.";
        return false;
      }
      DLOG(INFO) << "encoded codec version: " << codec::GetCurrentVersion();
    }

    const TransactionReference& reference = value().GetReference();
    if (!buff->PutReference(reference)){
      CANNOT_ENCODE_VALUE(FATAL, TransactionReference, reference);
      return false;
    }

    const User& user = value().GetUser();
    if (!buff->PutUser(user)){
      CANNOT_ENCODE_VALUE(FATAL, User, user);
      return false;
    }

    const Product& product = value().GetProduct();
    if (!buff->PutProduct(product)){
      CANNOT_ENCODE_VALUE(FATAL, Product, product);
      return false;
    }
    return true;
  }

  bool UnclaimedTransaction::Decoder::Decode(const BufferPtr& buff, UnclaimedTransaction& result) const{
    CHECK_CODEC_VERSION(FATAL, buff);

    TransactionReference reference = buff->GetReference();
    DLOG(INFO) << "decoded UnclaimedTransaction TransactionReference: " << reference;

    User user = buff->GetUser();
    DLOG(INFO) << "decoded UnclaimedTransaction User: " << user;

    Product product = buff->GetProduct();
    DLOG(INFO) << "decoded UnclaimedTransaction Product: " << product;

    result = UnclaimedTransaction(reference, user, product);
    return true;
  }
}