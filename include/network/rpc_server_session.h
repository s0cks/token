#ifndef TOKEN_RPC_SERVER_SESSION_H
#define TOKEN_RPC_SERVER_SESSION_H

#include <memory>
#include "network/rpc_session.h"

namespace token{
  namespace rpc{
    class ServerSession;

    class ServerSessionMessageHandler : public rpc::MessageHandler{
      friend class ServerSession;
     public:
      explicit ServerSessionMessageHandler(ServerSession* session):
        rpc::MessageHandler((rpc::Session*)session){}
      ~ServerSessionMessageHandler() override = default;

      ObjectPoolPtr GetPool() const;
      BlockChainPtr GetChain() const;

#define DECLARE_HANDLE(Name) \
    void On##Name##Message(const Name##MessagePtr& msg) override;
      FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE)
#undef DECLARE_HANDLE
    };

    class ServerSession : public rpc::Session{
     private:
      ObjectPoolPtr pool_;
      BlockChainPtr chain_;
      ServerSessionMessageHandler handler_;
     public:
      ServerSession(ObjectPoolPtr  pool,
                    BlockChainPtr  chain):
        rpc::Session(),
        pool_(std::move(pool)),
        chain_(std::move(chain)),
        handler_(this){}
      ServerSession(uv_loop_t* loop,
                    ObjectPoolPtr pool,
                    const BlockChainPtr& chain):
        rpc::Session(loop),
        pool_(std::move(pool)),
        chain_(chain),
        handler_(this){}
      ~ServerSession() override = default;

      rpc::MessageHandler& GetMessageHandler() override{
        return handler_;
      }

      ObjectPoolPtr GetPool() const{
        return pool_;
      }

      BlockChainPtr GetChain() const{
        return chain_;
      }

      void OnMessageRead(const BufferPtr& buff){
        rpc::MessagePtr msg = rpc::Message::From(buff);
        switch(msg->type()) {
#define DEFINE_HANDLE(Name) \
        case Type::k##Name##Message: \
          return handler_.On##Name##Message(std::static_pointer_cast<rpc::Name##Message>(msg));
          FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
          default: return;
        }
      }
    };
  }
}

#endif//TOKEN_RPC_SERVER_SESSION_H