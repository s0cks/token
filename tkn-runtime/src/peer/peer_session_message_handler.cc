#include "peer/peer_session.h"
#include "peer/peer_session_message_handler.h"

namespace token{
  namespace peer{
    void SessionMessageHandler::OnVersionMessage(const rpc::VersionMessagePtr& msg){
      auto session = (peer::Session*)GetSession();//TODO: fix cast
      session->SendVerack();
    }

    void SessionMessageHandler::OnVerackMessage(const rpc::VerackMessagePtr& msg){
      DLOG(INFO) << "connected!";//TODO: fix logging
    }

    void SessionMessageHandler::OnPrepareMessage(const rpc::PrepareMessagePtr& msg){

    }

    void SessionMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr& msg){

    }

    void SessionMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr& msg){

    }

    void SessionMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr& msg){

    }

    void SessionMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr& msg){

    }

    void SessionMessageHandler::OnTransactionMessage(const rpc::TransactionMessagePtr& msg){

    }

    void SessionMessageHandler::OnBlockMessage(const rpc::BlockMessagePtr& msg){

    }
  }
}