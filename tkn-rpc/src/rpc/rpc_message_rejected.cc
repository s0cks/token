#include "rpc/rpc_message_rejected.h"

namespace token{
  namespace rpc{
    RejectedMessage* RejectedMessage::Decoder::Decode(const BufferPtr& data) const{
      Proposal* proposal = nullptr;
      if(!(proposal = DecodeProposal(data)))
        CANNOT_DECODE_FIELD(proposal_, Proposal);
      return new RejectedMessage(*proposal);//TODO: fix memory-leak
    }
  }
}