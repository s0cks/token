#include "session.h"

namespace Token{
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)

    void Session::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Handle<Session> session = (Session*)handle->data;
        Handle<Buffer> rbuff = session->GetReadBuffer();
        buff->len = suggested_size;
        buff->base = rbuff->data();
    }

    void Session::SetState(Session::State state){
        state_ = state;
        SIGNAL_ALL;
    }

    void Session::SetStatus(Session::Status status){
        status_ = status;
        SIGNAL_ALL;
    }

    bool Session::WaitForState(State state, intptr_t timeout){
        LOCK;
        while(state_ != state) WAIT;
        return true;
    }

    bool Session::WaitForStatus(Status status, intptr_t timeout){
        LOCK;
        while(status_ != status) WAIT;
        return true;
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
        int32_t type = msg->GetMessageType();
        int64_t size = msg->GetMessageSize();
        int64_t total_size = Message::kHeaderSize + size;

        LOG(INFO) << "sending " << msg << " (" << total_size << " bytes)";
        Handle<Buffer> wbuff = GetWriteBuffer();
        wbuff->PutInt(type);
        wbuff->PutLong(size);
        if(!msg->Write(wbuff)){
            LOG(WARNING) << "couldn't encode message: " << msg->ToString();
            return;
        }

        uv_buf_t buff;
        buff.len = total_size;
        buff.base = wbuff->data();

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

            Handle<Buffer> wbuff = GetWriteBuffer();
            wbuff->PutInt(type);
            wbuff->PutLong(size);
            if(!msg->Write(wbuff)){
                LOG(WARNING) << "couldn't encode message: " << msg->ToString();
                return;
            }

            buffers[idx].len = total_size;
            buffers[idx].base = &wbuff->data()[0];
        }
        messages.clear();

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), buffers, total_messages, &OnMessageSent);
    }

    void Session::OnMessageSent(uv_write_t* req, int status){
        Handle<Session> session = (Session*)req->data;
        session->GetWriteBuffer()->Reset();
        if(status != 0)
            LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        free(req);
    }

    void Session::SendInventory(std::vector<InventoryItem>& items){
        std::vector<Handle<Message>> data;

        size_t n = InventoryMessage::kMaxAmountOfItemsPerMessage;
        size_t size = (items.size() - 1) / n + 1;
        for(size_t idx = 0; idx < size; idx++){
            auto start = std::next(items.cbegin(), idx*n);
            auto end = std::next(items.cbegin(), idx*n+n);

            std::vector<InventoryItem> inv(n);
            if(idx*n+n > items.size()){
                end = items.cend();
                inv.resize(items.size()-idx*n);
            }
            std::copy(start, end, inv.begin());

            data.push_back(InventoryMessage::NewInstance(inv).CastTo<Message>());
        }
        Send(data);
    }

    bool ThreadedSession::Disconnect(){
        if(pthread_self() == thread_){
            // Inside Session OSThreadBase
            uv_read_stop(GetStream());
            uv_stop(GetLoop());
            uv_loop_close(GetLoop());
            SetState(State::kDisconnected);
        } else{
            // Outside Session OSThreadBase
            uv_async_send(&shutdown_);
        }
        return true;
    }
}