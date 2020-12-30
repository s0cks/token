#include "utils/json_conversion.h"

namespace Token{
    typedef rapidjson::Writer<JsonString> JsonWriter;

    static inline void
    SetField(JsonWriter& writer, const std::string& name, const int64_t& value){
        writer.Key(name.data(), name.length());
        writer.Int64(value);
    }

    static inline void
    SetField(JsonWriter& writer, const std::string& name, const std::string& value){
        writer.Key(name.data(), name.length());
        writer.String(value.data(), value.length());
    }

    static inline void
    SetField(JsonWriter& writer, const std::string& name, const Hash& hash){
        SetField(writer, name, hash.HexString());
    }

    static inline void
    SetField(JsonWriter& writer, const std::string& name, const User& user){
        SetField(writer, name, user.str());
    }

    static inline void
    SetField(JsonWriter& writer, const std::string& name, const Product& product){
        SetField(writer, name, product.str());
    }

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