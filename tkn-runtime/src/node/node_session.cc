#include "proposal_scope.h"
#include "node/node_server.h"
#include "node/node_session.h"

namespace token{
  namespace node{
    Session::Session(Server* server):
      rpc::Session(server->GetLoop()),
      server_(server),
      handler_(this),
      on_send_promise_(),
      on_send_accepted_(),
      on_send_rejected_(){
      on_send_promise_.data = this;
      CHECK_UVRESULT2(FATAL, uv_async_init(server->GetLoop(), &on_send_promise_, &OnSendPromise), "cannot initialize on_send_promise_ callback");
      on_send_accepted_.data = this;
      CHECK_UVRESULT2(FATAL, uv_async_init(server->GetLoop(), &on_send_accepted_, &OnSendAccepted), "cannot initialize on_send_accepted_ callback");
      on_send_rejected_.data = this;
      CHECK_UVRESULT2(FATAL, uv_async_init(server->GetLoop(), &on_send_rejected_, &OnSendRejected), "cannot initialize on_send_rejected_ callback");
    }

    void Session::OnSendPromise(uv_async_t* handle){
      auto session = (Session*)handle->data;
      auto proposal = ProposalScope::GetCurrentProposal();
      session->Send(rpc::PromiseMessage::NewInstance(*proposal));//TODO: memory-leak
    }

    void Session::OnSendAccepted(uv_async_t* handle){
      auto session = (Session*)handle->data;
      auto proposal = ProposalScope::GetCurrentProposal();
      session->Send(rpc::AcceptedMessage::NewInstance(*proposal));//TODO: memory-leak
    }

    void Session::OnSendRejected(uv_async_t* handle){
      auto session = (Session*)handle->data;
      auto proposal = ProposalScope::GetCurrentProposal();
      session->Send(rpc::RejectedMessage::NewInstance(*proposal));//TODO: memory-leak
    }
  }
}