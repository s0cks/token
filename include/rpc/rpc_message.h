#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include <vector>

#include "buffer.h"
#include "message.h"

namespace token{
  template<class M>
  class Session;

  class RpcSession;

  class RpcMessage;
  typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
  typedef std::vector<RpcMessagePtr> RpcMessageList;

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
  Type GetType() const override{ return Type::k##Name##Message; }

#define DEFINE_RPC_MESSAGE(Name) \
  DEFINE_RPC_MESSAGE_TYPE(Name) \
  int64_t GetMessageSize() const override; \
  bool WriteMessage(const BufferPtr& buff) const override;

#define DEFINE_RPC_MESSAGE_CONSTRUCTORS(Name) \
  static inline Name##MessagePtr NewInstance(const BufferPtr& buffer){ return std::make_shared<Name##Message>(buffer); }
}

#include "rpc/rpc_message_getblocks.h"
#include "rpc/rpc_message_inventory.h"
#include "rpc/rpc_message_object.h"
#include "rpc/rpc_message_paxos.h"
#include "rpc/rpc_message_version.h"

namespace token{
#define DEFINE_RPC_MESSAGE_LIST_APPEND(Name) \
  static inline RpcMessageList&              \
  operator<<(RpcMessageList& messages, const Name##MessagePtr& msg){ \
    messages.push_back(msg);                 \
    return messages;                         \
  }
  FOR_EACH_MESSAGE_TYPE(DEFINE_RPC_MESSAGE_LIST_APPEND)
#undef DEFINE_RPC_MESSAGE_LIST_APPEND
}

#endif//TOKEN_RPC_MESSAGE_H