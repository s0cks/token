#include "block.h"

namespace token{
  static inline IndexedTransactionPtr
  CreateGenesisTransaction(const uint64_t& index, const User& user, const Product& product, const uint64_t& nouts){
    InputList inputs = {};
    // do we need inputs?

    OutputList outputs = {};
    for(auto idx = 0; idx < nouts; idx++)
      outputs << Output(user, product);

    auto timestamp = FromUnixTimestamp(0);
    return IndexedTransaction::NewInstance(index, timestamp, inputs, outputs);
  }

  BlockPtr Block::NewGenesis(){
    auto height = static_cast<uint64_t>(0);
    auto previous = Hash();
    auto timestamp = FromUnixTimestamp(0);
    IndexedTransactionSet transactions;
    transactions << CreateGenesisTransaction(0, User("VenueA"), Product("TestToken"), Block::kNumberOfGenesisOutputs);
    transactions << CreateGenesisTransaction(1, User("VenueB"), Product("TestToken"), Block::kNumberOfGenesisOutputs);
    transactions << CreateGenesisTransaction(2, User("VenueC"), Product("TestToken"), Block::kNumberOfGenesisOutputs);
    return Block::NewInstance(height, previous, timestamp, transactions);
  }

  Hash Block::GetMerkleRoot() const{
    NOT_IMPLEMENTED(FATAL);//TODO: implement
    return Hash();
  }

  internal::BufferPtr Block::ToBuffer() const{
    RawBlock raw;
    raw << (*this);
    auto data = internal::NewBufferForProto(raw);
    if(!data->PutMessage(raw)){
      LOG(FATAL) << "cannot serialize " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
      return data;
    }
    DVLOG(2) << "serialized " << ToString() << " (" << PrettySize(raw.ByteSizeLong()) << ") into " << data->ToString() << ".";
    return data;
  }
}