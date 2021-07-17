#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include <vector>

#include "message.h"
#include "codec/codec.h"

namespace token{
  namespace rpc{
    class Message;
    typedef std::shared_ptr<Message> MessagePtr;

    typedef std::vector<MessagePtr> MessageList;

    class Session;

    class Message: public MessageBase{
    protected:
      Message() = default;

      static inline codec::EncoderFlags
      GetDefaultMessageEncoderFlags(){
        return codec::EncodeTypeFlag::Encode(true)|codec::EncodeVersionFlag::Encode(true);
      }

      static inline codec::DecoderHints
      GetDefaultMessageDecoderHints(){
        return codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true);
      }
    public:
      ~Message() override = default;
      static MessagePtr From(const BufferPtr& buffer);
    };

    class MessageParser{
    protected:
      BufferPtr data_;
    public:
      explicit MessageParser(const BufferPtr& data):
        data_(data){}
      ~MessageParser() = default;

      bool HasNext() const{
        return data_->GetReadPosition() < data_->GetWritePosition();
      }

      MessagePtr Next() const;
    };

    class MessageHandler{
    protected:
      rpc::Session* session_;

      explicit MessageHandler(rpc::Session* session):
        session_(session){}
    public:
      virtual ~MessageHandler() = default;

      rpc::Session* GetSession() const{
        return session_;
      }

      void Send(const rpc::MessagePtr& msg) const;
      void Send(const rpc::MessageList& msgs) const;

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void On##Name##Message(const Name##MessagePtr& message) = 0; //TODO: change to const?

      FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    };
  }

  namespace codec{
    template<class M>
    class MessageEncoder : public EncoderBase<M>{
    protected:
      MessageEncoder(const M& value, const codec::EncoderFlags& flags):
        EncoderBase<M>(value, flags){}
    public:
      MessageEncoder(const MessageEncoder<M>& rhs) = default;
      ~MessageEncoder() override = default;
      MessageEncoder<M>& operator=(const MessageEncoder<M>& rhs) = default;
    };

    template<class M>
    class MessageDecoder : public DecoderBase<M>{
    protected:
      explicit MessageDecoder(const codec::DecoderHints& hints):
        DecoderBase<M>(hints){}
    public:
      MessageDecoder(const MessageDecoder<M>& rhs) = default;
      ~MessageDecoder() override = default;
      MessageDecoder<M>& operator=(const MessageDecoder<M>& rhs) = default;
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_H