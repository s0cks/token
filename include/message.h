#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include <vector>

#include "codec.h"
#include "object.h"
#include "decoder.h"
#include "encoder.h"

namespace token{
  class MessageBase;
  typedef std::shared_ptr<MessageBase> MessagePtr;
  typedef std::vector<MessagePtr> MessageList;

  static inline MessageList&
  operator<<(MessageList& messages, const MessagePtr& msg){
    messages.push_back(msg);
    return messages;
  }

  class MessageBase : public Object{
   public:
    template<class M>
    class MessageEncoder : public codec::EncoderBase<M>{
     protected:
      MessageEncoder(const M& value, const codec::EncoderFlags& flags):
        codec::EncoderBase<M>(value, flags){}
     public:
      MessageEncoder(const MessageEncoder<M>& other) = default;
      ~MessageEncoder<M>() override = default;
      MessageEncoder<M>& operator=(const MessageEncoder<M>& other) = default;
    };

    template<class M>
    class MessageDecoder : public codec::DecoderBase<M>{
     protected:
      explicit MessageDecoder(const codec::DecoderHints& hints):
        codec::DecoderBase<M>(hints){}
     public:
      MessageDecoder(const MessageDecoder<M>& other) = default;
      ~MessageDecoder() override = default;
      MessageDecoder<M>& operator=(const MessageDecoder<M>& other) = default;
    };
   protected:
    MessageBase() = default;
   public:
    ~MessageBase() override = default;
    virtual int64_t GetBufferSize() const = 0;
    virtual bool Write(const BufferPtr& buff) const = 0;
  };
}

#endif //TOKEN_MESSAGE_H