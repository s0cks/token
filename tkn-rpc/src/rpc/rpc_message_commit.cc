#include "rpc/rpc_message_commit.h"

namespace token{
  namespace codec{
    bool CommitMessageDecoder::Decode(const BufferPtr& buff, rpc::CommitMessage& result) const{
      Proposal proposal;
      if(!DecodeProposalData(buff, proposal)){
        LOG(FATAL) << "cannot decode proposal from buffer.";
        return false;
      }
      result = rpc::CommitMessage(proposal);
      return true;
    }
  }
}