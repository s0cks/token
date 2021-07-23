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
    codec::BlockEncoder encoder((*this), codec::kDefaultEncoderFlags);
    BufferPtr buffer = internal::NewBufferFor(encoder);
    if(!encoder.Encode(buffer)){
      LOG(FATAL) << "cannot encode block to buffer.";
      return nullptr;
    }
    return buffer;
  }

  namespace codec{
    int64_t BlockEncoder::GetBufferSize() const{
      auto size = codec::EncoderBase<Block>::GetBufferSize();
      size += sizeof(RawTimestamp); // timestamp_
      size += sizeof(int64_t); // height_
      size += Hash::GetSize(); // previous_hash_
      size += Hash::GetSize(); // merkle_root_
      //TODO: use IndexedTransactionListEncoder
      return size;
    }

    bool BlockEncoder::Encode(const BufferPtr& buff) const{
      if(!BaseType::Encode(buff))
        return false;

      const auto& timestamp = value().timestamp();
      if(!buff->PutTimestamp(timestamp)){
        CANNOT_ENCODE_VALUE(FATAL, Timestamp, ToUnixTimestamp(timestamp));
        return false;
      }

      const auto& height = value().height();
      if(!buff->PutLong(height)){
        CANNOT_ENCODE_VALUE(FATAL, int64_t, height);
        return false;
      }

      const auto& previous_hash = value().GetPreviousHash();
      if(!buff->PutHash(previous_hash)){
        CANNOT_ENCODE_VALUE(FATAL, Hash, previous_hash);
        return false;
      }

      //TODO: encode IndexedTransactionSet
      return true;
    }

    bool BlockDecoder::Decode(const BufferPtr &buff, Block &result) const{
      Timestamp timestamp = buff->GetTimestamp();
      DLOG(INFO) << "decoded Block timestamp: " << FormatTimestampReadable(timestamp);

      int64_t height = buff->GetLong();
      DLOG(INFO) << "decoded Block height: " << height;

      Hash previous_hash = buff->GetHash();
      DLOG(INFO) << "decoded Block previous_hash: " << previous_hash;

      Version version = Version::CurrentVersion();

      IndexedTransactionSet transactions = {};

      int64_t ntransactions = buff->GetLong();
      DLOG(INFO) << "decoding " << ntransactions << " transactions....";
      for(auto idx = 0; idx < ntransactions; idx++){
        auto itx = IndexedTransaction::DecodeNew(buff, hints());
        DLOG(INFO) << "decoded IndexedTransaction #" << idx << ": " << itx->ToString();
        transactions.insert(itx);
      }

      result = Block(timestamp, height, previous_hash, transactions, version);
      return true;
    }
  }
}