#include "unsigned_transaction.h"
#include "network/rpc_message_transaction.h"

namespace token{
  namespace rpc{
    std::string TransactionMessage::ToString() const{
      std::stringstream ss;
      ss << "TransactionMessage(value=" << value()->hash() << ")";
      return ss.str();
    }

    int64_t TransactionMessage::Encoder::GetBufferSize() const{
      NOT_IMPLEMENTED(ERROR);
      return 0;
    }

    bool TransactionMessage::Encoder::Encode(const BufferPtr& buff) const{
      NOT_IMPLEMENTED(ERROR);
      return false;
    }

    bool TransactionMessage::Decoder::Decode(const BufferPtr& buff, TransactionMessage& val) const{
      NOT_IMPLEMENTED(ERROR);
      return false;
    }
  }
}