#ifndef TOKEN_JSON_H
#define TOKEN_JSON_H

#include <vector>
#include <unordered_set>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "uuid.h"
#include "hash.h"
#include "address.h"
#include "version.h"
#include "timestamp.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  namespace http{
    class ErrorMessage;
  }

  namespace elastic{
    class Event;
    class SpendEvent;
  }

  namespace json{
    static inline bool
    SetFieldNull(Writer& writer, const char*  name){
      return writer.Key(name, strlen(name))
          && writer.Null();
    }

    static inline bool
    SetField(Writer& writer, const char* name, const int8_t& val){
      return writer.Key(name, strlen(name))
          && writer.Int(static_cast<int32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const uint8_t& val){
      return writer.Key(name, strlen(name))
          && writer.Uint(static_cast<uint32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const int16_t& val){
      return writer.Key(name, strlen(name))
          && writer.Int(static_cast<int32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const uint16_t& val){
      return writer.Key(name, strlen(name))
          && writer.Uint(static_cast<uint32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const int32_t& val){
      return writer.Key(name, strlen(name))
          && writer.Int(static_cast<int32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const uint32_t& val){
      return writer.Key(name, strlen(name))
          && writer.Uint(static_cast<uint32_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const int64_t& val){
      return writer.Key(name, strlen(name))
          && writer.Int64(static_cast<int64_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const uint64_t& val){
      return writer.Key(name, strlen(name))
          && writer.Uint64(static_cast<uint64_t>(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Timestamp& val){
      return SetField(writer, name, ToUnixTimestamp(val));
    }

    static inline bool
    SetField(Writer& writer, const char* name, const std::string& val){
      return writer.Key(name, strlen(name))
          && writer.String(val.data(), val.length());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Hash& val){
      return SetField(writer, name, val.HexString());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const User& val){
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Product& val){
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Version& val){
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UUID& val){
      return SetField(writer, name, val.str());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const NodeAddress& val){
      return SetField(writer, name, val.ToString());
    }

    bool SetField(Writer& writer, const char* name, const TransactionReference& val);
    bool SetField(Writer& writer, const char* name, const Wallet& val);
    bool SetField(Writer& writer, const char* name, const HashList& val);
    bool SetField(Writer& writer, const char* name, const BlockPtr& val);
    bool SetField(Writer& writer, const char* name, const TransactionPtr& val);
    bool SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val);
    bool SetField(Writer& writer, const char* name, const http::ErrorMessage& val);
    bool SetField(Writer& writer, const char* name, const InputList& val);
    bool SetField(Writer& writer, const char* name, const OutputList& val);

    bool ToJson(String& doc, const elastic::Event& val);
    bool ToJson(String& doc, const elastic::SpendEvent& val);
  }

  namespace Json{
    template<typename T>
    static inline bool
    SetField(Writer& writer, const char* name, const std::vector<T>& cont){
      if(!writer.Key(name, strlen(name)))
        return false;
      if(!writer.StartArray())
        return false;
      for(auto& it : cont)
        if(!it.Write(writer))
          return false;
      return writer.EndArray();
    }
  }
}

#endif//TOKEN_JSON_H