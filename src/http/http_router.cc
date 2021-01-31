#include "http/http_router.h"
#include "http/http_request.h"

namespace Token{
  HttpRouterMatch HttpRouter::Find(const HttpRequestPtr& request){
    return Search(GetRoot(), request->GetMethod(), request->GetPath());
  }
}