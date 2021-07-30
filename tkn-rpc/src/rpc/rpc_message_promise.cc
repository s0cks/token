#include "rpc/rpc_message_promise.h"

namespace token{
  namespace rpc{
    PromiseMessage* PromiseMessage::Decoder::Decode(const BufferPtr& data) const{
      Proposal* proposal = nullptr;
      if(!(proposal = DecodeProposal(data)))
        CANNOT_DECODE_FIELD(proposal_, Proposal);
      return new PromiseMessage(*proposal);//TODO: fix memory-leak
    }
  }
}