#include "transaction_reference.h"

namespace token{
  namespace codec{
    int64_t TransactionReferenceEncoder::GetBufferSize() const{
      //TODO: implement
      return 0;
    }

    bool TransactionReferenceEncoder::Encode(const BufferPtr& buff) const{
      //TODO: implement
      return false;
    }

    bool TransactionReferenceDecoder::Decode(const BufferPtr& buff, TransactionReference& result) const{
      //TODO: implement
      return false;
    }
  }
}