#ifndef TOKEN_CHANNEL_H
#define TOKEN_CHANNEL_H

#include <uv.h>
#include "message.h"

namespace Token{
  template<class M>
  class Channel{
   private:
    typedef std::shared_ptr<M> ChannelMessagePtr;
    typedef std::vector<ChannelMessagePtr> ChannelMessageList;

    struct ChannelWriteData{
      uv_write_t request;
      Channel* channel;
      BufferPtr buffer;

      ChannelWriteData(Channel* channel, int64_t size):
        request(),
        channel(channel),
        buffer(Buffer::NewInstance(size)){}
      ~ChannelWriteData() = default;
    };
   protected:
    uv_loop_t* loop_;
    uv_tcp_t handle_;

    void SendMessages(const ChannelMessageList& messages){
      size_t total_messages = messages.size();
      if(total_messages <= 0){
        LOG(WARNING) << "not sending any messages!";
        return;
      }

      int64_t total_size = 0;
      std::for_each(messages.begin(), messages.end(), [&total_size](const ChannelMessagePtr& msg){
        total_size += msg->GetBufferSize();
      });

      LOG(INFO) << "sending " << total_messages << " messages....";
      ChannelWriteData* data = new ChannelWriteData(this, total_size);
      uv_buf_t buffers[total_messages];

      int64_t offset = 0;
      for(size_t idx = 0; idx < total_messages; idx++){
        const ChannelMessagePtr& msg = messages[idx];
#ifdef TOKEN_DEBUG
        LOG(INFO) << "sending " << msg->GetName() << " (" << msg->GetBufferSize() << " Bytes)";
#endif//TOKEN_DEBUG
        if(!msg->Write(data->buffer)){
          LOG(WARNING) << "couldn't write " << msg->ToString();
          return;
        }

        int64_t msg_size = msg->GetBufferSize();
        buffers[idx].base = &data->buffer->data()[offset];
        buffers[idx].len = msg_size;
        offset += msg_size;
      }

      uv_write(&data->request, GetStream(), buffers, total_messages, &OnMessageSent);
    }

    static void
    OnMessageSent(uv_write_t* req, int status){
      if(status != 0)
        LOG(ERROR) << "failed to send message: " << uv_strerror(status);
      delete ((ChannelWriteData*)req->data);
    }
   public:
    Channel(uv_loop_t* loop):
      loop_(loop),
      handle_(){
      int err;
      if((err = uv_tcp_init(loop_, &handle_)) != 0){
        LOG(ERROR) << "couldn't initialize the channel handle: " << uv_strerror(err);
        return;
      }

      if((err = uv_tcp_keepalive(GetHandle(), 1, 60)) != 0){
        LOG(WARNING) << "couldn't configure channel keep-alive: " << uv_strerror(err);
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
  };
}

#endif //TOKEN_CHANNEL_H