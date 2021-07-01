#include "json.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

namespace token{
  namespace json{

    static inline bool
    Append(Writer& writer, const Hash& hash){
      std::string hex = hash.HexString();
      return writer.String(hex.data(), hex.length());
    }

    static inline bool
    Append(Writer& writer, const Input& val){
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "user", val.GetUser()))
          return false;
        if (!SetField(writer, "transaction", val.GetReference()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    static inline bool
    Append(Writer& writer, const Output& val){
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "product", val.GetProduct()))
          return false;
        if (!SetField(writer, "user", val.GetUser()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    template<class T>
      static inline bool
      SetArrayField(Writer& writer, const char* name, const std::vector<T>& val){
        KEY(ERROR, writer, name);
        START_ARRAY(ERROR, writer);
        for (auto& it : val){
          if (!Append(writer, it)){
            LOG(ERROR) << "cannot append " << it << " to json array.";
            return false;
          }
        }
        END_ARRAY(ERROR, writer);
        return true;
      }

    bool SetField(Writer& writer, const char* name, const TransactionReference& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "hash", val.transaction()))
          return false;
        if (!SetField(writer, "index", val.index()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "transaction", val->GetReference()))
          return false;

        if (!SetField(writer, "user", val->GetUser()))
          return false;

        if (!SetField(writer, "product", val->GetProduct()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const BlockPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "timestamp", val->timestamp()))
          return false;
        if (!SetField(writer, "version", val->GetVersion()))
          return false;
        if (!SetField(writer, "height", val->height()))
          return false;
        if (!SetField(writer, "previous_hash", val->GetPreviousHash()))
          return false;
        if (!SetField(writer, "hash", val->hash()))
          return false;
        if (!SetField(writer, "transactions", val->GetNumberOfTransactions()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const TransactionPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if (!SetField(writer, "timestamp", val->timestamp()))
          return false;
        if (!json::SetField(writer, "inputs", val->inputs()))
          return false;
        if (!json::SetField(writer, "outputs", val->outputs()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const HashList& val){
      return SetArrayField(writer, name, val);
    }

    bool SetField(Writer& writer, const char* name, const InputList& val){
      return SetArrayField(writer, name, val);
    }

    bool SetField(Writer& writer, const char* name, const OutputList& val){
      return SetArrayField(writer, name, val);
    }
  }
}