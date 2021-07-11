#include "network/rpc_message_accepted.h"

namespace token{
  namespace rpc{
    bool AcceptedMessage::Decoder::Decode(const BufferPtr &buff, AcceptedMessage &result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = AcceptedMessage(proposal);
      return true;
    }
  }
}