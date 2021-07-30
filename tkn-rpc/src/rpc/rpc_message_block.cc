#include "rpc/rpc_message_block.h"

namespace token{
  namespace rpc{
    BlockMessage* BlockMessage::Decoder::Decode(const BufferPtr& data) const{
      Block* value = nullptr;
      if(!(value = decode_value_.Decode(data)))
        CANNOT_DECODE_FIELD(value_, Block);
      return new BlockMessage(std::shared_ptr<Block>(value));//TODO: fix allocation
    }
  }
}