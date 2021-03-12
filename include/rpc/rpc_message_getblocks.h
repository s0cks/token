#ifndef TOKEN_RPC_MESSAGE_GETBLOCKS_H
#define TOKEN_RPC_MESSAGE_GETBLOCKS_H

#include "blockchain.h"

namespace token{
  class GetBlocksMessage : public RpcMessage{
   public:
    static const intptr_t kMaxNumberOfBlocks;
   private:
    Hash start_;
    Hash stop_;
   public:
    GetBlocksMessage(const Hash& start_hash, const Hash& stop_hash):
        RpcMessage(),
        start_(start_hash),
        stop_(stop_hash){}
    ~GetBlocksMessage(){}

    Hash GetHeadHash() const{
      return start_;
    }

    Hash GetStopHash() const{
      return stop_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "GetBlocksMessage(" << start_ << ":" << stop_ << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(GetBlocks);

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsGetBlocksMessage()){
        return false;
      }
      GetBlocksMessagePtr msg = std::static_pointer_cast<GetBlocksMessage>(obj);
      return start_ == msg->start_
          && stop_ == msg->stop_;
    }

    static GetBlocksMessagePtr NewInstance(const BufferPtr& buff);
    static GetBlocksMessagePtr NewInstance(const Hash& start_hash = BlockChain::GetHead()->GetHash(),
                                     const Hash& stop_hash = Hash()){
      return std::make_shared<GetBlocksMessage>(start_hash, stop_hash);
    }
  };
}

#endif//TOKEN_RPC_MESSAGE_GETBLOCKS_H