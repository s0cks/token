#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include "message.h"
#include "utils/buffer.h"

namespace token{
  template<class M>
  class Session;

  class RpcMessage;
  typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
  typedef std::vector<RpcMessagePtr> RpcMessageList;

#define DEFINE_MESSAGE(Name) \
  class Name##Message;       \
  typedef std::shared_ptr<Name##Message> Name##MessagePtr;
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE)
#undef DEFINE_MESSAGE

  class RpcMessage : public Message{
    friend class RpcSession;
   protected:
    RpcMessage() = default;
    virtual int64_t GetMessageSize() const = 0;
    virtual bool WriteMessage(const BufferPtr& buff) const = 0;
   public:
    virtual ~RpcMessage() = default;

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(RawObjectTag);
      size += GetMessageSize();
      return size;
    }

    bool Write(const BufferPtr& buff) const;

    virtual bool Equals(const RpcMessagePtr& msg) const = 0;

    static RpcMessagePtr From(Session<RpcMessage>* session, const BufferPtr& buffer);
  };

#define DEFINE_RPC_MESSAGE_TYPE(Name) \
    Type GetType() const{ return Type::k##Name##Message; }
#define DEFINE_RPC_MESSAGE(Name) \
    DEFINE_RPC_MESSAGE_TYPE(Name) \
    virtual int64_t GetMessageSize() const; \
    virtual bool WriteMessage(const BufferPtr& buff) const;
}

#endif//TOKEN_RPC_MESSAGE_H

#include "rpc/rpc_message_version.h"
#include "rpc/rpc_message_paxos.h"
#include "rpc/rpc_message_inventory.h"
#include "rpc/rpc_message_object.h"
#include "rpc/rpc_message_getblocks.h"