#include "proposal.h"

namespace token{
  Proposal::Encoder::Encoder(const Proposal* value, const codec::EncoderFlags& flags):
    codec::TypeEncoder<Proposal>(value, flags){}

  int64_t Proposal::Encoder::GetBufferSize() const{
    auto size = TypeEncoder<Proposal>::GetBufferSize();
    size += sizeof(RawTimestamp);
    size += internal::kMaxUUIDLength;
    size += internal::kMaxUUIDLength;
    size += sizeof(int64_t);
    size += Hash::GetSize();
    return size;//TODO: cleanup sizes
  }

  bool Proposal::Encoder::Encode(const BufferPtr& buff) const{
    if(!codec::TypeEncoder<Proposal>::Encode(buff))
      return false;

    const auto& timestamp = value()->timestamp();
    if(!buff->PutTimestamp(timestamp)){
      DLOG(FATAL) << "cannot encode timestamp_";
      return false;
    }
    ENCODED_FIELD(timestamp_, Timestamp, FormatTimestampReadable(timestamp));

    const auto& proposal_id = value()->proposal_id();
    if(!buff->PutBytes((uint8_t*)proposal_id.data(), proposal_id.size())){
      DLOG(FATAL) << "cannot encode proposal_id_";
      return false;
    }
    ENCODED_FIELD(proposal_id_, UUID, proposal_id);

    const auto& proposer_id = value()->proposer_id();
    if(!buff->PutBytes((uint8_t*)proposer_id.data(), proposer_id.size())){
      DLOG(FATAL) << "cannot encode proposer_id_";
      return false;
    }
    ENCODED_FIELD(proposer_id_, UUID, proposer_id);

    const auto& height = value()->height();
    if(!buff->PutLong(height)){
      DLOG(FATAL) << "cannot encode height_";
      return false;
    }
    ENCODED_FIELD(height_, int64_t, height);

    const auto& hash = value()->hash();
    if(!buff->PutHash(hash)){
      DLOG(FATAL) << "cannot encode hash.";
      return false;
    }
    ENCODED_FIELD(hash_, Hash, hash);
    return true;
  }

  Proposal::Decoder::Decoder(const codec::DecoderHints& hints):
    codec::TypeDecoder<Proposal>(hints){}

  Proposal* Proposal::Decoder::Decode(const BufferPtr& data) const{
    //TODO: fix UUID decoding
    Timestamp timestamp = data->GetTimestamp();
    DECODED_FIELD(timestamp_, Timestamp, FormatTimestampReadable(timestamp));

    uint8_t proposal_id[internal::kMaxUUIDLength];
    if(!data->GetBytes(proposal_id, internal::kMaxUUIDLength))
      CANNOT_DECODE_FIELD(proposal_id, UUID);
    DECODED_FIELD(proposal_id_, UUID, UUID(proposal_id, internal::kMaxUUIDLength));

    uint8_t proposer_id[internal::kMaxUUIDLength];
    if(!data->GetBytes(proposer_id, internal::kMaxUUIDLength))
      CANNOT_DECODE_FIELD(proposer_id_, UUID);
    DECODED_FIELD(proposer_id_, UUID, UUID(proposer_id, internal::kMaxUUIDLength));

    int64_t height = data->GetLong();
    DECODED_FIELD(height_, int64_t, height);

    Hash hash = data->GetHash();
    DECODED_FIELD(hash_, Hash, hash);
    return new Proposal(timestamp, UUID(proposal_id, internal::kMaxUUIDLength), UUID(proposer_id, internal::kMaxUUIDLength), height, hash);
  }
}