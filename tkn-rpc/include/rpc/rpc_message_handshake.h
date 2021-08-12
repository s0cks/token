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

        void SetVersion(const Version& val){
          return internal::ProtoBuilder<T, M>::raw_->set_version(val.ToString());
        }

        void SetNonce(const Hash& val){
          return internal::ProtoBuilder<T, M>::raw_->set_nonce(val.HexString());
        }

        void SetNodeId(const UUID& val){
          return internal::ProtoBuilder<T, M>::raw_->set_node_id(val.ToString());
        }
      };

      HandshakeMessage() = default;
      HandshakeMessage(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id):
        RawMessage<M>(){
        RawMessage<M>::raw_.set_timestamp(ToUnixTimestamp(timestamp));
        RawMessage<M>::raw_.set_client_type(static_cast<uint32_t>(client_type));
        RawMessage<M>::raw_.set_version(version.ToString());
        RawMessage<M>::raw_.set_nonce(nonce.HexString());
        RawMessage<M>::raw_.set_node_id(node_id.ToString());
      }
      explicit HandshakeMessage(M raw):
        RawMessage<M>(std::move(raw)){}
      explicit HandshakeMessage(const BufferPtr& data, const uint64_t& msize):
        RawMessage<M>(data, msize){}
    public:
      ~HandshakeMessage() override = default;

      Timestamp timestamp() const{
        return FromUnixTimestamp(RawMessage<M>::raw_.timestamp());
      }

      ClientType client_type() const{
        return static_cast<ClientType>(RawMessage<M>::raw_.client_type());
      }

      Version version() const{
        return Version();//TODO: implement
      }

      Hash nonce() const{
        return Hash::FromHexString(RawMessage<M>::raw_.nonce());
      }

      UUID node_id() const{
        return UUID(RawMessage<M>::raw_.node_id());
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_HANDSHAKE_H