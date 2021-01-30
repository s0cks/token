#ifndef TOKEN_CHANNEL_H
#define TOKEN_CHANNEL_H

#include <uv.h>
#include "message.h"

namespace Token{
  class Channel{
   private:
    uv_loop_t* loop_;
    uv_tcp_t handle_;

    static void OnMessageSent(uv_write_t* req, int status);
   public:
    Channel(uv_loop_t* loop):
        loop_(loop),
        handle_(){
      int err;
      if((err = uv_tcp_keepalive(GetHandle(), 1, 60)) != 0){
        LOG(WARNING) << "couldn't configure channel keep-alive: " << uv_strerror(err);
        return;
      }

      if((err = uv_tcp_init(loop_, &handle_)) != 0){
        LOG(ERROR) << "couldn't initialize the channel handle: " << uv_strerror(err);
        return;
      }
    }
    ~Channel() = default;

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    uv_tcp_t* GetHandle(){
      return &handle_;
    }

    uv_stream_t* GetStream(){
      return (uv_stream_t*) &handle_;
    }

    void Send(const MessageList& messages);
  };
}

#endif //TOKEN_CHANNEL_H