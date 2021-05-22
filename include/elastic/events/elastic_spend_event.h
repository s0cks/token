#ifndef TOKEN_ELASTIC_EVENT_SPEND_H
#define TOKEN_ELASTIC_EVENT_SPEND_H

#ifdef TOKEN_ENABLE_EXPERIMENTAL

#include "elastic/elastic_event.h"

namespace token{
#define TOKEN_ES_SPEND_EVENT_KIND "event"
#define TOKEN_ES_SPEND_EVENT_DATASET "spend"

  class SpendEvent : public elastic::Event{
   private:
    const User owner_;
    const User recipient;
    UnclaimedTransactionPtr token_;
   public:
    SpendEvent(const Timestamp& timestamp,
               const User& owner,
               const User& recipient,
               UnclaimedTransactionPtr  token):
      elastic::Event(timestamp, TOKEN_ES_SPEND_EVENT_KIND, TOKEN_ES_SPEND_EVENT_DATASET),
      owner_(owner),
      recipient_(recipient),
      token_(std::move(token)){}
    SpendEvent(const SpendEvent& other):
      elastic::Event(other),
      owner_(other.owner_),
      recipient_(other.recipient),
      token_(other.token_){}
    ~SpendEvent() override = default;

    User owner() const{
      return owner_;


    User recipient() const{
      return recipient_;
    }

    UnclaimedTransactionPtr token() const{
        return token_;
    }

    bool Write(Json::Writer& writer) const override{
      if(!Event::Write(writer))
        return false;
      if(!Json::SetField(writer, "owner", owner()))
        return false;
      if(!Json::SetField(writer, "recipient", recipient()))
        return false;
      if(!Json::SetField(writer, "token", token()))
        return false;
      return true;
    }
  };
}

#endif//TOKEN_ENABLE_EXPERIMENTAL
#endif//TOKEN_ELASTIC_EVENT_SPEND_H