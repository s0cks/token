#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "network/rpc_common.h"
#include "network/rpc_messages.h"
#include "network/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Session : public SessionBase{
      friend class PeerSession;
     protected:
      uv_async_t send_version_;
      uv_async_t send_verack_;
      uv_async_t send_prepare_;
      uv_async_t send_promise_;
      uv_async_t send_commit_;
      uv_async_t send_accepted_;
      uv_async_t send_rejected_;

      Session();
      explicit Session(uv_loop_t* loop, const UUID& uuid=UUID());

      static void OnSendVersion(uv_async_t* handle);
      static void OnSendVerack(uv_async_t* handle);
      static void OnSendPrepare(uv_async_t* handle);
      static void OnSendPromise(uv_async_t* handle);
      static void OnSendCommit(uv_async_t* handle);
      static void OnSendAccepted(uv_async_t* handle);
      static void OnSendRejected(uv_async_t* handle);
     public:
      ~Session() override = default;

      virtual rpc::MessageHandler& GetMessageHandler() = 0;
      virtual BlockChainPtr GetChain() const = 0;

      virtual void Send(const rpc::MessageList& messages){
        return SendMessages((const std::vector<std::shared_ptr<MessageBase>>&)messages);
      }

      virtual void Send(const rpc::MessagePtr& msg){
        std::vector<std::shared_ptr<MessageBase>> messages = { msg };
        return SendMessages(messages);
      }

      inline bool SendVersion(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_version_), "cannot send Version");
        return true;
      }

      inline bool SendVerack(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_verack_), "cannot send Verack");
        return true;
      }

      inline bool SendPrepare(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_prepare_), "cannot send Prepare");
        return true;
      }

      inline bool SendPromise(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_promise_), "cannot send Promise");
        return true;
      }

      inline bool SendCommit(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_commit_), "cannot send Commit");
        return true;
      }

      inline bool SendAccepted(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_accepted_), "cannot send Accepted");
        return true;
      }

      inline bool SendRejected(){
        VERIFY_UVRESULT2(WARNING, uv_async_send(&send_rejected_), "cannot send Rejected");
        return true;
      }

    };
  }
}

#endif//TOKEN_RPC_SESSION_H