#include "network/rpc_message_accepts.h"

namespace token{
  namespace rpc{
    bool AcceptsMessage::Decoder::Decode(const BufferPtr &buff, AcceptsMessage &result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = AcceptsMessage(proposal);
      return true;
    }
  }
}