#ifndef TOKEN_RPC_MESSAGE_GETBLOCKS_H
#define TOKEN_RPC_MESSAGE_GETBLOCKS_H

#include "rpc/rpc_message.h"
#include "blockchain.h"

namespace token{
  namespace rpc{
    static const int64_t kMaxNumberOfBlocksForGetBlocksMessage = 50;
    class GetBlocksMessage: public rpc::Message{
    private:
      Hash start_;
      Hash stop_;
    public:
      GetBlocksMessage(const Hash& start_hash, const Hash& stop_hash):
         rpc::Message(),
         start_(start_hash),
         stop_(stop_hash){}
      ~GetBlocksMessage(){}

      DEFINE_RPC_MESSAGE_TYPE(GetBlocks);

      Hash GetHeadHash() const{
        return start_;
      }

      Hash GetStopHash() const{
        return stop_;
      }

      bool Equals(const rpc::MessagePtr& obj) const override{
        if(!obj->IsGetBlocksMessage())
          return false;
        auto msg = std::static_pointer_cast<GetBlocksMessage>(obj);
        return start_ == msg->start_
           && stop_ == msg->stop_;
      }

      int64_t GetMessageSize() const override{
        return Hash::GetSize() * 2;
      }

      bool WriteMessage(const BufferPtr& buff) const override{
        return buff->PutHash(start_)
           && buff->PutHash(stop_);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "GetBlocksMessage(" << start_ << ":" << stop_ << ")";
        return ss.str();
      }

      static inline rpc::GetBlocksMessagePtr
      NewInstance(const BufferPtr& buff){
        return std::make_shared<rpc::GetBlocksMessage>(buff->GetHash(), buff->GetHash());
      }

      static inline rpc::GetBlocksMessagePtr
      NewInstance(const Hash& start_hash, const Hash& stop_hash = Hash()){
        return std::make_shared<rpc::GetBlocksMessage>(start_hash, stop_hash);
      }
    };

    static inline rpc::MessageList&
    operator<<(rpc::MessageList& list, const rpc::GetBlocksMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_GETBLOCKS_H