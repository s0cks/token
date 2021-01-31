#include "block.h"
#include "server.h"
#include "blockchain.h"

namespace Token{
  BlockHeader::BlockHeader(const BufferPtr& buff):
    timestamp_(FromUnixTimestamp(buff->GetLong())),
    height_(buff->GetLong()),
    previous_hash_(buff->GetHash()),
    merkle_root_(buff->GetHash()),
    hash_(buff->GetHash()),
    bloom_(),
    num_transactions_(buff->GetLong()){}

  BlockPtr BlockHeader::GetData() const{
    return BlockChain::GetBlock(GetHash());
  }

  bool BlockHeader::Write(const BufferPtr& buff) const{
    buff->PutLong(ToUnixTimestamp(timestamp_));
    buff->PutLong(height_);
    buff->PutHash(previous_hash_);
    buff->PutHash(merkle_root_);
    buff->PutHash(hash_);
    buff->PutLong(num_transactions_);
    return true;
  }

//######################################################################################################################
//                                          Block
//######################################################################################################################
  BlockPtr Block::Genesis(){
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

    TransactionSet transactions;
    transactions.insert(Transaction::NewInstance(1, inputs, outputs_a, FromUnixTimestamp(0)));
    transactions.insert(Transaction::NewInstance(2, inputs, outputs_b, FromUnixTimestamp(0)));
    transactions.insert(Transaction::NewInstance(3, inputs, outputs_c, FromUnixTimestamp(0)));
    return std::make_shared<Block>(0, Hash(), transactions, FromUnixTimestamp(0));
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

  BlockPtr Block::FromFile(const std::string& filename){
    BlockFileReader reader(filename);
    return reader.ReadBlock();
  }
}