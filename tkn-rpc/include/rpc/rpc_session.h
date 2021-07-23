#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "server/session.h"
#include "rpc/rpc_messages.h"
#include "rpc/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Session : public SessionBase{
     protected:
      Session() = default;
      explicit Session(uv_loop_t* loop, const UUID& uuid=UUID()):
        SessionBase(loop, uuid){}

      static inline void
      HandleMessages(rpc::MessageParser& parser, rpc::MessageHandler& handler){
        while(parser.HasNext()){
          auto next = parser.Next();
          DLOG(INFO) << "received message " << next->ToString();
          switch(next->type()){
#define HANDLE(Name) \
            case Type::k##Name##Message:{ \
              handler.On##Name##Message(std::static_pointer_cast<rpc::Name##Message>(next)); \
              break;          \
            }
            FOR_EACH_MESSAGE_TYPE(HANDLE)
#undef HANDLE
            default:{
              DLOG(WARNING) << "cannot handle message: " << next->ToString();
              break;
            }
          }
        }
      }

      static inline void
      HandleMessages(const BufferPtr& data, rpc::MessageHandler& handler){
        rpc::MessageParser parser(data);
        return HandleMessages(parser, handler);
      }
     public:
      ~Session() override = default;

      virtual void Send(const rpc::MessageList& messages){
        return SendMessages((const std::vector<std::shared_ptr<MessageBase>>&)messages);
      }

      virtual void Send(const rpc::MessagePtr& msg){
        std::vector<std::shared_ptr<MessageBase>> messages = { msg };
        return SendMessages(messages);
      }
    };
  }
}

#endif//TOKEN_RPC_SESSION_H