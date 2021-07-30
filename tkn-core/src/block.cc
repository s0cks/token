#include "block.h"

namespace token{
  std::string Block::ToString() const{
    std::stringstream stream;
    stream << "Block(#" << height() << ", " << GetNumberOfTransactions() << " Transactions)";
    return stream.str();
  }

  BlockPtr Block::Genesis(){
    Block::Builder builder;
    builder.SetTimestamp(FromUnixTimestamp(0));
    builder.SetPreviousHash(Hash());
    builder.SetHeight(0);

    IndexedTransaction::Builder tx1 = builder.AddTransaction();
    tx1.SetIndex(0);
    for(auto idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++)
      tx1.AddOutput("VenueA", "TestToken");

    IndexedTransaction::Builder tx2 = builder.AddTransaction();
    tx2.SetIndex(1);
    for(auto idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++)
      tx2.AddOutput("VenueB", "TestToken");

    IndexedTransaction::Builder tx3 = builder.AddTransaction();
    tx3.SetIndex(3);
    for(auto idx = 0; idx < Block::kNumberOfGenesisOutputs; idx++)
      tx3.AddOutput("VenueC", "TestToken");

    return builder.Build();
  }

  bool Block::Accept(BlockVisitor* vis) const{
    NOT_IMPLEMENTED(ERROR);
    return false;//TODO: implement
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
    auto length = raw_.ByteSizeLong();
    auto data = internal::NewBuffer(length);
    if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
      DLOG(FATAL) << "cannot serialize Block to buffer.";
      return nullptr;
    }
    return data;
  }
}