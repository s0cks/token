#ifndef TOKEN_JSON_CONVERSION_H
#define TOKEN_JSON_CONVERSION_H

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include "hash.h"
#include "block.h"

namespace Token{
  typedef rapidjson::Document JsonDocument;
  typedef rapidjson::StringBuffer JsonString;
  typedef rapidjson::Writer<JsonString> JsonWriter;

  static inline void
  SetFieldNull(JsonWriter& writer, const std::string& name){
    writer.Key(name.data(), name.length());
    writer.Null();
  }

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

  static inline void
  SetField(JsonWriter& writer, const std::string& name, const TransactionReference& ref){
    writer.Key(name.data(), name.length());
    writer.StartObject();
      SetField(writer, "Hash", ref.GetTransactionHash());
      SetField(writer, "OutputIndex", ref.GetIndex());
    writer.EndObject();
  }

  void SetField(JsonWriter& writer, const std::string& name, const BlockPtr& val);
  void SetField(JsonWriter& writer, const std::string& name, const TransactionPtr& val);
  void SetField(JsonWriter& writer, const std::string& name, const UnclaimedTransactionPtr& val);
  void SetField(JsonWriter& writer, const std::string& name, const HashList& val);
}

#endif //TOKEN_JSON_CONVERSION_H