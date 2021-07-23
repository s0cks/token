#include "type/user.h"

namespace token{
  namespace codec{
    int64_t UserEncoder::GetBufferSize() const{
      //TODO: implement
      return 0;
    }

    bool UserEncoder::Encode(const BufferPtr& buff) const{
      //TODO: implement
      return false;
    }

    bool UserDecoder::Decode(const BufferPtr& buff, User& result) const{
      //TODO: implement
      return false;
    }
  }
}