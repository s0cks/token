#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include <ostream>
#include "block.h"
#include "version.h"
#include "address.h"

namespace token{
  namespace rpc{
    //TODO:
    // - refactor this
    enum class ClientType{
      kUnknown = 0,
      kNode,
      kClient
    };

    static std::ostream& operator<<(std::ostream& stream, const ClientType& type){
      switch(type){
        case ClientType::kNode:return stream << "Node";
        case ClientType::kClient:return stream << "Client";
        case ClientType::kUnknown:
        default:return stream << "Unknown";
      }
    }

    class VersionMessage: public rpc::Message{
    private:
      Timestamp timestamp_; //TODO: make timestamp_ comparable
      ClientType client_type_; //TODO: refactor this field
      Version version_;
      Hash nonce_; //TODO: make useful
      UUID node_id_;
      BlockHeader head_;
    public:
      VersionMessage(const Timestamp& timestamp,
                     const ClientType& client_type,
                     const Version& version,
                     const Hash& nonce,
                     const UUID& node_id,
                     const BlockHeader& head):
         rpc::Message(),
         timestamp_(timestamp),
         client_type_(client_type),
         version_(version),
         nonce_(nonce),
         node_id_(node_id),
         head_(head){}

      explicit VersionMessage(const BufferPtr& buff):
         rpc::Message(),
         timestamp_(FromUnixTimestamp(buff->GetLong())),
         client_type_(static_cast<ClientType>(buff->GetInt())),
         version_(buff->GetVersion()),
         nonce_(buff->GetHash()),
         node_id_(buff->GetUUID()),
         head_(buff){}

      ~VersionMessage() = default;

      Timestamp GetTimestamp() const{
        return timestamp_;
      }

      BlockHeader GetHead() const{
        return head_;
      }

      Version GetVersion() const{
        return version_;
      }

      Hash GetNonce() const{
        return nonce_;
      }

      UUID GetID() const{
        return node_id_;
      }

      ClientType GetClientType() const{
        return client_type_;
      }

      bool IsNode() const{
        return GetClientType() == ClientType::kNode;
      }

      bool IsClient() const{
        return GetClientType() == ClientType::kClient;
      }

      bool Equals(const rpc::MessagePtr& obj) const override{
        if(!obj->IsVersionMessage())
          return false;
        auto other = std::static_pointer_cast<rpc::VersionMessage>(obj);
        return GetClientType() == other->GetClientType()
           && GetVersion() == other->GetVersion()
           && GetNonce() == other->GetNonce()
           && GetID() == other->GetID()
           && GetHead() == other->GetHead();
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "VersionMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
        ss << "client_type=" << client_type_ << ", ";
        ss << "version=" << version_ << ", ";
        ss << "nonce=" << nonce_ << ", ";
        ss << "node_id=" << node_id_ << ", ";
        ss << "head=" << head_;
        ss << ")";
        return ss.str();
      }

      DEFINE_RPC_MESSAGE(Version);

      static inline VersionMessagePtr
      NewInstance(const BufferPtr& buff){
        return std::make_shared<VersionMessage>(buff);
      }

      static inline VersionMessagePtr
      NewInstance(const Timestamp& timestamp,
                  const ClientType& client_type,
                  const Version& version,
                  const Hash& nonce,
                  const UUID& node_id,
                  const BlockHeader& head){
        return std::make_shared<VersionMessage>(timestamp, client_type, version, nonce, node_id, head);
      }

      static inline VersionMessagePtr
      NewInstance(const Timestamp& timestamp,
                  const ClientType& type,
                  const Version& version,
                  const Hash& nonce,
                  const UUID& node_id,
                  const BlockPtr& blk){
        return NewInstance(timestamp, type, version, nonce, node_id, blk->GetHeader());
      }
    };

    class VerackMessage: public rpc::Message{
    private:
      Timestamp timestamp_; //TODO: make timestamp_ comparable
      ClientType client_type_; //TODO: refactor this field
      Version version_;
      Hash nonce_;
      UUID node_id_;
      NodeAddress callback_;
      BlockHeader head_;
    public:
      VerackMessage(ClientType type,
                    const UUID& node_id,
                    const NodeAddress& callback,
                    const Version& version,
                    const BlockHeader& head,
                    const Hash& nonce,
                    Timestamp timestamp = Clock::now()):
         rpc::Message(),
         timestamp_(timestamp),
         client_type_(type),
         version_(version),
         nonce_(nonce),
         node_id_(node_id),
         callback_(callback),
         head_(head){}

      explicit VerackMessage(const BufferPtr& buff):
         rpc::Message(),
         timestamp_(FromUnixTimestamp(buff->GetLong())),
         client_type_(static_cast<ClientType>(buff->GetInt())),
         version_(buff->GetVersion()),
         nonce_(buff->GetHash()),
         node_id_(buff->GetUUID()),
         callback_(buff),
         head_(buff){}

      ~VerackMessage() = default;

      Timestamp GetTimestamp() const{
        return timestamp_;
      }

      ClientType GetClientType() const{
        return client_type_;
      }

      bool IsNode() const{
        return GetClientType() == ClientType::kNode;
      }

      bool IsClient() const{
        return GetClientType() == ClientType::kClient;
      }

      UUID GetID() const{
        return node_id_;
      }

      NodeAddress GetCallbackAddress() const{
        return callback_;
      }

      BlockHeader GetHead() const{
        return head_;
      }

      bool Equals(const rpc::MessagePtr& obj) const override{
        if(!obj->IsVerackMessage())
          return false;
        VerackMessagePtr msg = std::static_pointer_cast<VerackMessage>(obj);
        return client_type_ == msg->client_type_
           && version_ == msg->version_
           && nonce_ == msg->nonce_
           && node_id_ == msg->node_id_
           && callback_ == msg->callback_
           && head_ == msg->head_;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "VerackMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
        ss << "client_type=" << client_type_ << ", ";
        ss << "version=" << version_ << ", ";
        ss << "nonce=" << nonce_ << ", ";
        ss << "node_id=" << node_id_ << ", ";
        ss << "callback=" << callback_ << ", ";
        ss << "head=" << head_;
        ss << ")";
        return ss.str();
      }

      DEFINE_RPC_MESSAGE(Verack);

      static inline VerackMessagePtr
      NewInstance(ClientType type,
                  const UUID& node_id,
                  const NodeAddress& callback,
                  const Version& version,
                  const BlockHeader& head,
                  const Hash& nonce,
                  Timestamp timestamp = Clock::now()){
        return std::make_shared<VerackMessage>(type, node_id, callback, version, head, nonce, timestamp);
      }

      static inline VerackMessagePtr
      NewInstance(const BufferPtr& buff){
        return std::make_shared<VerackMessage>(buff);
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H