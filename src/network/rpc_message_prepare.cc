#include "buffer.h"
#include "network/rpc_message_prepare.h"

namespace token{
  namespace rpc{
    bool PrepareMessage::Decoder::Decode(const BufferPtr& buff, PrepareMessage& result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "couldn't decode proposal from buffer.";
        return false;
      }

      result = PrepareMessage(proposal);
      return true;
    }
  }
}