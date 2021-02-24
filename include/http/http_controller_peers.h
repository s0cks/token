#ifndef TOKEN_HTTP_CONTROLLER_PEERS_H
#define TOKEN_HTTP_CONTROLLER_PEERS_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller.h"

namespace token{
  class PeerController : public HttpController{
   private:
    PeerController() = delete;

    HTTP_CONTROLLER_ENDPOINT(GetPeers);
   public:
    ~PeerController() = delete;

    HTTP_CONTROLLER_INIT(){
      HTTP_CONTROLLER_GET("/peers", GetPeers);
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_PEERS_H