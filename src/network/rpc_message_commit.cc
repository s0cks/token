#include "network/rpc_message_commit.h"

namespace token{
  namespace rpc{
    bool CommitMessage::Decoder::Decode(const BufferPtr &buff, CommitMessage &result) const{
      //TODO: decode type
      //TODO: decode version

      RawProposal proposal;
      if(!proposal_decoder_.Decode(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = CommitMessage(proposal);
      return true;
    }
  }
}