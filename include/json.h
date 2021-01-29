#ifndef TOKEN_JSON_H
#define TOKEN_JSON_H

#include <vector>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "hash.h"

namespace Token{
  namespace Json{
    typedef rapidjson::Document Document;
    typedef rapidjson::StringBuffer String;
    typedef rapidjson::Writer<String> Writer;

    static inline bool
    SetFieldNull(Writer& writer, const std::string& name){
      return writer.Key(name.data(), name.length())
          && writer.Null();
   }

    static inline bool
    SetField(Writer& writer, const std::string& name, const int64_t& value){
      return writer.Key(name.data(), name.length())
          && writer.Int64(value);
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const std::string& value){
      return writer.Key(name.data(), name.length())
          && writer.String(value.data(), value.length());
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const Hash& val){
      return SetField(writer, name, val.HexString());
    }

    template<typename T>
    static inline bool
    SetField(Writer& writer, const std::string& name, const std::vector<T>& cont){
      if(!writer.StartArray())
        return false;
      for(auto& it : cont)
        if(!it.Write(writer))
          return false;
      return writer.EndArray();
    }

    static inline bool
    Append(Writer& writer, const Hash& hash){
      std::string hex = hash.HexString();
      return writer.String(hex.data(), hex.length());
    }
  }
}

#endif//TOKEN_JSON_H