#include "network/rpc_message_promise.h"

namespace token{
  namespace rpc{
    bool PromiseMessage::Decoder::Decode(const BufferPtr &buff, PromiseMessage &result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = PromiseMessage(proposal);
      return true;
    }
  }
}