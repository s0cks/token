#include "server/http/router.h"
#include "server/http/request.h"

namespace Token{
  HttpRouterMatch HttpRouter::Find(const HttpRequestPtr& request){
    return Search(GetRoot(), request->GetMethod(), request->GetPath());
  }
}