#include "buffer.h"
#include "network/rpc_message_prepare.h"

namespace token{
  namespace rpc{
    bool PrepareMessage::Encoder::Encode(const BufferPtr& buff) const{
      if(ShouldEncodeVersion()){
        if(!buff->PutVersion(codec::GetCurrentVersion())){
          CANNOT_ENCODE_CODEC_VERSION(FATAL);
          return false;
        }
        DLOG(INFO) << "encoded codec Version.";
      }
      const RawProposal& proposal = value().proposal();
      if(!buff->PutProposal(proposal)){
        CANNOT_ENCODE_VALUE(FATAL, RawProposal, proposal);
        return false;
      }
      DLOG(INFO) << "encoded RawProposal: " << proposal;
      return true;
    }

    bool PrepareMessage::Decoder::Decode(const BufferPtr& buff, PrepareMessage& result) const{
      CHECK_CODEC_VERSION(FATAL, buff);
      RawProposal proposal = buff->GetProposal();
      result = PrepareMessage(proposal);
      return true;
    }
  }
}