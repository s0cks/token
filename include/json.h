#ifndef TOKEN_JSON_H
#define TOKEN_JSON_H

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_set>

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "common.h"

#define JSON_START_OBJECT_AT_LEVEL(LevelName, Writer) \
  if(!(Writer).StartObject()){                        \
    LOG(LevelName) << "cannot write start of json object.";    \
    return false;                                     \
  }
#define JSON_START_OBJECT(Writer) \
  JSON_START_OBJECT_AT_LEVEL(FATAL, Writer)

#define JSON_END_OBJECT_AT_LEVEL(LevelName, Writer) \
  if(!(Writer).EndObject()){                        \
    LOG(LevelName) << "cannot write end of json object.";    \
    return false;                                   \
  }
#define JSON_END_OBJECT(Writer) \
  JSON_END_OBJECT_AT_LEVEL(FATAL, Writer)

#define JSON_START_ARRAY_AT_LEVEL(LevelName, Writer) \
  if(!(Writer).StartArray()){                        \
    LOG(LevelName) << "cannot write start of json array.";    \
    return false;                                    \
  }
#define JSON_START_ARRAY(Writer) \
  JSON_START_ARRAY_AT_LEVEL(FATAL, Writer)

#define JSON_END_ARRAY_AT_LEVEL(LevelName, Writer) \
  if(!(Writer).EndArray()){                        \
    LOG(LevelName) << "cannot write end of json array.";    \
    return false;                                  \
  }
#define JSON_END_ARRAY(Writer) \
  JSON_END_ARRAY_AT_LEVEL(FATAL, Writer)

#define JSON_KEY_AT_LEVEL(LevelName, Writer, Name) \
  if(!(Writer).Key((Name), strlen((Name)))){      \
    LOG(LevelName) << "cannot write json field key: " << (Name); \
    return false;                                  \
  }
#define JSON_KEY(Writer, Name) JSON_KEY_AT_LEVEL(FATAL, Writer, Name)

#define JSON_STRING_AT_LEVEL(LevelName, Writer, Value) \
  if(!(Writer).String((Value).data(), (Value).size())){       \
    LOG(LevelName) << "cannot write json string '" << (Value); \
    return false;                                      \
  }
#define JSON_STRING(Writer, Value) \
  JSON_STRING_AT_LEVEL(FATAL, Writer, Value)

#define JSON_LONG_AT_LEVEL(LevelName, Writer, Value) \
  if(!(Writer).Int64((Value))){ \
    LOG(LevelName) << "cannot write json long '" << (Value) << "'"; \
    return false;                                    \
  }
#define JSON_LONG(Writer, Value) \
  JSON_LONG_AT_LEVEL(FATAL, Writer, Value)

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
    SetField(Writer& writer, const char* name, const std::string& val){
      return writer.Key(name, strlen(name))
          && writer.String(val.data(), val.length());
    }
  }
}

#endif//TOKEN_JSON_H