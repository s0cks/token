#include "node/node_info.h"
#include "node/session.h"

namespace Token{
    static inline PeerInfo::State
    GetPeerState(Session::State state){
        switch(state){
            case Session::kConnecting: return PeerInfo::kConnecting;
            case Session::kConnected: return PeerInfo::kConnected;
            case Session::kDisconnected: return PeerInfo::kDisconnected;
            default: return PeerInfo::kDisconnected;
        }
    }

    PeerInfo::PeerInfo(PeerSession* session):
        state_(GetPeerState(session->GetState())),
        info_(session->GetInfo()){}
}