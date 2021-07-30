#ifndef TOKEN_RPC_MESSAGE_HANDSHAKE_H
#define TOKEN_RPC_MESSAGE_HANDSHAKE_H

#include "uuid.h"
#include "rpc/rpc_common.h"
#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    class HandshakeMessage : public Message{
     public:
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

  namespace codec{
    template<class M>
    class HandshakeMessageEncoder : public MessageEncoder<M>{
    protected:
      HandshakeMessageEncoder(const M* value, const codec::EncoderFlags& flags):
        MessageEncoder<M>(value, flags){}
    public:
      HandshakeMessageEncoder(const HandshakeMessageEncoder<M>& other) = default;
      ~HandshakeMessageEncoder<M>() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = MessageEncoder<M>::GetBufferSize();
        size += sizeof(RawTimestamp); // timestamp_
        size += Hash::GetSize(); // nonce_
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(!MessageEncoder<M>::Encode(buff))
          return false;

        const auto& timestamp = MessageEncoder<M>::value()->timestamp();
        if(!buff->PutTimestamp(timestamp)){
          LOG(FATAL) << "cannot encode timestamp to buffer.";
          return false;
        }

        //TODO: encoder client_type_
        //TODO: encoder version_

        const auto& nonce = MessageEncoder<M>::value()->nonce();
        if(!buff->PutHash(nonce)){
          LOG(FATAL) << "cannot encode hash to buffer.";
          return false;
        }

        //TODO: encode node_id
        return true;
      }

      HandshakeMessageEncoder<M>& operator=(const HandshakeMessageEncoder<M>& other) = default;
    };

    template<class M>
    class HandshakeMessageDecoder : public MessageDecoder<M>{
    protected:
      explicit HandshakeMessageDecoder(const codec::DecoderHints& hints):
        MessageDecoder<M>(hints){}

      static inline bool
      DecodeHandshakeData(const BufferPtr& buff, Timestamp& timestamp, Hash& nonce){
        timestamp = buff->GetTimestamp();
        DECODED_FIELD(timestamp_, Timestamp, FormatTimestampReadable(timestamp));

        nonce = buff->GetHash();
        DECODED_FIELD(nonce_, Hash, nonce);
        return true;
      }
    public:
      ~HandshakeMessageDecoder() override = default;
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_HANDSHAKE_H