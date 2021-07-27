#include "type/user.h"

namespace token{
  int64_t User::Encoder::GetBufferSize() const{
    auto size = codec::TypeEncoder<User>::GetBufferSize();
    size += internal::kMaxUserLength;
    return size;
  }

  bool User::Encoder::Encode(const BufferPtr& buff) const{
    const auto data = value()->data();
    const auto length = internal::kMaxUserLength;
    if(!buff->PutBytes((uint8_t*)data, length)){
      DLOG(ERROR) << "cannot put bytes (" << length << "b)";
      return false;
    }
    return true;
  }

  bool User::Decoder::Decode(const BufferPtr& buff, User& result) const{
    auto length = internal::kMaxUserLength;
    uint8_t data[length];
    if(!buff->GetBytes(data, length)){
      DLOG(FATAL) << "cannot decode data (uint8_t[" << length << "]) from buffer.";
      return false;
    }
    result = User(data, length);
    return true;
  }
}