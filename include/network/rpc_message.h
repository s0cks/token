#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include <vector>

#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "message.h"
#include "proposal.h"
#include "network/rpc_common.h"

namespace token{
  namespace rpc{
    class Message: public MessageBase{
    protected:
      Message() = default;
    public:
      ~Message() override = default;
      static MessagePtr From(const BufferPtr& buffer);
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_H