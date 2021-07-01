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
      int64_t size = ObjectMessageEncoder<BlockMessage>::GetBufferSize();
      //TODO: compute size
      return size;
    }

    bool BlockMessage::Encoder::Encode(const BufferPtr& buff) const{
      //TODO: encode value
      return true;
    }

    bool BlockMessage::Decoder::Decode(const BufferPtr& buff, BlockMessage& result) const{
      //TODO: decode value
      return true;
    }
  }
}