#ifndef TOKEN_RPC_MESSAGE_OBJECT_H
#define TOKEN_RPC_MESSAGE_OBJECT_H

#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    template<class T>
    class ObjectMessage : public Message{
     protected:
      std::shared_ptr<T> value_;

      ObjectMessage():
        Message(),
        value_(){}
      explicit ObjectMessage(const std::shared_ptr<T>& value):
        Message(),
        value_(value){}
     public:
      ObjectMessage(const ObjectMessage<T>& other) = default;
      ~ObjectMessage<T>() override = default;

      std::shared_ptr<T>& value(){
        return value_;
      }

      std::shared_ptr<T> value() const{
        return value_;
      }

      ObjectMessage<T>& operator=(const ObjectMessage<T>& other) = default;
    };
  }
}

#undef DEFINE_RPC_OBJECT_MESSAGE_TYPE

#endif//TOKEN_RPC_MESSAGE_OBJECT_H