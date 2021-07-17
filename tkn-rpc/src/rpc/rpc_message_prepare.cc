#include "rpc/rpc_message_prepare.h"

namespace token{
  namespace codec{
    bool PrepareMessageDecoder::Decode(const BufferPtr& buff, rpc::PrepareMessage& result) const{
      Proposal proposal;
      if(!DecodeProposalData(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal data.";
        return false;
      }

      result = rpc::PrepareMessage(proposal);
      return true;
    }
  }
}