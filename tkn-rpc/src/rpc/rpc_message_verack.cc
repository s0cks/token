#include "rpc/rpc_message_verack.h"

namespace token{
  namespace rpc{
    bool VerackMessage::Decoder::Decode(const BufferPtr& buff, rpc::VerackMessage& result) const{
      Timestamp timestamp;
      rpc::ClientType client_type = rpc::ClientType::kUnknown;
      Hash nonce;
      Version version(0, 0, 0);
      UUID node_id;
      if(!DecodeHandshakeData(buff, timestamp, nonce)){
        LOG(FATAL) << "cannot decode handshake data from buffer.";
        return false;
      }

      result = rpc::VerackMessage(timestamp, client_type, version, nonce, node_id);
      return true;
    }
  }
}