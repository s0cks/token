#ifndef TOKEN_JSON_H
#define TOKEN_JSON_H

#include <vector>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "uuid.h"
#include "hash.h"
#include "address.h"
#include "version.h"
#include "timestamp.h"

namespace token{
  namespace Json{
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
    SetField(Writer& writer, const std::string& name, const Timestamp& timestamp){
      return SetField(writer, name, ToUnixTimestamp(timestamp));
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

    static inline bool
    SetField(Writer& writer, const std::string& name, const User& val){
      return SetField(writer, name, val.str());
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const Version& val){
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const UUID& val){
      return SetField(writer, name, val.str());
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const NodeAddress& val){
      return SetField(writer, name, val.ToString());
    }

    static inline bool
    Append(Writer& writer, const Hash& hash){
      std::string hex = hash.HexString();
      return writer.String(hex.data(), hex.length());
    }

    template<typename T>
    static inline bool
    SetField(Writer& writer, const std::string& name, const std::vector<T>& cont){
      if(!writer.Key(name.data(), name.length()))
        return false;
      if(!writer.StartArray())
        return false;
      for(auto& it : cont)
        if(!it.Write(writer))
          return false;
      return writer.EndArray();
    }

    static inline bool
    SetField(Writer& writer, const std::string& name, const HashList& val){
      if(!writer.Key(name.data(), name.length()))
        return false;
      if(!writer.StartArray())
        return false;
      for(auto& it : val){
        std::string hex = it.HexString();
        if(!writer.String(hex.data(), hex.length()))
          return false;
      }
      return writer.EndArray();
    }

    template<class T, class Hasher, class Equals>
    static inline bool
    SetField(Writer& writer, const std::string& name, const std::unordered_set<T, Hasher, Equals>& cont){
      if(!writer.Key(name.data(), name.length()))
        return false;

      if(!writer.StartArray())
        return false;
      for(auto& it : cont){
        if(!Append(writer, it))
          return false;
      }
      return writer.EndArray();
    }

    static bool SetField(Writer& writer, const std::string& name, const UnclaimedTransactionPtr& val);
    static bool SetField(Writer& writer, const std::string& name, const BlockPtr& val);

    static bool ToJson(Writer& writer, const HashList& hashes);

    static inline bool
    FromJson(const std::string& document, HashList& result){
      rapidjson::Document doc;
      doc.Parse(document.data(), document.length());

      if(!doc.IsArray()){
        return false;
      }

      auto hashes = doc.GetArray();
      for(auto& v : hashes)
        result.push_back(Hash(v.GetString(), v.GetStringLength()));

      return true;
    }
  }
}

#endif//TOKEN_JSON_H