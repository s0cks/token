#include "json.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

#include "http/http_common.h"
#include "elastic/events/elastic_spend_event.h"

namespace token{
  namespace json{

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

    static inline bool
    Append(Writer& writer, const Hash& hash){
      std::string hex = hash.HexString();
      return writer.String(hex.data(), hex.length());
    }

    static inline bool
    Append(Writer& writer, const Input& val){
      START_OBJECT(ERROR, writer);
      {
        if(!SetField(writer, "user", val.GetUser()))
          return false;
        if(!SetField(writer, "transaction", val.GetReference()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    static inline bool
    Append(Writer& writer, const Output& val){
      START_OBJECT(ERROR, writer);
      {
        if(!SetField(writer, "product", val.GetProduct()))
          return false;
        if(!SetField(writer, "user", val.GetUser()))
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
      for(auto& it : val){
        if(!Append(writer, it)){
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
        if(!SetField(writer, "hash", val.GetTransactionHash()))
          return false;
        if(!SetField(writer, "index", val.GetIndex()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const Wallet& val){
      KEY(ERROR, writer, name);
      START_ARRAY(ERROR, writer);
      for(auto& it : val){
        if(!Append(writer, it)){
          LOG(ERROR) << "cannot append " << it << " to json array.";
          return false;
        }
      }
      END_ARRAY(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if(!SetField(writer, "transaction", val->GetReference()))
          return false;

        if(!SetField(writer, "user", val->GetUser()))
          return false;

        if(!SetField(writer, "product", val->GetProduct()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const BlockPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if(!SetField(writer, "timestamp", val->GetTimestamp()))
          return false;
        if(!SetField(writer, "version", val->GetVersion()))
          return false;
        if(!SetField(writer, "height", val->GetHeight()))
          return false;
        if(!SetField(writer, "previous_hash", val->GetPreviousHash()))
          return false;
        if(!SetField(writer, "hash", val->GetHash()))
          return false;
        if(!SetField(writer, "transactions", val->GetNumberOfTransactions()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const TransactionPtr& val){
      KEY(ERROR, writer, name);
      START_OBJECT(ERROR, writer);
      {
        if(!SetField(writer, "timestamp", val->GetTimestamp()))
          return false;
        if(!json::SetField(writer, "inputs", val->inputs()))
          return false;
        if(!json::SetField(writer, "outputs", val->outputs()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool SetField(Writer& writer, const char* name, const http::ErrorMessage& val){
      KEY(ERROR, writer, name);

      //TODO: refactor
      LOG_IF(WARNING, !val.ToJson(writer)) << "cannot serialize http::ErrorMessage to json.";
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

#define ELASTIC_EVENT_FIELDS(LevelName, Writer, Event) \
  if(!SetField(writer, "@timestamp", (Event).GetTimestamp())) \
    return false;                                      \
  KEY(LevelName, Writer, "event");                     \
  START_OBJECT(LevelName, Writer);                     \
  {                                                    \
    if(!SetField(Writer, "kind", (Event).GetKind()))   \
      return false;                                    \
    if(!SetField(Writer, "dataset", (Event).GetDataset()))    \
      return false;                                    \
  }                                                    \
  END_OBJECT(LevelName, Writer);


    bool ToJson(String& doc, const elastic::Event& val){
      Writer writer(doc);
      START_OBJECT(ERROR, writer);
      {
        ELASTIC_EVENT_FIELDS(ERROR, writer, val);
      }
      END_OBJECT(ERROR, writer);
      return true;
    }

    bool ToJson(String& doc, const elastic::SpendEvent& val){
      Writer writer(doc);
      START_OBJECT(ERROR, writer);
      {
        ELASTIC_EVENT_FIELDS(ERROR, writer, val);

        if(!SetField(writer, "token", val.token()))
          return false;

        if(!SetField(writer, "owner", val.owner()))
          return false;

        if(!SetField(writer, "recipient", val.recipient()))
          return false;
      }
      END_OBJECT(ERROR, writer);
      return true;
    }
  }
}