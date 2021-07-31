#ifndef TOKEN_RPC_MESSAGE_HANDSHAKE_H
#define TOKEN_RPC_MESSAGE_HANDSHAKE_H

#include "uuid.h"
#include "builder.h"
#include "rpc/rpc_common.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    template<class M>
    class HandshakeMessage : public RawMessage<M>{
    protected:
      template<class T>
      class HandshakeMessageBuilderBase : public internal::ProtoBuilder<T, M>{
      public:
        explicit HandshakeMessageBuilderBase(M* raw):
          internal::ProtoBuilder<T, M>(raw){}
        HandshakeMessageBuilderBase() = default;
        ~HandshakeMessageBuilderBase() = default;

        void SetTimestamp(const Timestamp& val){
          return internal::ProtoBuilder<T, M>::raw_->set_timestamp(ToUnixTimestamp(val));
        }

        void SetClientType(const ClientType& val){
          return internal::ProtoBuilder<T, M>::raw_->set_client_type(static_cast<uint32_t>(val));
        }
      };

      HandshakeMessage() = default;
      explicit HandshakeMessage(M raw):
        RawMessage<M>(std::move(raw)){}
      explicit HandshakeMessage(const BufferPtr& data):
        RawMessage<M>(data){}
    public:
      ~HandshakeMessage() override = default;

      Timestamp timestamp() const{
        return FromUnixTimestamp(RawMessage<M>::raw()->timestamp());
      }

      ClientType client_type() const{
        return static_cast<ClientType>(RawMessage<M>::raw()->client_type());
      }

      Version version() const{
        return Version();//TODO: implement
      }

      Hash nonce(){
        return Hash::FromHexString(RawMessage<M>::raw().nonce());
      }

      UUID node_id() const{
        return UUID(RawMessage<M>::raw().node_id());
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_HANDSHAKE_H