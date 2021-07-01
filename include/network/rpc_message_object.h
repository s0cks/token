#ifndef TOKEN_RPC_MESSAGE_OBJECT_H
#define TOKEN_RPC_MESSAGE_OBJECT_H

#include "network/rpc_message.h"

namespace token{
  namespace rpc{
    template<class T>
    class ObjectMessage : public Message{
     public:
      template<class M>
      class ObjectMessageEncoder : public MessageEncoder<M>{
       protected:
        ObjectMessageEncoder(const M& value, const codec::EncoderFlags& flags):
          MessageEncoder<M>(value, flags){}
       public:
        ObjectMessageEncoder(const ObjectMessageEncoder<M>& other) = default;
        virtual ~ObjectMessageEncoder<M>() override = default;
        virtual int64_t GetBufferSize() const override = 0;
        virtual bool Encode(const BufferPtr& buff) const override = 0;
        ObjectMessageEncoder<M>& operator=(const ObjectMessageEncoder<M>& other) = default;
      };

      template<class M>
      class ObjectMessageDecoder : public MessageDecoder<M>{
       protected:
        ObjectMessageDecoder(const codec::DecoderHints& hints):
          MessageDecoder<M>(hints){}
       public:
        ObjectMessageDecoder(const ObjectMessageDecoder<M>& other) = default;
        virtual ~ObjectMessageDecoder<M>() override = default;
        virtual bool Decode(const BufferPtr& buff, M& result) const override = 0;
        ObjectMessageDecoder<M>& operator=(const ObjectMessageDecoder<M>& other) = default;
      };
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
      virtual ~ObjectMessage<T>() = default;

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