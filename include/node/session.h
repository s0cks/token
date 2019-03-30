#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <iostream>
#include <memory>
#include "message.h"

namespace Token{
    namespace Node{
        class Session{
        private:
            std::shared_ptr<uv_tcp_t> conn_;
            ByteBuffer read_buff_;

            inline uv_tcp_t*
            GetConnection() const{
                return conn_.get();
            }

            inline ByteBuffer*
            GetReadBuffer() {
                return &read_buff_;
            }

            inline void
            Append(const char* buff, size_t len){
                GetReadBuffer()->PutBytes((uint8_t*)buff, len);
            }

            friend class Server;
        public:
            Session(std::shared_ptr<uv_tcp_t> conn):
                read_buff_(),
                conn_(conn){}
            ~Session(){}

            Message* GetMessage(){
                uint32_t type = GetReadBuffer()->GetInt();
                Message* msg = Message::Decode(type, GetReadBuffer());
                GetReadBuffer()->Clear();
                return msg;
            }
        };
    }
}

#endif //TOKEN_SESSION_H