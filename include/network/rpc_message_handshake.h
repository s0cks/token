#ifndef TOKEN_RPC_MESSAGE_HANDSHAKE_H
#define TOKEN_RPC_MESSAGE_HANDSHAKE_H

#include "buffer.h"
#include "network/rpc_message.h"

namespace token{
  namespace rpc{
    class HandshakeMessage : public Message{
     public:
      template<class M>
      class HandshakeMessageEncoder : public MessageEncoder<M>{
       protected:
        UUID::Encoder node_id_encoder_;

        HandshakeMessageEncoder(const M& value, const codec::EncoderFlags& flags):
            MessageEncoder<M>(value, flags),
            node_id_encoder_(value.node_id(), flags){}
       public:
        HandshakeMessageEncoder(const HandshakeMessageEncoder<M>& other) = default;
        ~HandshakeMessageEncoder<M>() override = default;

        int64_t GetBufferSize() const override{
          //TODO: use UUID encoder

          int64_t size = codec::EncoderBase<M>::GetBufferSize();
          size += sizeof(RawTimestamp); // timestamp_
          size += sizeof(uint32_t); // client_type_
          size += kVersionSize; // version_
          size += Hash::GetSize(); // nonce_
          size += node_id_encoder_.GetBufferSize(); // node_id
          return size;
        }

        bool Encode(const BufferPtr& buff) const override{
          const Timestamp& timestamp = MessageEncoder<M>::value().timestamp();
          if(!buff->PutTimestamp(timestamp)){
            CANNOT_ENCODE_VALUE(FATAL, Timestamp, ToUnixTimestamp(timestamp));
            return false;
          }

          //TODO: encode client_type_
          //TODO: encode version_

          const Hash& nonce = MessageEncoder<M>::value().nonce();
          if(!buff->PutHash(nonce)){
            CANNOT_ENCODE_VALUE(FATAL, Hash, nonce);
            return false;
          }

          if(!node_id_encoder_.Encode(buff)){
            LOG(FATAL) << "couldn't encode node_id to buffer.";
            return false;
          }

          //TODO: encode head_
          return true;
        }

        HandshakeMessageEncoder<M>& operator=(const HandshakeMessageEncoder<M>& other) = default;
      };

      template<class M>
      class HandshakeMessageDecoder : public MessageDecoder<M>{
       protected:
        UUID::Decoder node_id_decoder_;

        explicit HandshakeMessageDecoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
            MessageDecoder<M>(hints),
            node_id_decoder_(hints){}
       public:
        HandshakeMessageDecoder(const HandshakeMessageDecoder<M>& other) = default;
        ~HandshakeMessageDecoder<M>() override = default;
        virtual bool Decode(const BufferPtr& buff, M& result) const = 0;
        HandshakeMessageDecoder<M>& operator=(const HandshakeMessageDecoder<M>& other) = default;
      };

      Timestamp timestamp_; //TODO: make timestamp_ comparable
      ClientType client_type_; //TODO: refactor this field
      Version version_;
      Hash nonce_; //TODO: make useful
      UUID node_id_;

      HandshakeMessage():
          Message(),
          timestamp_(),
          client_type_(),
          version_(),
          nonce_(),
          node_id_(){}
      HandshakeMessage(const Timestamp& timestamp,
                       const ClientType& client_type,
                       const Version& version,
                       const Hash& nonce,
                       const UUID& node_id):
          Message(),
          timestamp_(timestamp),
          client_type_(client_type),
          version_(version),
          nonce_(nonce),
          node_id_(node_id){}
     public:
      HandshakeMessage(const HandshakeMessage& other) = default;
      ~HandshakeMessage() override = default;

      Timestamp& timestamp(){
        return timestamp_;
      }

      Timestamp timestamp() const{
        return timestamp_;
      }

      Version& version(){
        return version_;
      }

      Version version() const{
        return version_;
      }

      Hash& nonce(){
        return nonce_;
      }

      Hash nonce() const{
        return nonce_;
      }

      UUID& node_id(){
        return node_id_;
      }

      UUID node_id() const{
        return node_id_;
      }

      ClientType& client_type(){
        return client_type_;
      }

      ClientType client_type() const{
        return client_type_;
      }

      bool IsNode() const{
        return client_type_ == ClientType::kNode;
      }

      bool IsClient() const{
        return client_type_ == ClientType::kClient;
      }

      HandshakeMessage& operator=(const HandshakeMessage& other) = default;
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_HANDSHAKE_H