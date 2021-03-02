#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller_peers.h"

#include "peer/peer_session_manager.h"

namespace token{
  void PeerController::HandleGetPeers(HttpSession* session, const HttpRequestPtr& request){
    Json::String body;
    Json::Writer writer(body);

    if(!PeerSessionManager::GetConnectedPeers(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get peer info."));
    return session->Send(NewOkResponse(session, body));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE