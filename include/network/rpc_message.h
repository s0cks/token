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
  }
}

#endif//TOKEN_RPC_MESSAGE_H