#include "node/session.h"
#include "node/task.h"

namespace Token{
    void NodeClient::AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff){

    }

    void NodeClient::OnSignal(uv_signal_t* handle, int signum){
        NodeClient* client = (NodeClient*)handle->data;
        uv_stop(client->loop_);
    }

    void NodeClient::Connect(const NodeAddress& addr){
        uv_loop_init(loop_);
        uv_signal_init(loop_, &sigterm_);
        uv_signal_start(&sigterm_, &OnSignal, SIGTERM);

        uv_signal_init(loop_, &sigint_);
        uv_signal_start(&sigterm_, &OnSignal, SIGINT);

        uv_tcp_init(loop_, &stream_);
        uv_tcp_keepalive(&stream_, 1, 60);

        struct sockaddr_in address;
        int err;
        if((err = addr.GetSocketAddressIn(&address)) != 0){
            LOG(WARNING) << "couldn't resolved address '" << addr << "': " << uv_strerror(err);
            goto cleanup;
        }

        uv_pipe_init(loop_, &stdin_, 0);
        uv_pipe_open(&stdin_, 0);

        uv_pipe_init(loop_, &stdout_, 0);
        uv_pipe_open(&stdout_, 1);

        if((err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr*)&address, &OnConnect)) != 0){
            LOG(WARNING) << "couldn't connect to peer: " << uv_strerror(err);
            goto cleanup;
        }

        uv_run(loop_, UV_RUN_DEFAULT);
        uv_signal_stop(&sigterm_);
        uv_signal_stop(&sigint_);
        uv_read_stop((uv_stream_t*)&stdin_);

    cleanup:
        uv_loop_close(loop_);
    }

    void NodeClient::OnConnect(uv_connect_t* conn, int status){
        if(status != 0){
            LOG(WARNING) << "error connecting to server: " << uv_strerror(status);
            return; //TODO: terminate connection
        }

        NodeClient* client = (NodeClient*)conn->data;

        int err;
        if((err = uv_read_start(client->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << " client read error: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        if((err = uv_read_start(client->GetStdinPipe(), &AllocBuffer, &OnCommandReceived)) != 0){
            LOG(ERROR) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
            return; //TODO: terminate connection
        }
    }

    void NodeClient::OnMessageSent(uv_write_t* req, int status){
        if(status != 0) LOG(WARNING) << "failed to send message: " << uv_strerror(status);
        if(req->data) free(req->data);
        free(req);
    }

    void NodeClient::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        NodeClient* client = (NodeClient*)stream->data;
        if(nread == UV_EOF){
            LOG(WARNING) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(WARNING) << "[" << nread << "] client read error: " << uv_strerror(nread);
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread >= 4096){
            LOG(WARNING) << "too large of a buffer";
            return;
        }

        Message* msg;
        if(!(msg = Message::Decode((uint8_t*)buff->base, buff->len))){
            LOG(WARNING) << "couldn't decode message!";
            return;
        }

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new HandleMessageTask(client, msg);
        switch(msg->GetType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::Type::k##Name##Message: \
                uv_queue_work(stream->loop, work, &Handle##Name##Message, AfterHandleMessage); \
                break;
            FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
            case Message::Type::kUnknownMessage:
            default: //TODO: handle properly
                return;
        }
    }

    void NodeClient::HandleVersionMessage(uv_work_t* handle){}
    void NodeClient::HandleVerackMessage(uv_work_t *handle){}
    void NodeClient::HandleGetDataMessage(uv_work_t *handle){}
    void NodeClient::HandleInventoryMessage(uv_work_t *handle){}
    void NodeClient::HandleTransactionMessage(uv_work_t *handle){}
    void NodeClient::HandleBlockMessage(uv_work_t *handle){}
    void NodeClient::HandleCommitMessage(uv_work_t *handle){}
    void NodeClient::HandlePrepareMessage(uv_work_t *handle){}
    void NodeClient::HandlePromiseMessage(uv_work_t *handle){}
    void NodeClient::HandleRejectedMessage(uv_work_t *handle){}
    void NodeClient::HandleAcceptedMessage(uv_work_t *handle){}

    void NodeClient::OnCommandReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){

    }

    void NodeClient::AfterHandleMessage(uv_work_t *handle, int status){

    }
}