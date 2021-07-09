#include <glog/logging.h>
#include "network/rpc_message.h"
#include "network/rpc_server.h"

#include "pool.h"
#include "miner.h"

#include "network/peer_session_manager.h"

// The server session receives packets sent from peers
// this is for inbound packets
namespace token{
  namespace rpc{
#define INITIALIZE_ASYNC_HANDLER(LevelName, Handle, Handler) \
    (Handle)->data = this;                               \
    if((err = uv_async_init(loop, (Handle), (Handler))) != 0){ \
      LOG(LevelName) << "cannot initialize the " << #Handle << " callback handle: " << uv_strerror(err); \
      return;                                            \
    }

    Session::Session():
      SessionBase(),
      send_version_(),
      send_verack_(),
      send_prepare_(),
      send_promise_(),
      send_commit_(),
      send_accepted_(),
      send_rejected_(){

    }

    Session::Session(uv_loop_t* loop, const UUID& uuid):
      SessionBase(loop, uuid),
      send_version_(),
      send_verack_(),
      send_prepare_(),
      send_promise_(),
      send_commit_(),
      send_accepted_(),
      send_rejected_(){
      int err;
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_version_, &OnSendVersion);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_verack_, &OnSendVerack);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_prepare_, &OnSendPrepare);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_promise_, &OnSendPromise);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_commit_, &OnSendCommit);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_accepted_, &OnSendAccepted);
      INITIALIZE_ASYNC_HANDLER(ERROR, &send_rejected_, &OnSendRejected);
    }

    void Session::OnSendVersion(uv_async_t *handle){

      auto session = (Session*)handle->data;

      Timestamp timestamp = Clock::now();
      Version version = Version::CurrentVersion();
      Hash nonce = Hash::GenerateNonce();
      ClientType type = ClientType::kUnknown;
      UUID node_id = UUID();
      BlockHeader head;

      DLOG(INFO) << "sending Version message....";
      session->Send(VersionMessage::NewInstance(timestamp, type, version, nonce, node_id, head));
    }

    void Session::OnSendVerack(uv_async_t *handle){

      auto session = (Session*)handle->data;

      Timestamp timestamp = Clock::now();
      Version version = Version::CurrentVersion();
      Hash nonce = Hash::GenerateNonce();
      ClientType type = ClientType::kUnknown;
      UUID node_id = UUID();
      BlockHeader head;
      NodeAddress callback = NodeAddress();

      DLOG(INFO) << "sending Verack message.....";
      session->Send(VerackMessage::NewInstance(timestamp, type, version, nonce, node_id, head, callback));
    }

    void Session::OnSendPrepare(uv_async_t* handle){

    }

    void Session::OnSendPromise(uv_async_t *handle){

    }

    void Session::OnSendCommit(uv_async_t *handle){

    }

    void Session::OnSendAccepted(uv_async_t *handle){

    }

    void Session::OnSendRejected(uv_async_t *handle){

    }
  }
}