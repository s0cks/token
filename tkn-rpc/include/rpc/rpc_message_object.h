#ifndef TOKEN_RPC_MESSAGE_OBJECT_H
#define TOKEN_RPC_MESSAGE_OBJECT_H

#include "rpc/rpc_message.h"

namespace token{
  namespace codec{
    template<class M, class E>
    class ObjectMessageEncoder : public MessageEncoder<M>{
    protected:
      E encode_value_;

      ObjectMessageEncoder(const M* value, const codec::EncoderFlags& flags):
        MessageEncoder<M>(value, flags),
        encode_value_(value->value().get(), flags){}
    public:
      ObjectMessageEncoder(const ObjectMessageEncoder& other) = default;
      ~ObjectMessageEncoder() override = default;

      int64_t GetBufferSize() const override{
        auto size = MessageEncoder<M>::GetBufferSize();
        size += encode_value_.GetBufferSize();
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(!MessageEncoder<M>::Encode(buff))
          return false;
        if(!encode_value_.Encode(buff)){
          LOG(FATAL) << "cannot encode object value.";
          return false;
        }
        return true;
      }

      ObjectMessageEncoder& operator=(const ObjectMessageEncoder& other) = default;
    };

    template<class M, class D>
    class ObjectMessageDecoder : public MessageDecoder<M>{
    protected:
      D decode_value_;

      explicit ObjectMessageDecoder(const codec::DecoderHints& hints):
        MessageDecoder<M>(hints),
        decode_value_(hints){}
    public:
      ~ObjectMessageDecoder() override = default;
      M* Decode(const BufferPtr& data) const override = 0;
    };
  }

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