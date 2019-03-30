#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <uv.h>
#include <memory>
#include <map>
#include "bytes.h"
#include "session.h"
#include "peer.h"
#include "blockchain.h"

namespace Token{
    namespace Node{
        class Server{
        private:
            typedef std::map<uv_stream_t*, Session> SessionMap;

            static const size_t MAX_BUFF_SIZE = 4096;

            uv_loop_t loop_;
            uv_tcp_t server_;
            ByteBuffer read_buff_;
            bool running_;
            SessionMap sessions_;
            User user_;
            BlockChain* chain_;
            std::vector<Peer*> peers_;

            void OnNewConnection(uv_stream_t* server, int status);
            void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSent(uv_write_t* stream, int status);
            void Send(uv_stream_t* conn, Message* msg);

            void RemoveClient(uv_stream_t* client){
                //TODO: Implement
            }

            Server():
                user_("TestUser"),
                chain_(BlockChain::CreateInstance(&user_)),
                sessions_(),
                running_(false),
                server_(),
                loop_(),
                read_buff_(MAX_BUFF_SIZE){}
            ~Server(){}
        public:
            static Server* GetInstance();
            static void AddPeer(const std::string& addr, int port);
            static int Initialize(int port);
            static int Initialize(int port, const std::string& paddr, int pport);

            void Broadcast(uv_stream_t* from, Message* msg);

            ByteBuffer* GetReadBuffer(){
                return &read_buff_;
            }

            BlockChain* GetBlockChain() const{
                return chain_;
            }

            Block* GetHead() const{
                return GetBlockChain()->GetHead();
            }

            bool IsRunning() const{
                return running_;
            }
        };
    }
}

#endif //TOKEN_NODE_H