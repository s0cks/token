#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include "common.h"
#include "session.h"

namespace Token{
    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        const char* address_;
        int port_;

        static void* PeerSessionThread(void* data);
        static void OnConnect(uv_connect_t* conn, int status);
    protected:
        uv_stream_t* GetStream() const{
            return conn_.handle;
        }
    public:
        PeerSession(const char* address, int port):
            thread_(),
            conn_(),
            address_(address),
            port_(port){
            conn_.data = this;
        }
        ~PeerSession(){}

        const char* GetAddress() const{
            return address_;
        }

        int GetPort() const{
            return port_;
        }

        bool Connect(){
            return pthread_create(&thread_, NULL, &PeerSessionThread, (void*)this) == 0;
        }
    };
}

#endif //TOKEN_PEER_H