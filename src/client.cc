#include "client.h"

namespace Token{
    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Client* client = reinterpret_cast<Client*>(handle->data);
        ByteBuffer* bb = client->GetReadBuffer();
        *buff = uv_buf_init((char*)bb->GetBytes(), static_cast<unsigned int>(suggested_size));
    }

    Message*
    Client::GetNextMessage(){
        uint32_t type = GetReadBuffer()->GetInt();
        Message* msg = Message::Decode(type, GetReadBuffer());
        GetReadBuffer()->Clear();
        return msg;
    }

    void Client::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        if(nread == UV_EOF){
            std::cerr << "Disconnected" << std::endl;
            uv_read_stop(stream);
        } else if(nread > 0){
            GetReadBuffer()->PutBytes((uint8_t*) buff->base, static_cast<uint32_t>(buff->len));
            Message* msg = GetNextMessage();
            // Handle Message
        } else if(nread == 0){
            std::cout << "Read 0" << std::endl;
            GetReadBuffer()->Clear();
        } else{
            std::cerr << "Error reading" << std::endl;
            uv_read_stop(stream);
        }
    }

    void Client::OnConnect(uv_connect_t *connection, int status){
        if(status == 0){
            std::cout << "Connected" << std::endl;
            uv_read_start(connection_.handle, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                Client* client = reinterpret_cast<Client*>(stream->data);
                client->OnMessageRead(stream, nread, buff);
            });
            handle_ = connection->handle;
        } else{
            std::cerr << "Connection Unsuccessful" << std::endl;
            std::cerr << uv_strerror(status) << std::endl;
        }
    }

    void Client::OnMessageSend(uv_write_t *request, int status){
        if(status == 0){
            if(request) free(request);
        } else{
            std::cerr << "Error writing message" << std::endl;
        }
    }

    void Client::Initialize(const std::string &address, int port){
        uv_tcp_init(&loop_, &stream_);
        uv_ip4_addr(address.c_str(), port, &address_);
        connection_.data = this;
        uv_tcp_connect(&connection_, &stream_, (const struct sockaddr*)&address_, [](uv_connect_t* conn, int status){
            Client* client = reinterpret_cast<Client*>(conn->data);
            client->OnConnect(conn, status);
        });
    }

    void Client::Send(Token::Message *msg){
        std::cout << "Sending " << msg->GetName() << std::endl;
        ByteBuffer bb;
        msg->Encode(&bb);

        uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
        uv_write_t* request = (uv_write_t*) malloc(sizeof(uv_write_t));
        request->data = this;
        uv_write(request, handle_, &buff, 1, [](uv_write_t* request, int status){
            Client* client = reinterpret_cast<Client*>(request->data);
            client->OnMessageSend(request, status);
        });
    }

    Client* Client::Connect(const std::string &address, int port){
        Client* client = new Client();
        client->Initialize(address, port);
        return client;
    }
}