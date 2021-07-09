#ifndef TOKEN_RPC_CLIENT_H
#define TOKEN_RPC_CLIENT_H

#include "block.h"
#include "network/rpc_session.h"

namespace token{
  namespace rpc{
    class Client{
     private:
      NodeAddress address_;

      class ClientSession : public rpc::Session{
       private:
        NodeAddress address_;
        uv_connect_t connection_;

        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
          buff->base = (char*)malloc(suggested_size);
          buff->len = suggested_size;
        }

        static void OnConnect(uv_connect_t* handle, int status){
          auto session = (ClientSession*)handle->data;
          if(status != 0){
            LOG(ERROR) << "connect failure: " << uv_strerror(status);
            return session->CloseConnection();
          }

          BlockHeader head = Block::Genesis()->GetHeader();
          session->Send(VersionMessage::NewInstance(Clock::now(), ClientType::kClient, Version::CurrentVersion(), Hash::GenerateNonce(), session->GetUUID(), head);

          CHECK_UVRESULT2(ERROR, uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)), "read failure");
        }

        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
          auto session = (ClientSession*)stream->data;
          if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected.";
            return;
          }

          BufferPtr buffer = Buffer::From(buff->base, nread);
          rpc::MessagePtr msg = rpc::Message::From(buffer);
          DLOG(INFO) << "response: " << msg->ToString();
        }
       public:
        explicit Session(const NodeAddress& address):
          rpc::Session(uv_loop_new()),
          address_(address),
          connection_(){
          connection_.data = this;
        }
        ~Session() override{
          CHECK_UVRESULT2(FATAL, uv_loop_close(loop_), "cannot close rpc::Client::Session loop.");
          free(loop_);
        }

        bool Connect(){
          struct sockaddr_in address;
          VERIFY_UVRESULT2(FATAL, uv_ip4_addr(address_.GetAddress().c_str(), address_.port(), &address), "uv_ip4_addr failure");

          DLOG(INFO) << "creating connection to peer @ " << address() << "....";
          VERIFY_UVRESULT2(FATAL, uv_tcp_connect(&connection_, GetHandle(), (const struct sockaddr*)&address, &OnConnect), "couldn't connect to peer");
          VERIFY_UVRESULT2(FATAL, uv_run(GetLoop(), UV_RUN_DEFAULT), "uv_run failure");
          return true;
        }
      };
     public:
      explicit Client(const NodeAddress& address):
        address_(address){}
      Client(const std::string& address, const int& port):
        Client(NodeAddress(address, port)){}
      ~Client() = default;

      NodeAddress& address(){
        return address_;
      }

      NodeAddress address() const{
        return address_;
      }
    };
  }
}

#endif//TOKEN_RPC_CLIENT_H