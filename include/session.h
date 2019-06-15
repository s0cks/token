#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <iostream>
#include <memory>
#include "message.h"

namespace Token{
    class Session{
    private:
        std::shared_ptr<uv_tcp_t> connection_;
        ByteBuffer read_buffer_;

        inline uv_tcp_t*
        GetConnection() const{
            return connection_.get();
        }

        inline ByteBuffer*
        GetReadBuffer(){
            return &read_buffer_;
        }

        inline void
        Append(const char* buffer, size_t len){
            GetReadBuffer()->PutBytes((uint8_t*) buffer, static_cast<uint32_t>(len));
        }

        friend class BlockChain;
    public:
        Session(std::shared_ptr<uv_tcp_t> connection):
            read_buffer_(),
            connection_(connection){}
        ~Session(){}

        Message* GetNextMessage(){
            uint32_t type = GetReadBuffer()->GetInt();
            Message* msg = Message::Decode(type, GetReadBuffer());
            GetReadBuffer()->Clear();
            return msg;
        }
    };
}

#endif //TOKEN_SESSION_H