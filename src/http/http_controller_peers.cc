#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller_peers.h"

#include "peer/peer_session_manager.h"

namespace token{
  void PeerController::HandleGetPeers(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);
    Json::String& body = response->GetBody();
    Json::Writer writer(body);

    if(!PeerSessionManager::GetConnectedPeers(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get peer info."));

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(response);
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE