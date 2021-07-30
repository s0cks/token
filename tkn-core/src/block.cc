#include "block.h"

namespace token{
  std::string Block::ToString() const{
    std::stringstream stream;
    stream << "Block(#" << height() << ", " << GetNumberOfTransactions() << " Transactions)";
    return stream.str();
  }

  BlockPtr Block::Genesis(){
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);

    int64_t idx;
    InputList inputs;

    OutputList outputs_a;
    for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
      outputs_a.push_back(Output("VenueA", "TestToken"));
    }

    OutputList outputs_b;
    for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
      outputs_b.push_back(Output("VenueB", "TestToken"));
    }

    OutputList outputs_c;
    for(idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++){
      outputs_c.push_back(Output("VenueC", "TestToken"));
    }

    IndexedTransactionSet transactions;
    transactions.insert(IndexedTransaction::NewInstance(1, FromUnixTimestamp(0), inputs, outputs_a));
    transactions.insert(IndexedTransaction::NewInstance(2, FromUnixTimestamp(0), inputs, outputs_b));
    transactions.insert(IndexedTransaction::NewInstance(3, FromUnixTimestamp(0), inputs, outputs_c));
    return std::make_shared<Block>(0, version, Hash(), transactions);
  }

  bool Block::Accept(BlockVisitor* vis) const{
    if(!vis->VisitStart()){
      return false;
    }
    for(auto& tx : transactions_){
      if(!vis->Visit(tx)){
        return false;
      }
    }
    return vis->VisitEnd();
  }

  bool Block::Contains(const Hash& hash) const{
    return tx_bloom_.Contains(hash);
  }

  Hash Block::GetMerkleRoot() const{
    /*
     TODO:MerkleTreeBuilder builder;
    if(!Accept(&builder)){
      return Hash();
    }
    std::shared_ptr<MerkleTree> tree = builder.Build();
    return tree->GetRootHash();*/
    return Hash();
  }

  BufferPtr Block::ToBuffer() const{
    Encoder encoder((Block*)this, codec::kDefaultEncoderFlags);
    BufferPtr buffer = internal::NewBufferFor(encoder);
    if(!encoder.Encode(buffer)){
      LOG(FATAL) << "cannot encode block to buffer.";
      return nullptr;
    }
    return buffer;
  }
  
  int64_t Block::Encoder::GetBufferSize() const{
    auto size = codec::TypeEncoder<Block>::GetBufferSize();
    size += sizeof(RawTimestamp); // timestamp_
    size += sizeof(int64_t); // height_
    size += Hash::GetSize(); // previous_hash_
    size += Hash::GetSize(); // merkle_root_
    size += encode_transactions_.GetBufferSize();
    return size;
  }

  bool Block::Encoder::Encode(const BufferPtr& buff) const{
    if(!TypeEncoder<Block>::Encode(buff))
      return false;

    const auto& timestamp = value()->timestamp();
    if(!buff->PutTimestamp(timestamp)){
      CANNOT_ENCODE_VALUE(FATAL, Timestamp, ToUnixTimestamp(timestamp));
      return false;
    }

    const auto& height = value()->height();
    if(!buff->PutLong(height)){
      CANNOT_ENCODE_VALUE(FATAL, int64_t, height);
      return false;
    }

    const auto& previous_hash = value()->GetPreviousHash();
    if(!buff->PutHash(previous_hash)){
      CANNOT_ENCODE_VALUE(FATAL, Hash, previous_hash);
      return false;
    }

    if(!encode_transactions_.Encode(buff)){
      CANNOT_ENCODE_VALUE(FATAL, IndexedTransactionSet, value()->transactions().size());
      return false;
    }
    return true;
  }

  Block* Block::Decoder::Decode(const BufferPtr& data) const{
    Timestamp timestamp = data->GetTimestamp();
    DECODED_FIELD(timestamp_, Timestamp, FormatTimestampReadable(timestamp));

    int64_t height = data->GetLong();
    DECODED_FIELD(height_, Long, height);

    Hash previous_hash = data->GetHash();
    DECODED_FIELD(previous_hash_, Hash, previous_hash);

    Version version = Version::CurrentVersion();
    DECODED_FIELD(version_, Version, version);

    IndexedTransactionSet transactions = {};
    if(!decode_transactions_.Decode(data, transactions)){
      DLOG(FATAL) << "cannot decode transaction data from buffer.";
      return nullptr;
    }
    return new Block(timestamp, height, previous_hash, transactions, version);
  }
}