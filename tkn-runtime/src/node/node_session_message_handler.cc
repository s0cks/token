#include "rpc/rpc_common.h"
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
      NOT_IMPLEMENTED(WARNING);
    }

    void SessionMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr &msg) {
      NOT_IMPLEMENTED(WARNING);
    }

    void SessionMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr &msg) {
      NOT_IMPLEMENTED(WARNING);
    }

    void SessionMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr &msg) {
      NOT_IMPLEMENTED(ERROR);
    }

    void SessionMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr &msg){
      NOT_IMPLEMENTED(ERROR);
    }

    void SessionMessageHandler::OnBlockMessage(const rpc::BlockMessagePtr &msg){
      NOT_IMPLEMENTED(ERROR);
    }

    void SessionMessageHandler::OnTransactionMessage(const rpc::TransactionMessagePtr &msg){
      NOT_IMPLEMENTED(ERROR);
    }
  }
}

