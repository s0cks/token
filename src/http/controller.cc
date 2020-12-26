#include "http/controller.h"
#include "peer/peer_session_manager.h"

namespace Token{
    static inline void
    GetPeerManagerStatus(Json::Value& doc){
        std::set<UUID> peers;
        if(!PeerSessionManager::GetConnectedPeers(peers)){
            LOG(WARNING) << "couldn't get list of peers from peer session manager.";
            return;
        }

        Json::Value list;
        for(auto& it : peers){
            std::shared_ptr<PeerSession> session = PeerSessionManager::GetSession(it);

            Json::Value pinfo;
            pinfo["ID"] = it.ToString();
            pinfo["Address"] = session->GetAddress().ToString();

            list.append(pinfo);
        }
        doc["Peers"] = list;
    }

    void StatusController::HandleOverallStatus(HttpSession* session, HttpRequest* request){
        Json::Value doc;
        doc["Timestamp"] = GetTimestampFormattedReadable(GetCurrentTimestamp());
        doc["Version"] = GetVersion();
        GetPeerManagerStatus(doc);

        SendJson(session, doc);
    }

    void StatusController::HandleTestStatus(HttpSession* session, HttpRequest* request){
        std::string name = request->GetParameterValue("name");

        std::stringstream ss;
        ss << "Hello " << name << "!";
        SendText(session, ss);
    }
}