#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include <vector>

#include "codec/codec.h"
#include "server/message.h"

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
        return codec::EncodeTypeFlag::Encode(true);
      }

      static inline codec::DecoderHints
      GetDefaultMessageDecoderHints(){
        return codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true);
      }
    public:
      ~Message() override = default;
      virtual Type type() const = 0;
    };

    template<class T>
    class RawMessage : public Message{
    protected:
      T raw_;

      RawMessage():
        Message(),
        raw_(){}
      explicit RawMessage(T raw):
        Message(),
        raw_(raw){}
    public:
      ~RawMessage() override = default;
    };

    class MessageParser : codec::DecoderBase{
    protected:
      BufferPtr data_;

      inline bool
      HasMoreBytes() const{
        static int64_t kHeaderSize = sizeof(uint64_t) + Version::GetSize();

        return (data_->GetReadPosition() + kHeaderSize) < data_->GetWritePosition();
      }
    public:
      explicit MessageParser(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase(hints),
        data_(data){}
      ~MessageParser() override = default;

      bool HasNext() const{
        return HasMoreBytes();//TODO: check for message header & magic constant
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
    class MessageEncoder : public TypeEncoder<M>{
    protected:
      MessageEncoder(const M* value, const codec::EncoderFlags& flags):
        TypeEncoder<M>(value, flags){}
    public:
      MessageEncoder(const MessageEncoder<M>& rhs) = default;
      ~MessageEncoder() override = default;
      MessageEncoder<M>& operator=(const MessageEncoder<M>& rhs) = default;
    };

    template<class M>
    class MessageDecoder : public TypeDecoder<M>{
    protected:
      explicit MessageDecoder(const codec::DecoderHints& hints):
        TypeDecoder<M>(hints){}
    public:
      ~MessageDecoder() override = default;
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_H