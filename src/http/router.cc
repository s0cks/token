#include "http/router.h"
#include "http/request.h"

namespace Token{
  HttpRouterMatch HttpRouter::Find(const HttpRequestPtr& request){
    return Search(GetRoot(), request->GetMethod(), request->GetPath());
  }
}