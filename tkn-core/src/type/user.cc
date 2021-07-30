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

  User* User::Decoder::Decode(const BufferPtr& buff) const{
    auto length = internal::kMaxUserLength;
    uint8_t bytes[length];
    if(!buff->GetBytes(bytes, static_cast<int64_t>(length)))
      CANNOT_DECODE_FIELD(bytes_, uint8_t[]);
    return new User(bytes, length);
  }
}