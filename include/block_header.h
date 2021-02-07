#ifndef TOKEN_BLOCK_HEADER_H
#define TOKEN_BLOCK_HEADER_H

#include "block.h"

namespace token{
  class BlockHeader : public SerializableObject{
   public:
    static const int64_t kSize = sizeof(Timestamp) // timestamp
                               + sizeof(RawVersion) // version
                               + sizeof(int64_t) // height
                               + Hash::kSize // previous hash
                               + Hash::kSize // merkle root
                               + Hash::kSize // hash
                               + sizeof(int64_t); // number of transactions
    //TODO: add bloom filter?
   private:
    Timestamp timestamp_;
    Version version_;
    int64_t height_;
    Hash previous_hash_;
    Hash merkle_root_;
    Hash hash_;
    int64_t num_transactions_;
   public:
    BlockHeader():
      SerializableObject(),
      timestamp_(Clock::now()),
      version_(Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION)),
      height_(0),
      previous_hash_(),
      merkle_root_(), // fill w/ genesis's merkle root
      hash_(), //TODO: fill w/ genesis's Hash
      num_transactions_(0){}
    BlockHeader(Timestamp timestamp,
                const Version& version,
                int64_t height,
                const Hash& phash,
                const Hash& merkle_root,
                const Hash& hash,
                int64_t num_transactions):
      SerializableObject(),
      timestamp_(timestamp),
      version_(version),
      height_(height),
      previous_hash_(phash),
      merkle_root_(merkle_root),
      hash_(hash),
      num_transactions_(num_transactions){}
    BlockHeader(const BlockPtr& blk):
      SerializableObject(),
      timestamp_(blk->GetTimestamp()),
      version_(blk->GetVersion()),
      height_(blk->GetHeight()),
      previous_hash_(blk->GetPreviousHash()),
      merkle_root_(blk->GetMerkleRoot()),
      hash_(blk->GetHash()),
      num_transactions_(blk->GetNumberOfTransactions()){}
    BlockHeader(const BufferPtr& buffer):
      SerializableObject(),
      timestamp_(buffer->GetTimestamp()),
      version_(buffer->GetVersion()),
      height_(buffer->GetLong()),
      previous_hash_(buffer->GetHash()),
      merkle_root_(buffer->GetHash()),
      hash_(buffer->GetHash()),
      num_transactions_(buffer->GetLong()){}
    BlockHeader(const BlockHeader& blk):
      SerializableObject(),
      timestamp_(blk.timestamp_),
      version_(blk.version_),
      height_(blk.height_),
      previous_hash_(blk.previous_hash_),
      merkle_root_(blk.merkle_root_),
      hash_(blk.hash_),
      num_transactions_(blk.num_transactions_){}
    ~BlockHeader(){}

    Type GetType() const{
      return Type::kBlockHeader;
    }

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    int64_t GetHeight() const{
      return height_;
    }

    Hash GetPreviousHash() const{
      return previous_hash_;
    }

    Hash GetMerkleRoot() const{
      return merkle_root_;
    }

    Hash GetHash() const{
      return hash_;
    }

    int64_t GetNumberOfTransactions() const{
      return num_transactions_;
    }

    BlockHeader& operator=(const BlockHeader& other){
      timestamp_ = other.timestamp_;
      version_ = other.version_;
      height_ = other.height_;
      previous_hash_ = other.previous_hash_;
      merkle_root_ = other.merkle_root_;
      hash_ = other.hash_;
      num_transactions_ = other.num_transactions_;
      return (*this);
    }

    int64_t GetBufferSize() const{
      return kSize;
    }

    bool Write(const BufferPtr& buffer) const{
      return buffer->PutTimestamp(timestamp_)
          && buffer->PutVersion(version_)
          && buffer->PutLong(height_)
          && buffer->PutHash(previous_hash_)
          && buffer->PutHash(merkle_root_)
          && buffer->PutHash(hash_)
          && buffer->PutLong(num_transactions_);
    }

    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
              && Json::SetField(writer, "timestamp", timestamp_)
              && Json::SetField(writer, "version", version_)
              && Json::SetField(writer, "height", height_)
              && Json::SetField(writer, "previous_hash", previous_hash_)
              && Json::SetField(writer, "merkle_root", merkle_root_)
              && Json::SetField(writer, "hash", hash_)
              && Json::SetField(writer, "number_of_transactions", num_transactions_)
           && writer.EndObject();
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "BlockHeader(";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "version=" << version_ << ", ";
      ss << "height=" << height_ << ", ";
      ss << "previous_hash=" << previous_hash_ << ", ";
      ss << "merkle_root=" << merkle_root_ << ", ";
      ss << "hash=" << hash_ << ", ";
      ss << "number_of_transactions=" << num_transactions_;
      ss << ")";
      return ss.str();
    }

    friend bool operator==(const BlockHeader& a, const BlockHeader& b){
      return a.GetHash() == b.GetHash();
    }

    friend bool operator!=(const BlockHeader& a, const BlockHeader& b){
      return a.GetHash() != b.GetHash();
    }

    friend bool operator<(const BlockHeader& a, const BlockHeader& b){
      return a.GetHeight() < b.GetHeight();
    }

    friend std::ostream& operator<<(std::ostream& stream, const BlockHeader& blk){
      return stream << blk.ToString();
    }
  };
}

#endif//TOKEN_BLOCK_HEADER_H