#include "utils/json_conversion.h"

namespace Token{
  void ToJson(const HashList& hashes, JsonString& sb){
    JsonWriter writer(sb);
    writer.StartArray();
    for(auto& it : hashes){
      std::string hash = it.HexString();
      writer.String(hash.data(), Hash::GetSize());
    }
    writer.EndArray();
  }

  void ToJson(const BlockPtr& blk, JsonString& json){
    JsonWriter writer(json);
    writer.StartObject();
    SetField(writer, "Hash", blk->GetHash());
    SetField(writer, "Timestamp", blk->GetTimestamp());
    SetField(writer, "PreviousHash", blk->GetPreviousHash());
    SetField(writer, "Height", blk->GetHeight());
    SetField(writer, "MerkleRoot", blk->GetMerkleRoot());
    writer.EndObject();
  }

  void ToJson(const TransactionPtr& tx, JsonString& json){
    JsonWriter writer(json);
    writer.StartObject();
    SetField(writer, "Hash", tx->GetHash());
    SetField(writer, "Timestamp", tx->GetTimestamp());
    SetField(writer, "Index", tx->GetIndex());
    SetField(writer, "NumberOfInputs", tx->GetNumberOfInputs());
    SetField(writer, "NumberOfOutputs", tx->GetNumberOfOutputs());
    writer.EndObject();
  }

  void ToJson(const UnclaimedTransactionPtr& utxo, JsonString& json){
    JsonWriter writer(json);
    writer.StartObject();
    SetField(writer, "Hash", utxo->GetHash());
    SetField(writer, "TransactionHash", utxo->GetTransaction());
    SetField(writer, "OutputIndex", utxo->GetIndex());
    SetField(writer, "User", utxo->GetUser());
    SetField(writer, "Product", utxo->GetProduct());
    writer.EndObject();
  }
}