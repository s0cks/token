#include "rpc/rpc_message_block.h"

namespace token{
  namespace codec{
    bool BlockMessageDecoder::Decode(const BufferPtr& buff, rpc::BlockMessage& result) const{
      Block value;
      if(!decode_value_.Decode(buff, value)){
        LOG(FATAL) << "cannot decode value from buffer.";
        return false;
      }

      //TODO: fix allocation
      result = rpc::BlockMessage(std::make_shared<Block>(value));
      return true;
    }
  }
}