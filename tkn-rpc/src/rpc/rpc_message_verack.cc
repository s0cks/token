#include "rpc/rpc_message_verack.h"

namespace token{
  namespace rpc{
    VerackMessage* VerackMessage::Decoder::Decode(const BufferPtr& data) const{//TODO: finish implementation
      Timestamp timestamp;
      rpc::ClientType client_type = rpc::ClientType::kUnknown;
      Hash nonce;
      Version version(0, 0, 0);
      UUID node_id;
      if(!DecodeHandshakeData(data, timestamp, nonce)){
        LOG(FATAL) << "cannot decode handshake data from buffer.";
        return nullptr;
      }
      return new VerackMessage(timestamp, client_type, version, nonce, node_id);
    }
  }
}