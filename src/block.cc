#include "block.h"
#include "server.h"
#include "blockchain.h"

namespace token{
  BufferPtr Block::ToBuffer() const{
    Encoder encoder((*this));
    BufferPtr buffer = Buffer::AllocateFor(encoder);
    if(!encoder.Encode(buffer)){
      DLOG(ERROR) << "cannot encode Block.";
      buffer->clear();
    }
    return buffer;
  }

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
    MerkleTreeBuilder builder;
    if(!Accept(&builder)){
      return Hash();
    }
    std::shared_ptr<MerkleTree> tree = builder.Build();
    return tree->GetRootHash();
  }

  int64_t Block::Encoder::GetBufferSize() const{
    int64_t size = codec::EncoderBase<Block>::GetBufferSize();
    size += sizeof(RawTimestamp); // timestamp_
    size += sizeof(int64_t); // height_
    size += Hash::GetSize(); // previous_hash_
    size += Hash::GetSize(); // merkle_root_
    size += codec::EncoderBase<Block>::GetBufferSize<IndexedTransaction, IndexedTransaction::IndexComparator, IndexedTransaction::Encoder>(value().transactions());
    return size;
  }

  bool Block::Encoder::Encode(const BufferPtr& buff) const{
    if(ShouldEncodeVersion()){
      if(!buff->PutVersion(codec::GetCurrentVersion())){
        LOG(FATAL) << "cannot encode codec version.";
        return false;
      }
      DLOG(INFO) << "encoded codec version.";
    }

    const Timestamp& timestamp = value().timestamp();
    if(!buff->PutTimestamp(timestamp)){
      CANNOT_ENCODE_VALUE(FATAL, Timestamp, ToUnixTimestamp(timestamp));
      return false;
    }

    const int64_t& height = value().height();
    if(!buff->PutLong(height)){
      CANNOT_ENCODE_VALUE(FATAL, int64_t, height);
      return false;
    }

    const Hash& previous_hash = value().GetPreviousHash();
    if(!buff->PutHash(previous_hash)){
      CANNOT_ENCODE_VALUE(FATAL, Hash, previous_hash);
      return false;
    }

    const IndexedTransactionSet& transactions = value().transactions();
    if(!buff->PutLong(static_cast<int64_t>(transactions.size()))){
      CANNOT_ENCODE_VALUE(FATAL, transactions, transactions.size());
      return false;
    }

    for(auto& it : transactions){
      IndexedTransaction::Encoder encoder(*it, flags());
      if(!encoder.Encode(buff)){
        LOG(FATAL) << "cannot encode transactions (IndexedTransactionSet)";
        return false;
      }
    }

    return true;
  }

  bool Block::Decoder::Decode(const BufferPtr& buff, Block& result) const{
    DLOG(INFO) << "decoding block....";
    CHECK_CODEC_VERSION(FATAL, buff);

    Timestamp timestamp = buff->GetTimestamp();
    DLOG(INFO) << "decoded Block timestamp: " << FormatTimestampReadable(timestamp);

    int64_t height = buff->GetLong();
    DLOG(INFO) << "decoded Block height: " << height;

    Hash previous_hash = buff->GetHash();
    DLOG(INFO) << "decoded Block previous_hash: " << previous_hash;

    Version version = codec::GetCurrentVersion();//TODO: fixme

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