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

#define START_OBJECT(LevelName, Writer) \
    if(!(Writer).StartObject()){       \
      LOG(LevelName) << "cannot start json object."; \
      return false;                     \
    }
#define END_OBJECT(LevelName, Writer) \
    if(!(Writer).EndObject()){       \
      LOG(LevelName) << "cannot end json object."; \
      return false;                   \
    }

#define START_ARRAY(LevelName, Writer) \
    if(!(Writer).StartArray()){       \
      LOG(LevelName) << "cannot start json array."; \
      return false;                    \
    }
#define END_ARRAY(LevelName, Writer) \
    if(!(Writer).EndArray()){       \
      LOG(LevelName) << "cannot end json array."; \
      return false;                  \
    }

#define KEY(LevelName, Writer, Name) \
    if(!(Writer).Key((Name), strlen((Name)))){ \
      LOG(LevelName) << "cannot set field name '" << (Name) << "'."; \
      return false;                  \
    }

namespace token{
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
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    SetField(Writer& writer, const char* name, const NodeAddress& val){
      return SetField(writer, name, val.ToString());
    }

    bool SetField(Writer& writer, const char* name, const TransactionReference& val);
    bool SetField(Writer& writer, const char* name, const HashList& val);
    bool SetField(Writer& writer, const char* name, const BlockPtr& val);
    bool SetField(Writer& writer, const char* name, const TransactionPtr& val);
    bool SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val);
    bool SetField(Writer& writer, const char* name, const InputList& val);
    bool SetField(Writer& writer, const char* name, const OutputList& val);
  }
}

#endif//TOKEN_JSON_H