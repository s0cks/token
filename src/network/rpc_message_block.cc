#include "block.h"
#include "network/rpc_message_block.h"

namespace token{
  namespace rpc{
    std::string BlockMessage::ToString() const{
      std::stringstream ss;
      ss << "BlockMessage(value=" << value()->hash() << ")";
      return ss.str();
    }

    int64_t BlockMessage::Encoder::GetBufferSize() const{
      NOT_IMPLEMENTED(ERROR);
      return 0;
    }

    bool BlockMessage::Encoder::Encode(const BufferPtr& buff) const{
      NOT_IMPLEMENTED(ERROR);
      return false;
    }

    bool BlockMessage::Decoder::Decode(const BufferPtr& buff, BlockMessage& result) const{
      NOT_IMPLEMENTED(ERROR);
      return false;
    }
  }
}