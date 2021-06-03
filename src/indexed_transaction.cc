#include "buffer.h"
#include "indexed_transaction.h"

namespace token{
  IndexedTransactionPtr IndexedTransaction::FromBytes(const BufferPtr& buffer){
    int64_t index = buffer->GetLong();
    Timestamp timestamp = buffer->GetTimestamp();

    InputList inputs;
    if(!buffer->GetList(inputs)){
      LOG(WARNING) << "cannot read input list from buffer of size " << buffer->GetWrittenBytes();
      return nullptr;
    }

    OutputList outputs;
    if(!buffer->GetList(outputs)){
      LOG(WARNING) << "cannot read output list from buffer of size " << buffer->GetWrittenBytes();
      return nullptr;
    }
    return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
  }
}