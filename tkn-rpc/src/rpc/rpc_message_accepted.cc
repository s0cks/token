#include "buffer.h"
#include "rpc/rpc_message_accepted.h"

namespace token{
  namespace rpc{
    AcceptedMessage* AcceptedMessage::Decoder::Decode(const BufferPtr& data) const{
      //TODO: decode type
      //TODO: decode version

      Proposal* proposal = nullptr;
      if(!(proposal = DecodeProposal(data)))
        CANNOT_DECODE_FIELD(proposal_, Proposal);
      return new AcceptedMessage(*proposal);//TODO: fix memory-leak
    }
  }
}