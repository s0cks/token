#ifndef TOKEN_RPC_MESSAGE_HANDSHAKE_H
#define TOKEN_RPC_MESSAGE_HANDSHAKE_H

#include "uuid.h"
#include "builder.h"
#include "rpc/rpc_common.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    template<class M>
    class HandshakeMessage : public Message{
    protected:
      template<class T>
      class HandshakeMessageBuilderBase : public internal::ProtoBuilder<T, M>{
      public:
        explicit HandshakeMessageBuilderBase(M* raw):
          internal::ProtoBuilder<T, M>(raw){}
        HandshakeMessageBuilderBase() = default;
        ~HandshakeMessageBuilderBase() = default;

        void SetTimestamp(const Timestamp& val){
          return raw_->set_timestamp(ToUnixTimestamp(val));
        }

        void SetClientType(const ClientType& val){
          return raw_->set_client_type(static_cast<uint32_t>(val));
        }
      };

      M raw_;

      HandshakeMessage():
        Message(),
        raw_(){}
      explicit HandshakeMessage(M raw):
        Message(),
        raw_(std::move(raw)){}
    public:
      ~HandshakeMessage() override = default;

      Timestamp timestamp() const{
        return FromUnixTimestamp(raw_.timestamp());
      }

      ClientType client_type() const{
        return static_cast<ClientType>(raw_.client_type());
      }

      Version version() const{
        return Version();//TODO: implement
      }

      Hash nonce() const{
        return Hash::FromHexString(raw_.nonce());
      }

      UUID node_id() const{
        return UUID(raw_.node_id());
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_HANDSHAKE_H