#include "json.h"

#include "block.h"
#include "unclaimed_transaction.h"

namespace token{
  namespace Json{
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

    bool ToJson(Writer& writer, const HashList& hashes){
      if(!writer.StartArray())
        return false;

      for(auto& it: hashes)
        Append(writer, it);

      if(!writer.EndArray())
        return false;
      return true;
    }
  }
}