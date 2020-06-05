#include "old/session.h"
#include "layout.h"

namespace Token{
    Session::Session():
        rbuffer_(kByteBufferSize),
        wbuffer_(kByteBufferSize),
        state_(SessionState::kDisconnected){}

    void
    Session::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Session* session = (Session*)handle->data;
        buff->base = (char*)session->rbuffer_.data();
        buff->len = suggested_size;
    }

    void Session::Send(Token::Message* msg){
        uint32_t type = static_cast<uint32_t>(msg->GetType());
        uint64_t size = msg->GetSize();
        LOG(INFO) << "sending " << msg->GetName() << "(" << size << " bytes)";
        GetWriteBuffer()->PutInt(type);
        GetWriteBuffer()->PutLong(size);
        GetWriteBuffer()->PutMessage(msg);
        uv_buf_t buff = uv_buf_init((char*)GetWriteBuffer()->data(), GetWriteBuffer()->GetWrittenBytes());
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, OnMessageSent);
    }

    void Session::OnMessageSent(uv_write_t *req, int status){
        if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        if(req){
            Session* session = (Session*)req->data;
            session->GetWriteBuffer()->clear();
            free(req);
        }
    }
}