#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <uv.h>
#include <memory>
#include <map>
#include "bytes.h"
#include "session.h"
#include "blockchain.h"

namespace Token{
    namespace Node{
        class Server{
        private:
            static const size_t MAX_BUFF_SIZE = 4096;

            uv_loop_t loop_;
            uv_tcp_t server_;
            ByteBuffer read_buff_;
            bool running_;
            User user_;
            BlockChain* chain_;
            std::map<uv_stream_t*, Session*> sessions_;

            void OnNewConnection(uv_stream_t* server, int status);
            void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSent(uv_write_t* stream, int status);
            void Send(uv_stream_t* conn, Message* msg);

            void RemoveClient(uv_stream_t* client){
                //TODO: Implement
            }

            inline void
            GetPeers(std::vector<PeerSession*>& peers){
                for(auto& it : sessions_){
                    if(it.second->IsPeerSession()){
                        peers.push_back(it.second->AsPeerSession());
                    }
                }
            }

            inline void
            GetClients(std::vector<ClientSession*>& clients){
                for(auto& it : sessions_){
                    if(it.second->IsClientSession()){
                        clients.push_back(it.second->AsClientSession());
                    }
                }
            }

            bool Handle(Session* session, Message* msg);

            Server():
                user_("TestUser"),
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

            bool IsRunning() const{
                return running_;
            }
        };
    }
}

#endif //TOKEN_NODE_H