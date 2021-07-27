#include "env.h"
#include "configuration.h"
#include "proposal_scope.h"
#include "rpc/rpc_messages.h"
#include "peer/peer_session.h"

// The peer session sends packets to a peer
// any response received is purely in relation to the sent packets
// this is for outbound packets
namespace token{
  namespace peer{
    bool Session::Connect(){
      //TODO: StartHeartbeatTimer();
      struct sockaddr_in addr{};
      utils::GetAddress(address(), addr);

      DLOG(INFO) << "creating connection to peer " << address() << "....";
      VERIFY_UVRESULT(uv_tcp_connect(&connection_, &handle_, (const struct sockaddr*)&addr, &OnConnect), LOG(ERROR), "couldn't connect to peer");

      DLOG(INFO) << "starting session loop....";
      VERIFY_UVRESULT(uv_run(GetLoop(), UV_RUN_DEFAULT), LOG(ERROR), "couldn't run loop");
      return true;
    }

    bool Session::Disconnect(){
      int err;
      if((err = uv_async_send(&disconnect_)) != 0){
        LOG(WARNING) << "cannot send disconnect to peer: " << uv_strerror(err);
        return false;
      }
      return true;
    }

    void Session::OnConnect(uv_connect_t* conn, int status){
      auto session = (Session*)conn->data;
      if(status != 0){
        LOG(ERROR) << "connect failure: " << uv_strerror(status);
        session->CloseConnection();
        return;
      }

      session->SetState(Session::kConnectingState);
      session->SendVersion();
      CHECK_UVRESULT2(FATAL, uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived), "peer session read failure");
    }

    void Session::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      auto session = ((peer::Session*)stream->data);
      if(nread == UV_EOF){
        DLOG(WARNING) << "end of stream.";
        return;
      } else if(nread < 0){
        DLOG(ERROR) << "client read error: " << uv_strerror(nread);
        return;
      } else if(nread == 0){
        DLOG(WARNING) << "zero message size received.";
        return;
      } else if(nread > 65536){
        DLOG(WARNING) << "received too large of a buffer (" << nread << "b)";
        return;
      }

      BufferPtr data= internal::CopyBufferFrom((uint8_t *) buff->base, nread);
      HandleMessages(data, session->handler());
    }

    void Session::OnDisconnect(uv_async_t* handle){
      auto session = (Session*)handle->data;
      session->CloseConnection();
    }

    void Session::OnWalk(uv_handle_t* handle, void* data){
      uv_close(handle, &OnClose);
    }

    void Session::OnClose(uv_handle_t* handle){
      DLOG(INFO) << "OnClose.";
    }

    void Session::OnSendVersion(uv_async_t* handle){
      auto session = (peer::Session*)handle->data;
      auto timestamp = Clock::now();
      auto client_type = rpc::ClientType::kNode;
      auto nonce = Hash::GenerateNonce();
      auto version = Version::CurrentVersion();
      auto node_id = UUID();//TODO: use server's node id
      session->Send(rpc::VersionMessage::NewInstance(timestamp, client_type, version, nonce, node_id));
    }

    void Session::OnSendVerack(uv_async_t* handle){
      auto session = (peer::Session*)handle->data;
      auto timestamp = Clock::now();
      auto client_type = rpc::ClientType::kNode;
      auto nonce = Hash::GenerateNonce();
      auto version = Version::CurrentVersion();
      auto node_id = UUID();//TODO: use server's node id
      session->Send(rpc::VerackMessage::NewInstance(timestamp, client_type, version, nonce, node_id));
    }

    void Session::OnSendPrepare(uv_async_t* handle){
      auto session = (peer::Session*)handle->data;
      Proposal* proposal = ProposalScope::GetCurrentProposal();
      session->Send(rpc::PrepareMessage::NewInstance(*proposal));
    }

    void Session::OnSendCommit(uv_async_t* handle){
      auto session = (peer::Session*)handle->data;
      Proposal* proposal = ProposalScope::GetCurrentProposal();
      session->Send(rpc::CommitMessage::NewInstance(*proposal));
    }
  }
}