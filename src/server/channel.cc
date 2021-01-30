#include "utils/buffer.h"
#include "server/channel.h"

namespace Token{
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

  void Channel::Send(const MessageList& messages){
    size_t total_messages = messages.size();
    if(total_messages <= 0){
      LOG(WARNING) << "not sending any messages!";
      return;
    }

    LOG(INFO) << "sending " << total_messages << " messages....";
    ChannelWriteData* data = new ChannelWriteData(this, 65536);
    uv_buf_t buffers[total_messages];

    int64_t offset = 0;
    for(size_t idx = 0; idx < total_messages; idx++){
      const MessagePtr& msg = messages[idx];
      LOG(INFO) << "sending " << msg->ToString();
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

  void Channel::OnMessageSent(uv_write_t* req, int status){
    if(status != 0)
      LOG(ERROR) << "failed to send message: " << uv_strerror(status);
    delete ((ChannelWriteData*)req->data);
  }
}