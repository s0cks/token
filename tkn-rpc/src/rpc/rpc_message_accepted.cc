#include "buffer.h"
#include "rpc/rpc_message_accepted.h"

namespace token{
  namespace rpc{
    bool AcceptedMessage::Decoder::Decode(const BufferPtr& buff, rpc::AcceptedMessage& result) const{
      //TODO: decode type
      //TODO: decode version

      Proposal proposal;
      if(!DecodeProposalData(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = rpc::AcceptedMessage(proposal);
      return true;
    }
  }
}