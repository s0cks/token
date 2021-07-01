#include "transaction.h"
#include "network/rpc_message_transaction.h"

namespace token{
  namespace rpc{
    std::string TransactionMessage::ToString() const{
      std::stringstream ss;
      ss << "TransactionMessage(value=" << value()->hash() << ")";
      return ss.str();
    }

    int64_t TransactionMessage::Encoder::GetBufferSize() const{
      int64_t size = ObjectMessageEncoder<TransactionMessage>::GetBufferSize();

      return size;
    }

    bool TransactionMessage::Encoder::Encode(const BufferPtr& buff) const{
      return true;//TODO: encode value
    }

    bool TransactionMessage::Decoder::Decode(const BufferPtr& buff, TransactionMessage& val) const{
      return true;//TODO: decode value
    }
  }
}