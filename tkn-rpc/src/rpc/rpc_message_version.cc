#include "rpc/rpc_message_version.h"

namespace token{
  namespace codec{
    bool VersionMessageDecoder::Decode(const BufferPtr& buff, rpc::VersionMessage& result) const{
      Timestamp timestamp;
      rpc::ClientType client_type = rpc::ClientType::kUnknown;
      Hash nonce;
      Version version(0, 0, 0);
      UUID node_id;
      if(!DecodeHandshakeData(buff, timestamp, nonce)){
        LOG(FATAL) << "cannot decode handshake data from buffer.";
        return false;
      }

      result = rpc::VersionMessage(timestamp, client_type, version, nonce, node_id);
      return true;
    }
  }
}