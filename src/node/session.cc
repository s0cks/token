#include "node/session.h"

namespace Token{
    void Session::OnMessageSent(uv_write_t *req, int status){
        if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        if(req) free(req);
    }

    void Session::Send(Message* msg){
        uint32_t type = static_cast<uint32_t>(msg->GetMessageType());
        uint64_t size = msg->GetMessageSize();
        uint64_t total_size = Message::kHeaderSize + size;

        uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*total_size);
        memcpy(&bytes[Message::kTypeOffset], &type, Message::kTypeLength);
        memcpy(&bytes[Message::kSizeOffset], &size, Message::kSizeLength);
        if(!msg->Encode(&bytes[Message::kDataOffset], size)){
            LOG(WARNING) << "couldn't encode message " << msg->ToString();
            return;
        }

        uv_buf_t buff = uv_buf_init((char*)bytes, total_size);
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, &OnMessageSent);
    }

    void Session::Send(std::vector<Message*>& messages){
        size_t total_messages = messages.size();
        uv_buf_t buffers[total_messages];

        for(size_t idx = 0; idx < total_messages; idx++){
            Message* msg = messages[idx];

            uint32_t type = static_cast<uint32_t>(msg->GetMessageType());
            uint64_t size = msg->GetMessageSize();
            uint64_t total_size = Message::kHeaderSize + size;

            uint8_t* bytes = (uint8_t*)malloc(total_size);
            memcpy(&bytes[Message::kTypeOffset], &type, Message::kTypeLength);
            memcpy(&bytes[Message::kSizeOffset], &size, Message::kSizeLength);
            if(!msg->Encode(&bytes[Message::kDataOffset], size)){
                LOG(WARNING) << "couldn't encode message #" << idx << ": " << msg;
                continue;
            }

            buffers[idx] = uv_buf_init((char*)bytes, total_size);
        }
        messages.clear();

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), buffers, total_messages, &OnMessageSent);

        for(size_t idx = 0; idx < total_messages; idx++) free(buffers[idx].base);
    }
}