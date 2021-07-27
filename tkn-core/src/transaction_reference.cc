#include "transaction_reference.h"

namespace token{
  int64_t TransactionReference::Encoder::GetBufferSize() const{
    auto size = codec::TypeEncoder<TransactionReference>::GetBufferSize();
    size += Hash::GetSize();
    size += sizeof(int64_t);
    return size;
  }

  bool TransactionReference::Encoder::Encode(const BufferPtr& buff) const{
    const auto& transaction = value()->transaction();
    if(!buff->PutHash(transaction)){
      DLOG(FATAL) << "cannot encode transaction (Hash).";
      return false;
    }
    ENCODED_FIELD(transaction_, Hash, transaction);

    const auto& index = value()->index();
    if(!buff->PutLong(index)){
      DLOG(FATAL) << "cannot encode index (int64_t)";
      return false;
    }
    ENCODED_FIELD(index_, int64_t, index);
    return true;
  }

  bool TransactionReference::Decoder::Decode(const BufferPtr& buff, TransactionReference& result) const{
    auto transaction = buff->GetHash();
    DECODED_FIELD(transaction_, Hash, transaction);

    auto index = buff->GetLong();
    DECODED_FIELD(index_, int64_t, index);

    result = TransactionReference(transaction, index);
    return true;
  }
}