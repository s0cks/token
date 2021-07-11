#include "buffer.h"
#include "network/rpc_message_version.h"

namespace token{
  namespace rpc{
    bool VersionMessage::Decoder::Decode(const BufferPtr& buff, VersionMessage& result) const{
      Timestamp timestamp = buff->GetTimestamp();
      DLOG(INFO) << "decoded Timestamp: " << ToUnixTimestamp(timestamp);

      ClientType client_type = ClientType::kNode; //TODO: decode client_type_
      DLOG(INFO) << "decoded ClientType: " << client_type;

      Version version = Version::CurrentVersion(); //TODO: decode version_
      DLOG(INFO) << "decoded Version: " << version;

      Hash nonce = buff->GetHash();
      DLOG(INFO) << "decoded Hash: " << nonce;

      UUID node_id;
      if(!node_id_decoder_.Decode(buff, node_id)){
        LOG(FATAL) << "couldn't decode node_id from buffer.";
        return false;
      }
      DLOG(INFO) << "decoded UUID: " << node_id;

      BlockHeader head; //TODO: decode head_
      DLOG(INFO) << "decoded BlockHeader: " << head;

      result = VersionMessage(timestamp, client_type, version, nonce, node_id, head);
      return true;
    }
  }
}