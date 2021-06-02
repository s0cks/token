#include "json.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

#include "http/http_common.h"

namespace token{
  namespace json{
    static inline bool
    Append(Writer& writer, const Hash& hash){
      std::string hex = hash.HexString();
      return writer.String(hex.data(), hex.length());
    }

    bool SetField(Writer& writer, const std::string& name, const HashList& val){
      LOG_IF(ERROR, !writer.Key(name.data(), name.length())) << "cannot set field name '" << name << "' for json array.";
      LOG_IF(ERROR, !writer.StartArray()) << "cannot start json array.";
      for(auto& it : val)
        LOG_IF(ERROR, !Append(writer, it)) << "cannot append hash " << it << " to json array.";
      LOG_IF(ERROR, !writer.EndArray(val.size())) << "cannot end json array.";
      return true;
    }

    bool SetField(Writer& writer, const std::string& name, const Wallet& val){
      LOG_IF(ERROR, !writer.Key(name.data(), name.length())) << "cannot set field name '" << name << "' for json array.";
      LOG_IF(ERROR, !writer.StartArray()) << "cannot start json array";
      for(auto& it : val)
        LOG_IF(ERROR, !Append(writer, it)) << "cannot append hash " << it << " to json array.";
      LOG_IF(ERROR, !writer.EndArray(val.size())) << "cannot end json array.";
      return true;
    }

    bool SetField(Writer& writer, const std::string& name, const UnclaimedTransactionPtr& val){
      if(!writer.Key(name.data(), name.length()))
        return false;
      if(!val->Write(writer))
        return false;
      return true;
    }

    bool SetField(Writer& writer, const std::string& name, const BlockPtr& val){
      if(!writer.Key(name.data(), name.length()))
        return false;
      if(!val->Write(writer))
        return false;
      return true;
    }

    bool SetField(Writer& writer, const std::string& name, const TransactionPtr& val){
      if(!writer.Key(name.data(), name.length()))
        return false;
      if(!val->Write(writer))
        return false;
      return true;
    }

    bool SetField(Writer& writer, const std::string& name, const http::ErrorMessage& val){
      LOG_IF(WARNING, !writer.Key(name.data(), name.length())) << "cannot set json key '" << name << "'.";
      LOG_IF(WARNING, !val.ToJson(writer)) << "cannot serialize http::ErrorMessage to json.";
      return true;
    }
  }
}