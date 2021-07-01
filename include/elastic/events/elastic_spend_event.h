#ifndef TOKEN_ELASTIC_EVENT_SPEND_H
#define TOKEN_ELASTIC_EVENT_SPEND_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "user.h"
#include "elastic/elastic_event.h"

namespace token{
#define TOKEN_ES_SPEND_EVENT_KIND "event"
#define TOKEN_ES_SPEND_EVENT_DATASET "spend"

  namespace elastic{
    class SpendEvent : public elastic::Event{
     private:
      const User owner_;
      const User recipient_;
      UnclaimedTransactionPtr token_;
     public:
      SpendEvent(const Timestamp& timestamp,
                 const User& owner,
                 const User& recipient,
                 UnclaimedTransactionPtr token):
        elastic::Event(timestamp, TOKEN_ES_SPEND_EVENT_KIND, TOKEN_ES_SPEND_EVENT_DATASET),
        owner_(owner),
        recipient_(recipient),
        token_(std::move(token)){}
      SpendEvent(const SpendEvent& other) = default;
      ~SpendEvent() override = default;

      User owner() const{
        return owner_;
      }

      User recipient() const{
        return recipient_;
      }

      UnclaimedTransactionPtr token() const{
        return token_;
      }

      SpendEvent& operator=(const SpendEvent& other) = default;

      friend std::ostream&
      operator<<(std::ostream& stream, const SpendEvent& event){
        stream << "SpendEvent(";
        stream << "event=" << event.event_class() << ", ";
        stream << "timestamp=" << ToUnixTimestamp(event.timestamp()) << ", ";
        stream << "owner=" << event.owner() << ", ";
        stream << "recipient=" << event.recipient() << ", ";
        stream << "token=" << event.token()->hash();//TODO: refactor?
        stream << ")";
        return stream;
      }

      friend json::Writer&
      operator<<(json::Writer& writer, const SpendEvent& val){
        if(!writer.StartObject()){
          DLOG(ERROR) << "cannot start SpendEvent json object.";
          return writer;
        }

        if(!writer.Key("event")){
          DLOG(ERROR) << "cannot set SpendEvent event json object field.";
          return writer;
        }
        writer << val.event_class();

        if(!json::SetField(writer, "timestamp", val.timestamp())){
          DLOG(ERROR) << "cannot set SpendEvent timestamp json object field.";
          return writer;
        }

        if(!json::SetField(writer, "owner", val.owner())){
          DLOG(ERROR) << "cannot set SpendEvent owner json object field.";
          return writer;
        }

        if(!json::SetField(writer, "recipient", val.recipient())){
          DLOG(ERROR) << "cannot set SpendEvent owner json object field.";
          return writer;
        }

        if(!json::SetField(writer, "token", val.token())){
          DLOG(ERROR) << "cannot set SpendEvent token json object field.";
          return writer;
        }

        DLOG_IF(ERROR, !writer.EndObject()) << "cannot end SpendEvent json object.";
        return writer;
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_SPEND_H