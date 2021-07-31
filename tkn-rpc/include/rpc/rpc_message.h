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
    public:
      ~Message() override = default;
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
      explicit RawMessage(const BufferPtr& data):
        RawMessage(){
        if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
          DLOG(FATAL) << "cannot parse from buffer.";
      }

      inline T&
      raw(){
        return raw_;
      }
    public:
      ~RawMessage() override = default;

      uint64_t GetBufferSize() const override{
        return raw_.ByteSizeLong();
      }

      internal::BufferPtr ToBuffer() const override{
        auto length = raw_.ByteSizeLong();
        auto data = internal::NewBuffer(length);
        if(!raw_.SerializeToArray(data->data(), static_cast<int>(length))){
          DLOG(FATAL) << "cannot serialize type to buffer.";
          return nullptr;
        }
        return data;
      }
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
}

#endif//TOKEN_RPC_MESSAGE_H