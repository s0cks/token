#include "buffer.h"
#include "network/rpc_message_verack.h"

namespace token{
  namespace rpc{
    bool VerackMessage::Decoder::Decode(const BufferPtr& buff, VerackMessage& result) const{
      CHECK_CODEC_VERSION(FATAL, buff);

      Timestamp timestamp = buff->GetTimestamp();
      DLOG(INFO) << "decoded Timestamp: " << ToUnixTimestamp(timestamp);

      ClientType client_type = ClientType::kNode; //TODO: decode client_type_
      DLOG(INFO) << "decoded ClientType: " << client_type;

      Version version = Version::CurrentVersion(); //TODO: decode version_
      DLOG(INFO) << "decoded Version: " << version;

      Hash nonce = buff->GetHash();
      DLOG(INFO) << "decoded Hash: " << nonce;

      UUID node_id = buff->GetUUID();
      DLOG(INFO) << "decoded UUID: " << node_id;

      BlockHeader head; //TODO: decode head_
      DLOG(INFO) << "decoded BlockHeader: " << head;

      NodeAddress callback; //TODO: decode callback_
      DLOG(INFO) << "decoded NodeAddress: " << callback;

      result = VerackMessage(timestamp, client_type, version, nonce, node_id, head, callback);
      return true;
    }
  }
}