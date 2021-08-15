#include "block.h"

#include "runtime.h"
#include "node/node_server.h"
#include "node/node_session.h"
#include "node/node_session_message_handler.h"

namespace token{
  namespace node{
    //TODO:
    // - state check
    // - version check
    // - echo nonce
    void SessionMessageHandler::OnVersionMessage(const rpc::VersionMessagePtr &msg) {
      // upon receiving a VersionMessage from a new client, respond w/ a VersionMessage
      // to initiate the connection handshake
      Timestamp timestamp = Clock::now();
      rpc::ClientType client_type = rpc::ClientType::kNode;
      Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      Hash nonce = msg->nonce();
      UUID node_id; //TODO: get server node id
      BlockPtr head = Block::Genesis();//TODO: get head

      Send(rpc::VersionMessage::NewInstance(timestamp, client_type, version, nonce, node_id));
    }

//TODO:
// - verify nonce
    void SessionMessageHandler::OnVerackMessage(const rpc::VerackMessagePtr &msg) {
      // upon receiving a VerackMessage from a new client, respond w/ a VerackMessage
      // to finish the connection handshake
      Timestamp timestamp = Clock::now();
      rpc::ClientType client_type = rpc::ClientType::kNode;
      Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      Hash nonce = msg->nonce();
      UUID node_id; //TODO: get server node id
      BlockPtr head = Block::Genesis();//TODO: get head
      Send(rpc::VerackMessage::NewInstance(Clock::now(), client_type, version, nonce, node_id));
      //TODO: connect to peer if not connected
    }

    void SessionMessageHandler::OnPrepareMessage(const rpc::PrepareMessagePtr& msg){
      auto session = (node::Session*)GetSession();
      auto proposal = msg->proposal();
      //TODO:
      // - check if there is an active proposal
      // - check if the proposal is valid
      // - set proposer reference near proposal


      auto& state = session->GetServer()->runtime()->GetProposalState();
      state.SetCurrentProposal(proposal, session);//TODO: reject proposal if you cant set it

      DLOG(INFO) << "received prepare message.";
      auto& acceptor = session->GetServer()->runtime()->GetAcceptor();


//TODO:
//      if(!acceptor.OnProposalPrepare()){
//        DLOG(FATAL) << "cannot start accepting proposal.";
//        //TODO: reject proposal
//      }
    }

    void SessionMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr &msg){
      NOT_IMPLEMENTED(WARNING);//should never happen
    }

    void SessionMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr &msg){
      auto session = (node::Session*)GetSession();
      //TODO:
      // - check if there is an active proposal
      // - check if the proposal is valid
      // - set proposer reference near proposal

      auto& acceptor = session->GetServer()->runtime()->GetAcceptor();

//TODO:
//      if(!acceptor.OnProposalCommit())
//        DLOG(FATAL) << "cannot start committing proposal.";
    }

    void SessionMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr &msg){
      NOT_IMPLEMENTED(ERROR);//TODO: implement?
    }

    void SessionMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr &msg){
      NOT_IMPLEMENTED(ERROR);//TODO: implement?
    }

    void SessionMessageHandler::OnBlockMessage(const rpc::BlockMessagePtr& msg){
      auto session = (node::Session*)GetSession();
      auto blk = Block::Genesis();//TODO: use message block
      auto hash = blk->hash();
      DLOG(INFO) << "received block: " << hash;
//TODO:
//      auto& pool = session->GetServer()->runtime()->GetPool();
//      if(!pool.PutBlock(hash, blk))
//        DLOG(FATAL) << "cannot put block in object pool.";
    }
  }
}

