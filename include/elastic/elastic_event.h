#ifndef TOKEN_ELASTIC_EVENT_H
#define TOKEN_ELASTIC_EVENT_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "json.h"
#include "timestamp.h"

namespace token{
  namespace elastic{
    class Event{
     protected:
      Timestamp timestamp_;
      std::string kind_;
      std::string dataset_;
      std::string category_;
      std::string action_;
     public:
      Event(const Timestamp& timestamp,
            const std::string& kind,
            const std::string& dataset,
            const std::string& category,
            const std::string& action):
            timestamp_(timestamp),
            kind_(kind),
            dataset_(dataset),
            category_(category),
            action_(action){}
      virtual ~Event() = default;

      Timestamp GetTimestamp() const{
        return timestamp_;
      }

      std::string GetKind() const{
        return kind_;
      }

      std::string GetCategory() const{
        return category_;
      }

      std::string GetDataset() const{
        return dataset_;
      }

      std::string GetAction() const{
        return action_;
      }

      virtual bool Write(Json::Writer& writer) const = 0;

      virtual std::string ToString() const{
        std::stringstream ss;
        ss << "Event(";
          ss << "@timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
          ss << "kind=" << kind_ << ", ";
          ss << "category=" << category_ << ", ";
          ss << "dataset=" << dataset_ << ", ";
          ss << "action=" << action_;
        ss << ")";
        return ss.str();
      }
    };

    class SpendEvent : public Event{
     private:
      User owner_;
      User recipient_;
      Hash token_;
     public:
      SpendEvent(const Timestamp& timestamp,
                 const User& owner,
                 const User& recipient,
                 const Hash& token):
                 Event(timestamp, "event", "tokens", "ledger", "spend"),
                 owner_(owner),
                 recipient_(recipient),
                 token_(token){}
      ~SpendEvent() = default;

      User GetOwner() const{
        return owner_;
      }

      User GetRecipient() const{
        return recipient_;
      }

      Hash GetToken() const{
        return token_;
      }

      bool Write(Json::Writer& writer) const{
        writer.StartObject();
        {

          Json::SetField(writer, "@timestamp", timestamp_);

          writer.Key("event");
          writer.StartObject();
          {
            Json::SetField(writer, "kind", kind_);
            Json::SetField(writer, "dataset", dataset_);
            Json::SetField(writer, "category", category_);
            Json::SetField(writer, "action", action_);
          }
          writer.EndObject();

          writer.Key("owner");
          writer.StartObject();
          {
            Json::SetField(writer, "id", owner_);
          }
          writer.EndObject();

          writer.Key("recipient");
          writer.StartObject();
          {
            Json::SetField(writer, "id", recipient_);
          }
          writer.EndObject();

          Json::SetField(writer, "token", token_);
        }
        writer.EndObject();
        return true;
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_H