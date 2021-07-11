#include "network/rpc_message_rejected.h"

namespace token{
  namespace rpc{
    bool RejectedMessage::Decoder::Decode(const BufferPtr &buff, RejectedMessage &result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = RejectedMessage(proposal);
      return true;
    }
  }
}