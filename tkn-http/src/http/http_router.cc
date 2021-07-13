#include "../../include/http/http_router.h"
#include "../../include/http/http_request.h"

namespace token{
  namespace http{
    RouterMatch Router::Find(const RequestPtr& request){
      return Search(GetRoot(), request->GetMethod(), request->GetPath());
    }
  }
}