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

      std::string ToString() const{
        json::String doc;
        json::Writer writer(doc);
        if(!json::ToJson(doc, (*this)))
          return "{}";
        return std::string(doc.GetString(), doc.GetSize());
      }

      SpendEvent& operator=(const SpendEvent& other) = default;

      friend std::ostream&
      operator<<(std::ostream& stream, const SpendEvent& event){
        return stream << event.ToString();
      }
    };
  }
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_SPEND_H