#include "server/http/controller/health_controller.h"

namespace Token{
  void HealthController::HandleGetReadyStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }

  void HealthController::HandleGetLiveStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }
}