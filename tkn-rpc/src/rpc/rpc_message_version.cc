#include "rpc/rpc_message_version.h"

namespace token{
  namespace rpc{
    VersionMessage* VersionMessage::Decoder::Decode(const BufferPtr& data) const{//TODO: finish implementation
      Timestamp timestamp;
      rpc::ClientType client_type = rpc::ClientType::kUnknown;
      Hash nonce;
      Version version(0, 0, 0);
      UUID node_id;
      if(!DecodeHandshakeData(data, timestamp, nonce)){
        DLOG(FATAL) << "cannot decode handshake data.";
        return nullptr;
      }
      return new VersionMessage(timestamp, client_type, version, nonce, node_id);
    }
  }
}