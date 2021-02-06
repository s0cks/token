#include "block.h"
#include "server.h"
#include "blockchain.h"

namespace token{
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

    TransactionSet transactions;
    transactions.insert(Transaction::NewInstance(1, inputs, outputs_a, FromUnixTimestamp(0)));
    transactions.insert(Transaction::NewInstance(2, inputs, outputs_b, FromUnixTimestamp(0)));
    transactions.insert(Transaction::NewInstance(3, inputs, outputs_c, FromUnixTimestamp(0)));
    return std::make_shared<Block>(0, version, Hash(), transactions, FromUnixTimestamp(0));
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
}