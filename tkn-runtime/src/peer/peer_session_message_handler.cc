#include "peer/peer_session.h"
#include "peer/peer_session_message_handler.h"

#include "proposal_scope.h"

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
      NOT_IMPLEMENTED(WARNING);//should never happen
    }

    void SessionMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr& msg){
      auto session = GetSession();
      ProposalScope::Promise(session->GetUUID());
    }

    void SessionMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr& msg){
      NOT_IMPLEMENTED(WARNING);//should never happen
    }

    void SessionMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr& msg){
      auto session = GetSession();
      ProposalScope::Accepted(session->GetUUID());
    }

    void SessionMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr& msg){
      auto session = GetSession();
      ProposalScope::Rejected(session->GetUUID());
    }
  }
}