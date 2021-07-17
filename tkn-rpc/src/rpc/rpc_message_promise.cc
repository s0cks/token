#include "rpc/rpc_message_promise.h"

namespace token{
  namespace codec{
    bool PromiseMessageDecoder::Decode(const BufferPtr& buff, rpc::PromiseMessage& result) const{
      Proposal proposal;
      if(!DecodeProposalData(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal data from buffer.";
        return false;
      }

      result = rpc::PromiseMessage(proposal);
      return true;
    }
  }
}