#include "rpc/rpc_message_rejected.h"

namespace token{
  namespace codec{
    bool RejectedMessageDecoder::Decode(const BufferPtr& buff, rpc::RejectedMessage& result) const{
      Proposal proposal;
      if(!DecodeProposalData(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }

      result = rpc::RejectedMessage(proposal);
      return true;
    }
  }
}