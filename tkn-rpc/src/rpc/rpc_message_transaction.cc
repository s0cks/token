#include "rpc/rpc_message_transaction.h"

namespace token{
  namespace codec{
    bool TransactionMessageDecoder::Decode(const BufferPtr& buff, rpc::TransactionMessage& result) const{
      UnsignedTransaction value;
      if(!decode_value_.Decode(buff, value)){
        LOG(FATAL) << "cannot decode value.";
        return false;
      }

      //TODO: fix allocation
      result = rpc::TransactionMessage(std::make_shared<UnsignedTransaction>(value));
      return true;
    }
  }
}