#include "http/controller_health.h"

namespace Token{
  void HealthController::HandleGetReadyStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }

  void HealthController::HandleGetLiveStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }
}