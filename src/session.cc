#include "session.h"

namespace Token{
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)

    void Session::WaitForState(State state){
        //TODO: add a timeout to Session::WaitForState(State)
        LOCK;
        while(state_ != state) WAIT;
    }

    void Session::WaitForItem(const InventoryItem& item){
        LOCK;
        while(!item.ItemExists()) WAIT;
    }

    void Session::OnItemReceived(const InventoryItem& item){
        LOCK;// wuuuuuuuuut???
        SIGNAL_ALL;
    }

    void Session::OnNextMessageReceived(const Handle<Message>& msg){
        LOCK;
        next_ = msg;
        SIGNAL_ALL;
    }

    Handle<Message> Session::GetNextMessage(){
        LOCK;
        return next_;
    }

    void Session::SetState(Session::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    Session::State Session::GetState() {
        LOCK_GUARD;
        return state_;
    }

    void Session::SetID(const std::string& id){
        uuid_parse(id.c_str(), uuid_);
    }

    void Session::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        //TODO: buff->base = (char*)Allocator::Allocate(kBufferSize);
        buff->base = (char*)malloc(kBufferSize);
        buff->len = kBufferSize;
    }

    void Session::OnMessageSent(uv_write_t *req, int status){
        if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        if(req) free(req);
    }

    //TODO: there seems to be some memory issues
    void Session::Send(const Handle<Message>& msg){
        uint32_t type = static_cast<uint32_t>(msg->GetMessageType());
        intptr_t size = msg->GetMessageSize();
        intptr_t total_size = Message::kHeaderSize + size;

        LOG(INFO) << "sending " << msg << " (" << size << " bytes)";

        ByteBuffer bytes(total_size);
        bytes.PutInt(type);
        bytes.PutLong(size);
        if(!msg->WriteMessage(&bytes)){
            LOG(WARNING) << "couldn't encode message: " << msg->ToString();
            return;
        }

        uv_buf_t buff = uv_buf_init((char*)bytes.data(), total_size);
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, &OnMessageSent);
    }

    //TODO: there seems to be some memory issues
    void Session::Send(std::vector<Handle<Message>>& messages){
        size_t total_messages = messages.size();
        if(total_messages <= 0){
            LOG(WARNING) << "not sending any messages!";
            return;
        }

        LOG(INFO) << "sending " << total_messages << " messages....";
        uv_buf_t buffers[total_messages];
        for(size_t idx = 0; idx < total_messages; idx++){
            Handle<Message> msg = messages[idx];

            LOG(INFO) << "sending " << msg->ToString();

            uint32_t type = static_cast<uint32_t>(msg->GetMessageType());
            intptr_t size = msg->GetMessageSize();
            intptr_t total_size = Message::kHeaderSize + size;

            ByteBuffer* bytes = new ByteBuffer(total_size); // this is bad?
            bytes->PutInt(type);
            bytes->PutLong(size);
            if(!msg->WriteMessage(bytes)){
                LOG(WARNING) << "couldn't encode message: " << msg->ToString();
                return;
            }
            buffers[idx] = uv_buf_init((char*)bytes->data(), total_size);
        }
        messages.clear();

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), buffers, total_messages, &OnMessageSent);

        for(size_t idx = 0; idx < total_messages; idx++) free(buffers[idx].base);
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
}