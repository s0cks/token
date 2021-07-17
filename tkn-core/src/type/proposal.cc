#include "type/proposal.h"

namespace token{
  namespace codec{
    ProposalEncoder::ProposalEncoder(const Proposal& value, const codec::EncoderFlags& flags):
      codec::EncoderBase<Proposal>(value, flags){}

    int64_t ProposalEncoder::GetBufferSize() const{
      auto size = codec::EncoderBase<Proposal>::GetBufferSize();
      size += sizeof(RawTimestamp);


      return size;
    }

    bool ProposalEncoder::Encode(const BufferPtr& buff) const{

      return true;
    }

    ProposalDecoder::ProposalDecoder(const codec::DecoderHints& hints):
      codec::DecoderBase<Proposal>(hints){}

    bool ProposalDecoder::Decode(const BufferPtr& buff, Proposal& result) const{

      return true;
    }
  }
}