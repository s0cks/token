#include "rpc/rpc_message.h"
#include "rpc/messages/rpc_message_version.h"

namespace token{
  namespace rpc{
    int64_t VersionMessage::GetMessageSize() const{
      int64_t size = 0;
      size += sizeof(Timestamp); // timestamp_
      size += sizeof(int32_t); // client_type_
      size += sizeof(RawVersion); // version_
      size += Hash::GetSize(); // nonce_
      size += UUID::GetSize(); // node_id_
      size += BlockHeader::GetSize(); // head_
      return size;
    }

    bool VersionMessage::WriteMessage(const BufferPtr& buff) const{
      return buff->PutLong(ToUnixTimestamp(timestamp_))
         && buff->PutInt(static_cast<int32_t>(client_type_))
         && buff->PutVersion(version_)
         && buff->PutHash(nonce_)
         && buff->PutUUID(node_id_)
         && head_.Write(buff);
    }

    int64_t VerackMessage::GetMessageSize() const{
      int64_t size = 0;
      size += sizeof(Timestamp); // timestamp_
      size += sizeof(int32_t); // client_type_
      size += sizeof(RawVersion); // version_
      size += Hash::GetSize(); // nonce_
      size += UUID::GetSize(); // node_id_
      size += NodeAddress::kSize; // callback_
      size += BlockHeader::GetSize(); // head_
      return size;
    }

    bool VerackMessage::WriteMessage(const BufferPtr& buff) const{
      return buff->PutLong(ToUnixTimestamp(timestamp_))
         && buff->PutInt(static_cast<int32_t>(GetClientType()))
         && buff->PutVersion(version_)
         && buff->PutHash(nonce_)
         && buff->PutUUID(node_id_)
         && callback_.Write(buff)
         && head_.Write(buff);
    }
  }
}