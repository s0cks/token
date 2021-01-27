#include "utils/json_conversion.h"

namespace Token{
  namespace Json{
    void SetField(Writer& writer, const std::string& name, const BlockPtr& val){
      writer.Key(name.data(), name.length());
      writer.StartObject();
      SetField(writer, "Hash", val->GetHash());
      SetField(writer, "Timestamp", val->GetTimestamp());
      SetField(writer, "PreviousHash", val->GetPreviousHash());
      SetField(writer, "Height", val->GetHeight());
      SetField(writer, "MerkleRoot", val->GetMerkleRoot());
      writer.EndObject();
    }

    void SetField(Writer& writer, const std::string& name, const TransactionPtr& val){
      writer.Key(name.data(), name.length());
      writer.StartObject();
      SetField(writer, "Hash", val->GetHash());
      SetField(writer, "Timestamp", val->GetTimestamp());
      SetField(writer, "Index", val->GetIndex());
      SetField(writer, "NumberOfInputs", val->GetNumberOfInputs());
      SetField(writer, "NumberOfOutputs", val->GetNumberOfOutputs());
      writer.EndObject();
    }

    void SetField(Writer& writer, const std::string& name, const UnclaimedTransactionPtr& val){
      writer.Key(name.data(), name.length());
      writer.StartObject();
      SetField(writer, "Hash", val->GetHash());
      SetField(writer, "Transaction", val->GetReference());
      SetField(writer, "User", val->GetUser());
      SetField(writer, "Product", val->GetProduct());
      writer.EndObject();
    }

    void SetField(Writer& writer, const std::string& name, const HashList& val){
      writer.Key(name.data(), name.length());
      writer.StartArray();
      for(auto& it : val){
        std::string hex = it.HexString();
        writer.String(hex.data(), hex.length());
      }
      writer.EndArray();
    }
  }
}