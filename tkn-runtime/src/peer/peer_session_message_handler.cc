#include "runtime.h"
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
      PeerSessionManager::OnPeerConnected(GetSession()->GetUUID());
    }

    void SessionMessageHandler::OnPrepareMessage(const rpc::PrepareMessagePtr& msg){
      NOT_IMPLEMENTED(WARNING);//should never happen
    }

    void SessionMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr& msg){
      auto session = (peer::Session*)GetSession();
      auto& election = session->GetRuntime()->GetProposer().GetPrepareElection();
      if(!election.IsInProgress()){
        DLOG(ERROR) << "election is not in-progress.";
        return;
      }
      auto node_id = session->GetUUID();
      election.Accept(node_id);
    }

    void SessionMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr& msg){
      NOT_IMPLEMENTED(WARNING);//should never happen
    }

    void SessionMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr& msg){
      auto session = (peer::Session*)GetSession();
      auto& election = session->GetRuntime()->GetProposer().GetCommitElection();
      if(!election.IsInProgress()){
        DLOG(ERROR) << "election is not in-progress.";
        return;
      }
      election.Accept(session->GetUUID());
    }

    void SessionMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr& msg){
      auto session = (peer::Session*)GetSession();
      auto& election = session->GetRuntime()->GetProposer().GetCommitElection();
      if(!election.IsInProgress()){
        DLOG(ERROR) << "election is not in-progress.";
        return;
      }
      election.Reject(session->GetUUID());
    }

    void SessionMessageHandler::OnBlockMessage(const rpc::BlockMessagePtr& msg){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
    }
  }
}