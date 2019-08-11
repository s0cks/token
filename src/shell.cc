#include "shell.h"
#include <string>

namespace Token{
    static ByteBuffer READ_BUFFER = ByteBuffer();

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        *buff = uv_buf_init((char*)READ_BUFFER.GetBytes(), READ_BUFFER.Size());
    }

    void BlockChainServerShell::OnRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        if(nread == UV_EOF){
            uv_read_stop(stream);
        } else if(nread > 0){
            std::string command(buff->base, nread - 1);
            std::cout << "len: " << command.size() << "/" << nread << std::endl;
            if(command == "gethead"){
                std::cout << "Sending get head" << std::endl;
                Message m(Message::Type::kGetHeadMessage);
                GetPeer()->Send(GetPeer()->GetStream(), &m);
            } else if(command == "append"){
                //22EDF1BD83B68A18DD40B81BFC36A14C82CB489D955639B2C1B9324973A65859

            }
            READ_BUFFER.Clear();
        } else{
            uv_read_stop(stream);
        }
    }

    BlockChainServerShell::BlockChainServerShell(uv_loop_t *loop, PeerSession* peer):
        input_(),
        peer_(peer),
        stream_(peer->GetStream()),
        loop_(loop){
        uv_pipe_init(loop_, &input_, false);
        uv_pipe_open(&input_, 0);
        input_.data = this;
        uv_read_start((uv_stream_t*)&input_, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
            BlockChainServerShell* shell = (BlockChainServerShell*) stream->data;
            shell->OnRead(stream, nread, buf);
        });
    }
}