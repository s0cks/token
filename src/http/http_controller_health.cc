#include "http/http_controller_health.h"

namespace token{
  void HealthController::HandleGetReadyStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }

  void HealthController::HandleGetLiveStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }
}