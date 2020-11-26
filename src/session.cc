#include "session.h"

namespace Token{
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)

    void Session::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Handle<Session> session = (Session*)handle->data;
        session->GetReadBuffer()->Initialize(buff);
    }

    void Session::SetState(Session::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    void Session::SetStatus(Session::Status status){
        LOCK;
        status_ = status;
        SIGNAL_ALL;
    }

    Session::State Session::GetState(){
        LOCK_GUARD;
        return state_;
    }

    Session::Status Session::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void Session::Send(const Handle<Message>& msg){
        // TODO: convert to message header struct
        uint32_t type = static_cast<uint32_t>(msg->GetType());
        intptr_t size = msg->GetMessageSize();
        intptr_t total_size = Message::kHeaderSize + size;

        LOG(INFO) << "sending " << msg << " (" << total_size << " bytes)";
        Handle<Buffer> buffer = GetWriteBuffer();
        buffer->PutUnsignedInt(type);
        buffer->PutLong(size);
        if(!msg->Write(buffer)){
            LOG(WARNING) << "couldn't encode message: " << msg->ToString();
            return;
        }

        uv_buf_t buff;
        buffer->Initialize(&buff);

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, &OnMessageSent);
    }

    void Session::Send(std::vector<Handle<Message>>& messages){
        //TODO: fix SessionBase::Send(std::vector<Handle<Message>>&)
        size_t total_messages = messages.size();
        if(total_messages <= 0){
            LOG(WARNING) << "not sending any messages!";
            return;
        }

        LOG(INFO) << "sending " << total_messages << " messages....";
        uv_buf_t buffers[total_messages];
        for(size_t idx = 0; idx < total_messages; idx++){
            Handle<Message> msg = messages[idx];


            uint32_t type = static_cast<uint32_t>(msg->GetMessageType());
            intptr_t size = msg->GetMessageSize();
            intptr_t total_size = Message::kHeaderSize + size;

            LOG(INFO) << "sending " << msg->ToString() << " (" << total_size << " Bytes)";

            Handle<Buffer> buffer = GetWriteBuffer();
            buffer->PutInt(type);
            buffer->PutLong(size);
            if(!msg->Write(buffer)){
                LOG(WARNING) << "couldn't encode message: " << msg->ToString();
                return;
            }

            buffer->Initialize(&buffers[idx]);
        }
        messages.clear();

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), buffers, total_messages, &OnMessageSent);

        for(size_t idx = 0; idx < total_messages; idx++) free(buffers[idx].base);
    }

    void Session::OnMessageSent(uv_write_t* req, int status){
        if(status != 0)
            LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        free(req);
    }
}