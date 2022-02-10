#include "block_header.h"

namespace token{
  int64_t BlockHeader::Encoder::GetBufferSize() const{
    int64_t size = TypeEncoder<BlockHeader>::GetBufferSize();
    size += sizeof(RawTimestamp); // timestamp
    size += sizeof(int64_t); // height
    size += Hash::GetSize(); // previous hash
    size += Hash::GetSize(); // merkle root
    size += Hash::GetSize(); // hash
    return size;
  }

  bool BlockHeader::Encoder::Encode(const BufferPtr &buff) const{
    if(!TypeEncoder<BlockHeader>::Encode(buff))
      return false;
    const auto& timestamp = value()->timestamp();
    if(!buff->PutTimestamp(timestamp)){
      LOG(FATAL) << "cannot serialize timestamp to buffer.";
      return false;
    }
    const auto& height = value()->height();
    if(!buff->PutLong(height)){
      LOG(FATAL) << "cannot serialize height to buffer.";
      return false;
    }
    const auto& previous_hash = value()->previous_hash();
    if(!buff->PutHash(previous_hash)){
      LOG(FATAL) << "cannot serialize hash to buffer.";
      return false;
    }
    const auto& merkle_root = value()->merkle_root();
    if(!buff->PutHash(merkle_root)){
      LOG(FATAL) << "cannot serialize merkle root to buffer.";
      return false;
    }
    const auto& hash = value()->hash();
    if(!buff->PutHash(hash)){
      LOG(FATAL) << "cannot serialize hash to buffer.";
      return false;
    }
    return true;
  }

  BlockHeader* BlockHeader::Decoder::Decode(const BufferPtr& data) const{
    //TODO: decode type
    //TODO: decode version
    auto timestamp = data->GetTimestamp();
    DECODED_FIELD(timestamp_, Timestamp, FormatTimestampReadable(timestamp));

    auto height = data->GetLong();
    DECODED_FIELD(height_, Long, height);

    auto previous_hash = data->GetHash();
    DECODED_FIELD(previous_hash_, Hash, previous_hash);

    auto merkle_root = data->GetHash();
    DECODED_FIELD(merkle_root_, Hash, merkle_root);

    auto hash = data->GetHash();
    DECODED_FIELD(hash_, Hash, hash);

    return new BlockHeader(timestamp, height, previous_hash, merkle_root, hash);
  }
}