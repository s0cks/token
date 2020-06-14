#include "node/session.h"
#include "node/node.h"

namespace Token{
    void NodeClient::SetState(Token::NodeClient::State state){
        pthread_mutex_trylock(&smutex_);
        state_ = state;
        pthread_cond_signal(&scond_);
        pthread_mutex_unlock(&smutex_);
    }

    NodeClient::State NodeClient::GetState(){
        pthread_mutex_trylock(&smutex_);
        State state = state_;
        pthread_mutex_unlock(&smutex_);
        return state;
    }

    void NodeClient::WaitForState(NodeClient::State state){
        pthread_mutex_trylock(&smutex_);
        while(state_ != state) pthread_cond_wait(&scond_, &smutex_);
        pthread_mutex_unlock(&smutex_);
    }

    bool NodeClient::WaitForShutdown(){
        if(IsStarting()) WaitForState(State::kRunning);
        if(!IsRunning()) return false;
        return true;
    }
}