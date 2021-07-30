#include "rpc/rpc_message_prepare.h"

namespace token{
  namespace rpc{
    PrepareMessage* PrepareMessage::Decoder::Decode(const BufferPtr& data) const{
      Proposal* proposal = nullptr;
      if(!(proposal = DecodeProposal(data)))
        CANNOT_DECODE_FIELD(proposal_, Proposal);
      return new PrepareMessage(*proposal);//TODO: fix memory-leak
    }
  }
}