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

      void HandleMessages(BufferPtr& data, MessageHandler& handler){
        rpc::MessageParser parser(data);
        DVLOG(1) << "parsing message buffer of size: " << data->GetWritePosition();
        while(parser.HasNext()){
          auto next = parser.Next();
          DVLOG(1) << "parsed message: " << next->ToString();
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