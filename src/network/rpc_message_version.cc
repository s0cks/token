#include "buffer.h"
#include "network/rpc_message_version.h"

namespace token{
  namespace rpc{
    bool VersionMessage::Encoder::Encode(const BufferPtr &buffer) const{
      if(ShouldEncodeType() && !buffer->PutUnsignedLong(static_cast<uint64_t>(value().type()))){
        DLOG(ERROR) << "couldn't encode type.";
        return false;
      }

      if(ShouldEncodeVersion() && !buffer->PutVersion(value().version())){
        DLOG(ERROR) << "couldn't encode version.";
        return false;
      }
      return HandshakeMessageEncoder::Encode(buffer);
    }

    bool VersionMessage::Decoder::Decode(const BufferPtr& buff, VersionMessage& result) const{
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

      result = VersionMessage(timestamp, client_type, version, nonce, node_id, head);
      return true;
    }
  }
}