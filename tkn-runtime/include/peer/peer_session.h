#ifndef TOKEN_PEER_SESSION_H
#define TOKEN_PEER_SESSION_H

#include <utility>
#include "rpc/rpc_message.h"
#include "rpc/rpc_session.h"
#include "peer/peer_session_message_handler.h"

namespace token{
  namespace peer{
    class Session: public rpc::Session{
      friend class PeerSessionThread;
    private:
      SessionMessageHandler handler_;
      utils::Address address_;
      // uv handles
      uv_connect_t connection_;
      uv_async_t disconnect_;
      uv_async_t send_version_;
      uv_async_t send_verack_;

      SessionMessageHandler& handler(){
        return handler_;
      }

      static void OnWalk(uv_handle_t* handle, void* arg);
      static void OnClose(uv_handle_t* handle);
      static void OnConnect(uv_connect_t* conn, int status);
      static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
      static void OnDisconnect(uv_async_t* handle);

      static void
      AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
        auto session = (SessionBase*)handle->data;
        LOG(INFO) << "allocating buffer of size " << suggested_size << "b for session: " << session->GetUUID();
        buf->base = (char*)malloc(suggested_size);
        buf->len = suggested_size;
      }

      static void OnSendVersion(uv_async_t* handle);
      static void OnSendVerack(uv_async_t* handle);
    public:
      Session():
        rpc::Session(),
        handler_(this),
        address_(),
        connection_(),
        disconnect_(),
        send_version_(),
        send_verack_(){
        connection_.data = this;
        disconnect_.data = this;
        send_version_.data = this;
        CHECK_UVRESULT2(FATAL, uv_async_init(GetLoop(), &send_version_, &OnSendVersion), "couldn't initialize send_version_ callback");
        send_verack_.data = this;
        CHECK_UVRESULT2(FATAL, uv_async_init(GetLoop(), &send_verack_, &OnSendVerack), "couldn't initialize send_verack_ callback");
      }
      Session(uv_loop_t* loop, utils::Address address):
        rpc::Session(loop),
        handler_(this),
        address_(address),
        connection_(),
        disconnect_(),
        send_version_(),
        send_verack_(){
        connection_.data = this;
        disconnect_.data = this;
        send_version_.data = this;
        CHECK_UVRESULT2(FATAL, uv_async_init(GetLoop(), &send_version_, &OnSendVersion), "couldn't initialize send_version_ callback");
        send_verack_.data = this;
        CHECK_UVRESULT2(FATAL, uv_async_init(GetLoop(), &send_verack_, &OnSendVerack), "couldn't initialize send_verack_ callback");
      }
      explicit Session(utils::Address address):
        Session(uv_loop_new(), address){}
      ~Session() override = default;

      utils::Address& address(){
        return address_;
      }

      utils::Address address() const{
        return address_;
      }

      bool SendVersion(){
        VERIFY_UVRESULT2(ERROR, uv_async_send(&send_version_), "couldn't invoke send_version_ callback");
        return true;
      }

      bool SendVerack(){
        VERIFY_UVRESULT2(ERROR, uv_async_send(&send_verack_), "couldn't invoke send_verack_ callback");
        return true;
      }

      bool Connect();
      bool Disconnect();
    };
  }
}
#endif //TOKEN_PEER_SESSION_H