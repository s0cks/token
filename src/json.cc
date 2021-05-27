#include "json.h"
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
  }
}