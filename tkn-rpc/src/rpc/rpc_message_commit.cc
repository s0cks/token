#include "rpc/rpc_message_commit.h"

namespace token{
  namespace rpc{
    CommitMessage* CommitMessage::Decoder::Decode(const BufferPtr& data) const{
      Proposal* proposal = nullptr;
      if(!(proposal = DecodeProposal(data)))
        CANNOT_DECODE_FIELD(proposal_, Proposal);
      return new CommitMessage(*proposal);//TODO: fix allocation
    }
  }
}