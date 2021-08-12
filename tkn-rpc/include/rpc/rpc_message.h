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
      explicit RawMessage(const BufferPtr& data, const uint64_t& msize):
        RawMessage(){
        DLOG(INFO) << "parsing message of size " << msize << " from buffer of size " << data->length() << " @" << data->GetReadPosition();
        if(!data->GetMessage(raw_, msize))
          DLOG(FATAL) << "cannot parse message from buffer of size: " << data->length();
      }


      uint64_t GetRawSize() const{
        return raw_.ByteSizeLong();
      }

      bool WriteHeader(const internal::BufferPtr& data) const{
        auto mtype = static_cast<uint64_t>(type());
        if(!data->PutUnsignedLong(mtype)){
          DLOG(ERROR) << "cannot serialize mtype to buffer.";
          return false;
        }
        ENCODED_FIELD(mtype_, Type, mtype);

        auto msize = GetRawSize();
        if(!data->PutUnsignedLong(msize)){
          DLOG(ERROR) << "cannot serialize msize to buffer.";
          return false;
        }
        ENCODED_FIELD(msize_, uint64_t, msize);
        return true;
      }

      static inline uint64_t
      GetHeaderSize(){
        uint64_t size = 0;
        size += sizeof(uint64_t);//mtype
        size += sizeof(uint64_t);//msize
        return size;
      }
    public:
      ~RawMessage() override = default;

      uint64_t GetBufferSize() const override{
        return GetRawSize()+GetHeaderSize();
      }

      internal::BufferPtr ToBuffer() const override{
        auto data = internal::NewBuffer(GetBufferSize());
        if(!WriteHeader(data)){
          DLOG(FATAL) << "cannot serialize message header to buffer.";
          return nullptr;
        }

        if(!data->PutMessage(raw_)){
          DLOG(FATAL) << "cannot serialize type to buffer.";
          return nullptr;
        }
        return data;
      }
    };

    class MessageParser : codec::DecoderBase{
    protected:
      BufferPtr& data_;

      static inline uint64_t
      GetHeaderSize(){
        uint64_t size = 0;
        size += sizeof(uint64_t);
        size += sizeof(uint64_t);
        return size;
      }

      inline bool
      HasMoreBytes() const{
        return (data_->GetReadPosition()+GetHeaderSize()) < data_->length();
      }
    public:
      explicit MessageParser(BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
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